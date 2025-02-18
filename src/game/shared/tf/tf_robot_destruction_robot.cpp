//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The robots for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//

#include "cbase.h"
#include "tf_logic_robot_destruction.h"
#include "tf_shareddefs.h"
#include "particle_parse.h"
#include "tf_gamerules.h"
#include "debugoverlay_shared.h"
#ifdef  GAME_DLL
	#include "tf_ammo_pack.h"
	#include "entity_bonuspack.h"
	#include "entity_capture_flag.h"
	#include "effect_dispatch_data.h"
	#include "te_effect_dispatch.h"
	#include "tf_gamestats.h"
	#include "eventlist.h"
#else
	#include "eventlist.h"
#endif

#ifdef GAME_DLL
	extern ConVar tf_obj_gib_velocity_min;
	extern ConVar tf_obj_gib_velocity_max;
	extern ConVar tf_obj_gib_maxspeed;
#endif

#define ADD_POINTS_CONTEXT "add_points_context"
#define SPEW_BARS_CONTEXT "spew_bars_context"
#define SELF_DESTRUCT_THINK "self_destruct_think"
#define ANIMS_THINK	"anims_think"

ConVar tf_rd_robot_repair_rate( "tf_rd_robot_repair_rate", "60", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

RobotData_t* g_RobotData[ NUM_ROBOT_TYPES ] =
{ 
					// Model									// Busted model										// Pain			// Death		// Collide		// Idle				// Bar offset
	new RobotData_t( "models/bots/bot_worker/bot_worker_A.mdl",	"models/bots/bot_worker/bot_worker_A.mdl",			"Robot.Pain",	"Robot.Death",	"Robot.Collide", "Robot.Greeting", -35.f ),
	new RobotData_t( "models/bots/bot_worker/bot_worker2.mdl", "models/bots/bot_worker/bot_worker2.mdl",			"Robot.Pain",	"Robot.Death",	"Robot.Collide", "Robot.Greeting", -30.f ),
	new RobotData_t( "models/bots/bot_worker/bot_worker3.mdl",	"models/bots/bot_worker/bot_worker3_nohead.mdl",	"Robot.Pain",	"Robot.Death",	"Robot.Collide", "Robot.Greeting", -10.f ),
};

#define ROBOT_DEATH_EXPLOSION "RD.BotDeathExplosion"
#define SCORING_POINTS_PARTICLE_EFFECT "bot_radio_waves"
#define DAMAGED_ROBOT_PARTICLE_EFFECT "sentrydamage_4"
#define DEATH_PARTICLE_EFFECT "rd_robot_explosion"


void RobotData_t::Precache()
{
	if ( GetStringData( MODEL_KEY ) )
	{
		CBaseEntity::PrecacheModel( GetStringData( MODEL_KEY ) );
		PrecacheGibsForModel( modelinfo->GetModelIndex( GetStringData( MODEL_KEY ) ) ); 
		PrecachePropsForModel( modelinfo->GetModelIndex( GetStringData( MODEL_KEY ) ), "spawn" );
	}
	if ( GetStringData( DAMAGED_MODEL_KEY ) ) CBaseEntity::PrecacheModel( GetStringData( DAMAGED_MODEL_KEY ) );
	if ( GetStringData( HURT_SOUND_KEY ) ) CBaseEntity::PrecacheScriptSound( GetStringData( HURT_SOUND_KEY ) );
	if ( GetStringData( DEATH_SOUND_KEY ) ) CBaseEntity::PrecacheScriptSound( GetStringData( DEATH_SOUND_KEY ) );
	if ( GetStringData( COLLIDE_SOUND_KEY ) ) CBaseEntity::PrecacheScriptSound( GetStringData( COLLIDE_SOUND_KEY ) );
	if ( GetStringData( IDLE_SOUND_KEY ) ) CBaseEntity::PrecacheScriptSound( GetStringData( IDLE_SOUND_KEY ) );
}


CTFRobotDestruction_RobotAnimController::CTFRobotDestruction_RobotAnimController( CTFRobotDestruction_Robot *pOuter )
	: m_vecOldOrigin( vec3_origin )
	, m_vecLean( vec3_origin )
	, m_pOuter( pOuter )
	, m_vecImpulse( vec3_origin )
{}

void CTFRobotDestruction_RobotAnimController::Update()
{
	if( !m_pOuter )
		return;

	CStudioHdr *pStudioHdr = m_pOuter->GetModelPtr();
	if ( !pStudioHdr )
		return;

	const Vector vecNewOrigin = m_pOuter->GetAbsOrigin();
	const Vector vecVelocity = m_vecOldOrigin - vecNewOrigin;
	m_vecOldOrigin = vecNewOrigin;

	Approach( m_vecLean, vecVelocity + m_vecImpulse, 2.f );
	GetPoseParams();
	Approach( m_vecImpulse, vec3_origin, 200.f );

	Vector vecForward, vecRight;
	AngleVectors( m_pOuter->GetAbsAngles(), &vecForward, &vecRight, NULL );

	float flRightLean = vecRight.Dot( m_vecLean );
	float flForwardLean = vecForward.Dot( m_vecLean );

	m_pOuter->SetPoseParameter( m_poseParams.m_nMoveX, flForwardLean );
	m_pOuter->SetPoseParameter( m_poseParams.m_nMoveY, flRightLean );
}

void CTFRobotDestruction_RobotAnimController::Impulse( const Vector& vecImpulse )
{ 
	m_vecImpulse += vecImpulse * 5;
}

void CTFRobotDestruction_RobotAnimController::Approach( Vector& vecIn, const Vector& vecTarget, float flRate )
{
	Vector vecApproach = ( vecTarget - vecIn ) * flRate * gpGlobals->frametime;
	if ( vecApproach.LengthSqr() > ( vecIn - vecTarget ).LengthSqr() )
		vecIn = vecTarget;
	else
		vecIn += vecApproach;		
}

void CTFRobotDestruction_RobotAnimController::GetPoseParams()
{
	m_poseParams.m_nMoveX = m_pOuter->LookupPoseParameter( "move_x" );
	m_poseParams.m_nMoveY = m_pOuter->LookupPoseParameter( "move_y" );
}

IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotDestruction_Robot, DT_TFRobotDestruction_Robot )

BEGIN_NETWORK_TABLE( CTFRobotDestruction_Robot, DT_TFRobotDestruction_Robot  )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO(m_iHealth) ),
	RecvPropInt( RECVINFO(m_iMaxHealth) ),
	RecvPropInt( RECVINFO(m_eType) ),
#else
	SendPropInt(SENDINFO(m_iHealth), -1, SPROP_VARINT ),
	SendPropInt(SENDINFO(m_iMaxHealth), -1, SPROP_VARINT ),
	SendPropInt(SENDINFO(m_eType), -1, SPROP_VARINT ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFRobotDestruction_Robot )
#ifdef GAME_DLL
	DEFINE_INPUTFUNC( FIELD_VOID, "StopAndUseComputer", InputStopAndUseComputer ),
#endif
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_robot_destruction_robot, CTFRobotDestruction_Robot );

#ifdef _WIN32
// Old code, stricter compiler
#pragma warning(disable : 4355) // warning C4355: 'this': used in base member initializer list
#endif
CTFRobotDestruction_Robot::CTFRobotDestruction_Robot()
	: m_animController( this )
{
#ifdef GAME_DLL
	m_nPointsSpewed = 0;

	m_intention = new CRobotIntention( this );
	m_locomotor = new CRobotLocomotion( this );
	m_body = new CHeadlessHatmanBody( this );
	m_bIsPanicked = false;
#else
	ListenForGameEvent( "rd_robot_impact" );
#endif
}

CTFRobotDestruction_Robot::~CTFRobotDestruction_Robot()
{
#ifdef CLIENT_DLL
	m_hDamagedParticleEffect = NULL;
#else
	if ( m_hSpawn )
		m_hSpawn->ClearRobot();
	if ( m_intention )
		delete m_intention;
	if ( m_locomotor )
		delete m_locomotor;
	if ( m_body )
		delete m_body;
#endif
}

void CTFRobotDestruction_Robot::StaticPrecache()
{	
	PrecacheParticleSystem( DEATH_PARTICLE_EFFECT );
	PrecacheParticleSystem( SCORING_POINTS_PARTICLE_EFFECT );
	PrecacheParticleSystem( DAMAGED_ROBOT_PARTICLE_EFFECT );
	PrecacheScriptSound( ROBOT_DEATH_EXPLOSION );

	for( int i=0; i < ARRAYSIZE( g_RobotData ); ++i )
	{
		g_RobotData[i]->Precache();
	}
}

void CTFRobotDestruction_Robot::Precache()
{
	BaseClass::Precache();
	StaticPrecache();
}

void CTFRobotDestruction_Robot::Spawn()
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );
	BuildPropList( "spawn", m_aSpawnProps, GetModelIndex(), 1.f, COLLISION_GROUP_NONE );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );

	m_takedamage = DAMAGE_YES;
	m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
#ifdef GAME_DLL
	SetContextThink( &CTFRobotDestruction_Robot::UpdateAnimsThink, gpGlobals->curtime, ANIMS_THINK );

	if ( m_hGroup )
	{
		m_hGroup->UpdateState();
	}

	m_hNextPath.Set( dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, m_spawnData.m_pszPathName ) ) );
	// The path needs to exist
	if ( !m_hNextPath )
	{
		UTIL_Remove( this );
	}

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotCreated( this );

	// Create our dispenser	
	m_pDispenser = dynamic_cast<CRobotDispenser*>( CreateEntityByName( "rd_robot_dispenser" ) );
	Assert( m_pDispenser );
	m_pDispenser->SetParent( this );
	m_pDispenser->Spawn();
	m_pDispenser->ChangeTeam( GetTeamNumber() );
	m_pDispenser->OnGoActive();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Dont collide with players
//-----------------------------------------------------------------------------
bool CTFRobotDestruction_Robot::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}


#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Specify where our healthbars should go over our heads
//-----------------------------------------------------------------------------
float CTFRobotDestruction_Robot::GetHealthBarHeightOffset() const
{
	return g_RobotData[ m_eType ]->GetFloatData( RobotData_t::HEALTH_BAR_OFFSET_KEY );
}


void CTFRobotDestruction_Robot::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateDamagedEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Play damaged effects, similar to sentries
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::UpdateDamagedEffects()
{
	// Start playing our damaged particle if we're damaged
	bool bLowHealth = GetHealth() <= (GetMaxHealth() * 0.5);
	if ( bLowHealth && !m_hDamagedParticleEffect )
	{
		m_hDamagedParticleEffect = ParticleProp()->Create( DAMAGED_ROBOT_PARTICLE_EFFECT,
														 PATTACH_ABSORIGIN_FOLLOW, 
														 INVALID_PARTICLE_ATTACHMENT, 
														 Vector(0,0,50) );

	}
	else if ( !bLowHealth && m_hDamagedParticleEffect )
	{
		ParticleProp()->StopEmission( m_hDamagedParticleEffect );
		m_hDamagedParticleEffect = NULL;
	}
}

void CTFRobotDestruction_Robot::UpdateClientSideAnimation( void )
{
	m_animController.Update();

	BaseClass::UpdateClientSideAnimation();
}

void CTFRobotDestruction_Robot::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_RD_ROBOT_POP_PANELS_OFF )
	{
		CUtlVector<breakmodel_t> vecProp;
		FOR_EACH_VEC( m_aSpawnProps, i )
		{
			char pstrLowerName[ MAX_PATH ];
			memset( pstrLowerName, 0, sizeof(pstrLowerName) );
			Q_snprintf( pstrLowerName, sizeof(pstrLowerName), "%s", options );
			Q_strlower( pstrLowerName );
			if ( Q_strstr( m_aSpawnProps[i].modelName, pstrLowerName ) )
			{
				vecProp.AddToTail( m_aSpawnProps[i] );
			}
		}

		if ( vecProp.Count() )
		{
			Vector vForward, vRight, vUp;
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );

			Vector vecBreakVelocity = Vector(0,0,200);
			AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
			Vector vecOrigin = GetAbsOrigin() + vForward*70 + vUp*10;
			QAngle vecAngles = GetAbsAngles();
			breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
			breakParams.impactEnergyScale = 1.0f;
			breakParams.defBurstScale = 3.f;
			int nModelIndex = GetModelIndex();

			CreateGibsFromList( vecProp, nModelIndex, NULL, breakParams, this, -1 , false, true );
		}
	}

	BaseClass::FireEvent( origin, angles, event, options );
}

void CTFRobotDestruction_Robot::FireGameEvent( IGameEvent *pEvent )
{
	const char *pszName = pEvent->GetName();
	if ( FStrEq( pszName, "rd_robot_impact" ) )
	{
		const int index_ = pEvent->GetInt( "entindex" );
		if ( index_ == entindex() )
		{
			Vector vecImpulse( pEvent->GetFloat( "impulse_x" ), pEvent->GetFloat( "impulse_y" ), pEvent->GetFloat( "impulse_z" ) );
			m_animController.Impulse( vecImpulse.Normalized() * 20 );
		}
	}
}

 CStudioHdr* CTFRobotDestruction_Robot::OnNewModel()
 {
	 CStudioHdr *hdr = BaseClass::OnNewModel();
	 BuildPropList( "spawn", m_aSpawnProps, GetModelIndex(), 1.f, COLLISION_GROUP_NONE );

	 return hdr;
 }

#endif


#ifdef GAME_DLL
void CTFRobotDestruction_Robot::HandleAnimEvent( animevent_t *pEvent )
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
	/*	if ( pEvent->event == AE_RD_ROBOT_POP_PANELS_OFF )
		{
			CUtlVector<breakmodel_t> vecProp;
			FOR_EACH_VEC( m_aSpawnProps, i )
			{
				char pstrLowerName[ MAX_PATH ];
				memset( pstrLowerName, 0, sizeof(pstrLowerName) );
				Q_snprintf( pstrLowerName, sizeof(pstrLowerName), "%s", pEvent->options );
				Q_strlower( pstrLowerName );
				if ( Q_strstr( m_aSpawnProps[i].modelName, pstrLowerName ) )
				{
					vecProp.AddToTail( m_aSpawnProps[i] );
				}
			}

			if ( vecProp.Count() )
			{
				Vector vForward, vRight, vUp;
				AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );

				Vector vecBreakVelocity = Vector(0,0,200);
				AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
				Vector vecOrigin = GetAbsOrigin() + vForward*70 + vUp*10;
				QAngle vecAngles = GetAbsAngles();
				breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
				breakParams.impactEnergyScale = 1.0f;

				int nModelIndex = modelinfo->GetModelIndex( STRING(GetModelName()) );
				CreateGibsFromList( vecProp, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
			}
		}*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tell the game logic we're gone
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotRemoved( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play our death visual and audio effects
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::PlayDeathEffects()
{
	EmitSound( g_RobotData[ GetRobotSpawnData().m_eType ]->GetStringData( RobotData_t::DEATH_SOUND_KEY ) ); 
	EmitSound( ROBOT_DEATH_EXPLOSION );
	DispatchParticleEffect( DEATH_PARTICLE_EFFECT, GetAbsOrigin(), QAngle( 0,0,0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Handle getting killed
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::Event_Killed( const CTakeDamageInfo &info )
{
	// Let the game logic know that we died
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotRemoved( this );
	}

	PlayDeathEffects();

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, this ) );
	CTFPlayer *pAssister = NULL;

	if ( pScorer )
	{
		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( pHealer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC ) )
		{
			pAssister = pHealer;
		}

		// Work out what killed the player, and send a message to all clients about it
		int iWeaponID;
		const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, NULL, &iWeaponID );
		const char *killer_weapon_log_name = killer_weapon_name;

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pScorer->Weapon_OwnsThisID( iWeaponID ) );
		if ( pWeapon )
		{
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();

			if ( pItem )
			{
				if ( pItem->GetStaticData()->GetIconClassname() )
				{
					killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				}

				if ( pItem->GetStaticData()->GetLogClassname() )
				{
					killer_weapon_log_name = pItem->GetStaticData()->GetLogClassname();
				}
			}
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "rd_robot_killed" );
		if ( event )
		{
			if ( pAssister && ( pAssister != pScorer ) )
			{
				event->SetInt( "assister", pAssister->GetUserID() );
			}
			
			event->SetInt( "attacker", pScorer->GetUserID() );	// attacker
			event->SetString( "weapon", killer_weapon_name );
			event->SetString( "weapon_logclassname", killer_weapon_log_name );
			event->SetInt( "weaponid", iWeaponID );
			event->SetInt( "priority", 6 );		// HLTV event priority, not transmitted

			gameeventmanager->FireEvent( event );
		}
	}

	if ( m_hSpawn )
	{
		m_hSpawn->OnRobotKilled();
	}

	if ( m_hGroup )
	{
		m_hGroup->OnRobotKilled();
	}

	// Kings dont die right away.  Their head cracks open and they spew points out over time, then self-destruct
	if ( m_spawnData.m_eType == ROBOT_TYPE_KING )
	{
		// Change our model to be the damage king model
		SetModel( g_RobotData[ m_spawnData.m_eType ]->GetStringData( RobotData_t::DAMAGED_MODEL_KEY ) );
		ResetSequence( LookupSequence("idle") );

		m_takedamage = DAMAGE_NO;
		SetContextThink( &CTFRobotDestruction_Robot::SpewBarsThink, gpGlobals->curtime, SPEW_BARS_CONTEXT );
		SetContextThink( &CTFRobotDestruction_Robot::SelfDestructThink, gpGlobals->curtime + 5.f, SELF_DESTRUCT_THINK );

		return;
	}

	// Spew our points
	SpewBars( m_spawnData.m_nNumGibs );

	// Spew ammo gibs out
	SpewGibs();

	CBaseAnimating::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Spew out robot gibs that give ammo
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::SpewGibs()
{
	for ( int i=0; i<m_aGibs.Count(); i++ )
	{
		CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin() + m_aGibs[i].offset, GetAbsAngles(), this, m_aGibs[i].modelName );
		Assert( pAmmoPack );
		if ( pAmmoPack )
		{
			pAmmoPack->ActivateWhenAtRest();

			// Calculate the initial impulse on the weapon.
			Vector vecImpulse( random->RandomFloat( -0.5f, 0.5f ), random->RandomFloat( -0.5f, 0.5f ), random->RandomFloat( 0.75f, 1.25f ) );
			VectorNormalize( vecImpulse );
			// Detect the head model
			bool bIsTheHead = FStrEq( "models/bots/bot_worker/bot_worker_head_gib.mdl", m_aGibs[i].modelName );
			if ( bIsTheHead )
			{
				// Pop more up than anything
				vecImpulse[2] = 3.f;
				vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_max.GetFloat() * 0.75, tf_obj_gib_velocity_max.GetFloat()  );
			}
			else
			{
				vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_min.GetFloat(), tf_obj_gib_velocity_max.GetFloat() );
			}


			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			if ( pAmmoPack->VPhysicsGetObject() )
			{
				AngularImpulse angImpulse( 0.f, random->RandomFloat( 0.f, 100.f ), 0.f );
				if ( bIsTheHead )
				{
					// Make the head spin around like a top
					angImpulse = AngularImpulse( RandomFloat(-60.f,60.f), RandomFloat(-60.f,60.f), 100000.f );
				}
				pAmmoPack->VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
			}

			pAmmoPack->SetInitialVelocity( vecImpulse );

			pAmmoPack->m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;

			// Give the ammo pack some health, so that trains can destroy it.
			pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pAmmoPack->m_takedamage = DAMAGE_YES;		
			pAmmoPack->SetHealth( 900 );
			pAmmoPack->m_bObjGib = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do some special effects when we take damage
//-----------------------------------------------------------------------------
int CTFRobotDestruction_Robot::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Check teams
	if ( info.GetAttacker() )
	{
		if ( InSameTeam(info.GetAttacker()) )
			return 0;
						
		CBasePlayer *pAttacker = ToBasePlayer( info.GetAttacker() );
		if ( pAttacker )
		{
			pAttacker->SetLastObjectiveTime( gpGlobals->curtime );
		}
	}

	SetContextThink( &CTFRobotDestruction_Robot::RepairSelfThink, gpGlobals->curtime + 5.f, "RepairSelfThink" );

	if ( m_bShielded )
	{
		return 0;
	}

	CTakeDamageInfo newInfo;
	newInfo = info;
	ModifyDamage( &newInfo );

	// Get our attack vectors
	Vector vecDamagePos = newInfo.GetDamagePosition();
	QAngle vecDamageAngles;
	VectorAngles( -newInfo.GetDamageForce(), vecDamageAngles );
	// Use worldspace center if no damage position (happens with flamethrowers)
	if ( vecDamagePos == vec3_origin )
	{
		vecDamagePos = WorldSpaceCenter();
	}

	// Play a spark effect
	DispatchParticleEffect( "rd_bot_impact_sparks", vecDamagePos, vecDamageAngles );

	// Send an impulse event to the client for this bot
	Vector vecImpulse( newInfo.GetDamageForce() );
	m_animController.Impulse( vecImpulse.Normalized() * 20.f );

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_robot_impact" );
	if ( event )
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "impulse_x", vecImpulse.x );
		event->SetInt( "impulse_y", vecImpulse.y );
		event->SetInt( "impulse_z", vecImpulse.z );

		gameeventmanager->FireEvent( event );
	}

	if( m_hGroup )
	{
		m_hGroup->OnRobotAttacked();
	}

	int nBaseResult = BaseClass::OnTakeDamage( newInfo );

	// Let the game logic know that we got hurt
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotAttacked( this );
	
	return nBaseResult;
}


void CTFRobotDestruction_Robot::ModifyDamage( CTakeDamageInfo *info ) const
{
	CTFPlayer *pAttacker = ToTFPlayer( info->GetAttacker() );
	if ( pAttacker )
	{
		float flScale = 1.f;

		if ( pAttacker->IsPlayerClass( TF_CLASS_SCOUT ) )
			flScale = 1.5f;
		else if( pAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
			flScale = 2.25f;
		else if ( pAttacker->IsPlayerClass( TF_CLASS_SPY ) )
			flScale = 2.f;
		else if ( pAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
			flScale = 0.75;
		else if ( pAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			flScale = 0.75;
		else if ( pAttacker->IsPlayerClass( TF_CLASS_MEDIC ) )
			flScale = 2.f;
			
		info->SetDamage( info->GetDamage() * flScale );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base traceattack to prevent visible effects from team members shooting me
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Prevent team damage here so blood doesn't appear
	if ( inputInfo.GetAttacker() && InSameTeam(inputInfo.GetAttacker()) )
		return;

	AddMultiDamage( inputInfo, this );
}

void CTFRobotDestruction_Robot::UpdateAnimsThink( void )
{
	m_animController.Update();

	SetContextThink( &CTFRobotDestruction_Robot::UpdateAnimsThink, gpGlobals->curtime, ANIMS_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: Change our AI state to use a computer
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::InputStopAndUseComputer( inputdata_t &inputdata )
{
	// TODO: Fire off into the AI
}

//-----------------------------------------------------------------------------
// Purpose: Shoot bars out as we die
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::SpewBars( int nNumToSpew )
{
	for( int i=0; i < nNumToSpew; ++i )
	{
		CBonusPack *pBonusPack = assert_cast< CBonusPack* >( CreateEntityByName( "item_bonuspack" ) );
		if ( pBonusPack )
		{
			pBonusPack->ChangeTeam( GetEnemyTeam( GetTeamNumber() ) );
			pBonusPack->SetDisabled( false );
			pBonusPack->SetAbsOrigin( GetAbsOrigin() + Vector(0,0,20) );
			pBonusPack->SetAbsAngles( QAngle( 0.f, RandomFloat( 0, 360.f ), 0.f ) );
			// Calculate the initial impulse on the cores
			Vector vecImpulse( random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( 1.0, 1.25 ) );
			VectorNormalize( vecImpulse );
			vecImpulse *= random->RandomFloat( 125.f, 150.f );

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			pBonusPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pBonusPack->AddSpawnFlags( SF_NORESPAWN );
			pBonusPack->m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 0 : 1;

			DispatchSpawn( pBonusPack );
			pBonusPack->DropSingleInstance( vecImpulse, NULL, 0, 0 );
			pBonusPack->SetCycle( RandomFloat( 0.f, 1.f ) );
			pBonusPack->SetGravity( 0.2f );
		}
	}
}

void CTFRobotDestruction_Robot::SpewBarsThink()
{
	int nNumToSpew = 1;
	m_nPointsSpewed += nNumToSpew;
	SpewBars( nNumToSpew );

	if ( m_nPointsSpewed >= m_spawnData.m_nNumGibs )
	{
		SelfDestructThink();
	}
	else
	{
		SetContextThink( &CTFRobotDestruction_Robot::SpewBarsThink, gpGlobals->curtime + 0.1f, SPEW_BARS_CONTEXT );
	}
}

void CTFRobotDestruction_Robot::SelfDestructThink()
{
	SpewGibs();
	SpewBars( m_spawnData.m_nNumGibs - m_nPointsSpewed );
	PlayDeathEffects();
	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Purpose: Repair ourselves!
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::RepairSelfThink()
{
	// Heal!
	int nHealth = GetHealth();
	if ( tf_rd_robot_repair_rate.GetFloat() != 0.f )
	{
		nHealth += GetMaxHealth() / tf_rd_robot_repair_rate.GetFloat();
	}
	nHealth = Min( nHealth, GetMaxHealth() );
	SetHealth( nHealth );

	// Continue to heal if we're still hurt
	if ( GetHealth() != GetMaxHealth() )
	{
		SetContextThink( &CTFRobotDestruction_Robot::RepairSelfThink, gpGlobals->curtime + 1.f, "RepairSelfThink" );
	}
}

void CTFRobotDestruction_Robot::ArriveAtPath()
{
	m_hNextPath->AcceptInput( "InPass", this, this, variant_t(), 0 );
	m_hNextPath = m_hNextPath->GetNext();
}

void CTFRobotDestruction_Robot::EnableUber()
{
	m_bShielded = true;
	m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 2 : 3;

	if ( m_hGroup )
	{
		m_hGroup->UpdateState();
	}
}

void CTFRobotDestruction_Robot::DisableUber()
{
	m_bShielded = false;
	m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 0 : 1;

	if ( m_hGroup )
	{
		m_hGroup->UpdateState();
	}
}

void CTFRobotDestruction_Robot::SetNewActivity( Activity activity )
{
	int nSequence = SelectWeightedSequence( activity );
	if ( nSequence )
	{
		SetSequence( nSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();
	}
}

#define CLOSE_ENOUGH_TO_PATH 50.f

//---------------------------------------------------------------------------------------------
class CRobotPatrol : public Action< CTFRobotDestruction_Robot >
{
public:
	void PlayIdleActivity( CTFRobotDestruction_Robot *pMe )
	{
		pMe->SetNewActivity( ACT_BOT_PRIMARY_MOVEMENT );
	}

	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
	{
		PlayIdleActivity( pMe );

		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > OnResume( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *interruptingAction )	
	{
		PlayIdleActivity( pMe );

		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval )
	{
		CPathTrack* pNextPath = pMe->GetNextPath();

		if ( pMe->IsRangeGreaterThan( pNextPath, CLOSE_ENOUGH_TO_PATH ) )
		{
			if ( m_path.GetAge() > 0.5f )
			{
				CRobotPathCost cost( pMe );
				m_path.Compute( pMe, pNextPath->GetAbsOrigin(), cost );
			}

			m_path.Update( pMe );
		}
		else
		{
			pMe->ArriveAtPath();
		}

		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "Patrol"; }		// return name of this action
	PathFollower m_path;
};

class CRobotSpawn : public Action< CTFRobotDestruction_Robot >
{
	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
	{
		pMe->SetNewActivity( ACT_BOT_SPAWN );
		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval )
	{
		if ( pMe->IsActivityFinished() )
		{
			return ChangeTo( new CRobotPatrol, "I've finished my spawn sequence" );
		}
		
		return Continue();
	}

	EventDesiredResult< CTFRobotDestruction_Robot > OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info )
	{
		return TryToSustain( RESULT_CRITICAL, "I'm spawning and being attacked" );
	}

	virtual void OnEnd( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *nextAction )
	{
		pMe->SetBodygroup( pMe->FindBodygroupByName( "head_shell" ), 1 );
		pMe->SetBodygroup( pMe->FindBodygroupByName( "body_shell" ), 1 );
	}

	virtual const char *GetName( void ) const	{ return "Spawn"; }		// return name of this action
};

class CRobotMaterialize : public Action< CTFRobotDestruction_Robot >
{
public:
	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
	{
		// TODO: Play the materialize anim and effects
		/*int nSequence = pMe->SelectWeightedSequence( ACT_BOT_MATERIALIZE );
		pMe->ResetSequence( nSequence );*/
		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval )
	{
		// TODO: Check if materialize activity is finished
		//if ( pMe->IsActivityFinished() )
		{
			return ChangeTo( new CRobotSpawn, "I've fully materialized" );
		}

		// TODO: Control the materialize anim
		
		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "Materialize"; }		// return name of this action
	PathFollower m_path;
};


class CRobotPanic : public Action< CTFRobotDestruction_Robot >
{
public:
	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction );
	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval );
	virtual EventDesiredResult< CTFRobotDestruction_Robot > OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info );
	virtual void OnEnd( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *nextAction );

	virtual const char *GetName( void ) const	{ return "Panic"; }

private:

	CountdownTimer m_SpeakTimer;
	CountdownTimer m_attackedTimer;
	CountdownTimer m_spinTimer;
	bool m_bSpinRight;
};

class CRobotEnterPanic : public Action< CTFRobotDestruction_Robot >
{
	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
	{
		pMe->SetNewActivity( ACT_BOT_PANIC_START );
		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval )
	{
		if ( pMe->IsActivityFinished() )
		{
			return ChangeTo( new CRobotPanic, "I've finished my enter panic sequence" );
		}
		
		return Continue();
	}

	EventDesiredResult< CTFRobotDestruction_Robot > OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info )
	{
		return TryToSustain( RESULT_CRITICAL, "I'm entering panic and being attacked" );
	}

	virtual const char *GetName( void ) const	{ return "Enter Panic"; }		// return name of this action
};

class CRobotLeavePanic : public Action< CTFRobotDestruction_Robot >
{
	virtual ActionResult< CTFRobotDestruction_Robot > OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
	{
		pMe->SetNewActivity( ACT_BOT_PANIC_END );
		return Continue();
	}

	virtual ActionResult< CTFRobotDestruction_Robot > Update( CTFRobotDestruction_Robot *pMe, float interval )
	{
		if ( pMe->IsActivityFinished() )
		{
			return Done( "I've finished my leave panic sequence" );
		}
		
		return Continue();
	}

	EventDesiredResult< CTFRobotDestruction_Robot > OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info )
	{
		return TryToSustain( RESULT_CRITICAL, "I'm leaving panic and being attacked" );
	}

	virtual const char *GetName( void ) const	{ return "Leave Panic"; }		// return name of this action
};

//---------------------------------------------------------------------------------------------
ActionResult< CTFRobotDestruction_Robot > CRobotPanic::OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
{
	m_bSpinRight = RandomInt(0,1) == 1; // Randomly pick which way to spin
	pMe->SetIsPanicked( true );			// Let our bot know he's panicked
	m_attackedTimer.Start( 5.f );		// We panic for 5 seconds
	m_spinTimer.Start( RandomFloat( 0.75f, 1.25f ) );			// Spin for a little bit
	pMe->GetLocomotionInterface()->SetDesiredSpeed( 150.f );	// We go fast when panicked
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pMe, "wheel" ); // Smoke trails on our tire when panicked
	pMe->SetNewActivity( ACT_BOT_PANIC );	// Play panicked activity

	m_SpeakTimer.Start( 3.f );
	const RobotSpawnData_t & data = pMe->GetRobotSpawnData();
	pMe->EmitSound( g_RobotData[ data.m_eType ]->GetStringData( RobotData_t::HURT_SOUND_KEY ) );
	
	return Continue();
}

ActionResult< CTFRobotDestruction_Robot > CRobotPanic::Update( CTFRobotDestruction_Robot *pMe, float interval )
{
	// If we haven't been attacked in awhile, then we're done panicking
	if ( m_attackedTimer.IsElapsed() )
	{
		return ChangeTo( new CRobotLeavePanic, "I'm done panicking" );
	}

	QAngle angles = pMe->GetLocalAngles();

	// If our spin timer is still going, then spin!
	if ( m_spinTimer.GetRemainingTime() < ( m_spinTimer.GetCountdownDuration() * 0.5f ) )
	{
		float flSpinAmt = 2500.f * gpGlobals->frametime;
		flSpinAmt *= m_bSpinRight ? 1.f : -1.f;
		angles.y += flSpinAmt;
		pMe->SetLocalAngles( angles );
	}

	// We just drive forard
	Vector vForward;
	AngleVectors( angles, &vForward );
	Vector vArrivePosition = pMe->GetAbsOrigin() + vForward * 30;

	trace_t tr;
	// See if we hit anything solid a little bit below the robot.  We dont want to jump off cliffs
	UTIL_TraceLine( vArrivePosition, vArrivePosition + Vector(0,0,-30), MASK_PLAYERSOLID, pMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	if ( tr.fraction < 1.0f ) 
	{
		pMe->GetLocomotionInterface()->Approach( vArrivePosition );
	}
		
	// It's time change our spin direction, choose when to spin next, and reapply our smoke particle
	if ( m_spinTimer.IsElapsed() )
	{
		m_bSpinRight = RandomInt(0,1) == 1;
		m_spinTimer.Start( RandomFloat( 0.75f, 1.25f ) );
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pMe, "wheel" );
	}

	return Continue();
}

EventDesiredResult< CTFRobotDestruction_Robot > CRobotPanic::OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info )
{
	if ( m_SpeakTimer.IsElapsed() )
	{
		m_SpeakTimer.Start( RandomFloat( 1.5f, 2.f ) );
		const RobotSpawnData_t & data = pMe->GetRobotSpawnData();
		pMe->EmitSound( g_RobotData[ data.m_eType ]->GetStringData( RobotData_t::HURT_SOUND_KEY ) );
	}

	m_attackedTimer.Start( m_attackedTimer.GetCountdownDuration() );
	return TryToSustain( RESULT_IMPORTANT, "I'm panicking and getting attacked" );
}

void CRobotPanic::OnEnd( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *nextAction )
{ 
	pMe->SetIsPanicked( false );
	pMe->GetLocomotionInterface()->SetDesiredSpeed( 80.f );
}


//---------------------------------------------------------------------------------------------
Action< CTFRobotDestruction_Robot > *CRobotBehavior::InitialContainedAction( CTFRobotDestruction_Robot *pMe )	
{
	return new CRobotMaterialize;
}

ActionResult< CTFRobotDestruction_Robot > CRobotBehavior::OnStart( CTFRobotDestruction_Robot *pMe, Action< CTFRobotDestruction_Robot > *priorAction )
{
	return Continue();
}

ActionResult< CTFRobotDestruction_Robot > CRobotBehavior::Update( CTFRobotDestruction_Robot *pMe, float interval )
{
	//const CKnownEntity *pThreat = pMe->GetVisionInterface()->GetPrimaryKnownThreat();
	//if ( pThreat )
	//{
	//	return SuspendFor( new CRobotAttackEnemy, "I see an enemy!" );
	//}
	{
		// We've been wandering for a bit.  Speak!
		if ( m_IdleSpeakTimer.IsElapsed() && m_SpeakTimer.IsElapsed() )
		{
			m_SpeakTimer.Start( 1.f );
			m_IdleSpeakTimer.Start( RandomFloat( 6.f, 10.f ) );
			const RobotSpawnData_t & data = pMe->GetRobotSpawnData();
			pMe->EmitSound( g_RobotData[ data.m_eType ]->GetStringData( RobotData_t::IDLE_SOUND_KEY ) ); 
		}
	}

	// Do stuff!
	return Continue();
}

EventDesiredResult< CTFRobotDestruction_Robot > CRobotBehavior::OnInjured( CTFRobotDestruction_Robot *pMe, const CTakeDamageInfo &info )
{
	return TrySuspendFor( new CRobotEnterPanic, RESULT_TRY, "I've been attacked" );
}


EventDesiredResult< CTFRobotDestruction_Robot > CRobotBehavior::OnContact( CTFRobotDestruction_Robot *pMe, CBaseEntity *pOther, CGameTrace *result )
{
	if ( m_SpeakTimer.IsElapsed() && ( pOther->IsPlayer() || dynamic_cast< CTFRobotDestruction_Robot * >( pOther ) ) )
	{
		m_SpeakTimer.Start( 3.f );

		{
			const RobotSpawnData_t & data = pMe->GetRobotSpawnData();
			pMe->EmitSound( g_RobotData[ data.m_eType ]->GetStringData( RobotData_t::COLLIDE_SOUND_KEY ) );
		}
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
CRobotIntention::CRobotIntention( CTFRobotDestruction_Robot *pMe ) : IIntention( pMe )
{ 
	m_behavior = new Behavior< CTFRobotDestruction_Robot >( new CRobotBehavior ); 
}

CRobotIntention::~CRobotIntention()
{
	delete m_behavior;
}

void CRobotIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CTFRobotDestruction_Robot >( new CRobotBehavior );
}

void CRobotIntention::Update( void )
{
	m_behavior->Update( static_cast< CTFRobotDestruction_Robot * >( GetBot() ), GetUpdateInterval() ); 
}

// is this a place we can be?
QueryResultType CRobotIntention::IsPositionAllowed( const INextBot *pMeBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
float CRobotLocomotion::GetRunSpeed( void ) const
{
	CTFRobotDestruction_Robot *pRobotMe = static_cast< CTFRobotDestruction_Robot *>( GetBot()->GetEntity() );
	return pRobotMe->GetIsPanicked() ? 150.f : 80.f;
}

float CRobotLocomotion::GetGroundSpeed() const
{
	CTFRobotDestruction_Robot *pRobotMe = static_cast< CTFRobotDestruction_Robot *>( GetBot()->GetEntity() );
	return pRobotMe->GetIsPanicked() ? 150.f : 80.f;
}

//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CRobotLocomotion::GetStepHeight( void ) const
{
	return 24.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CRobotLocomotion::GetMaxJumpHeight( void ) const
{
	return 40.0f;
}


//---------------------------------------------------------------------------------------------
// Return max rate of yaw rotation
float CRobotLocomotion::GetMaxYawRate( void ) const
{
	return 200.0f;
}


//---------------------------------------------------------------------------------------------
bool CRobotLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	return false;
}

#endif

//-----------------------------------------------------------------------------
// Robot Dispenser
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CRobotDispenser )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( RobotDispenser, DT_RobotDispenser )
LINK_ENTITY_TO_CLASS( rd_robot_dispenser, CRobotDispenser );

BEGIN_NETWORK_TABLE( CRobotDispenser, DT_RobotDispenser  )
END_NETWORK_TABLE()

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRobotDispenser::CRobotDispenser()
{
	m_bUseGenerateMetalSound = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRobotDispenser::Spawn( void )
{
	// This cast is for the benefit of GCC
	m_fObjectFlags |= (int)OF_DOESNT_HAVE_A_MODEL;
	m_takedamage = DAMAGE_NO;
	m_iUpgradeLevel = 1;

	TFGameRules()->OnDispenserBuilt( this );
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CRobotDispenser::OnGoActive( void )
{
	BaseClass::OnGoActive();

	if ( m_hTouchTrigger )
	{
		m_hTouchTrigger->SetParent( GetParent() );
	}

	SetModel( "" ); 
}

//-----------------------------------------------------------------------------
// Spawn the vgui control screens on the object
//-----------------------------------------------------------------------------
void CRobotDispenser::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// no panels
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRobotDispenser::SetModel( const char *pModel )
{
	CBaseObject::SetModel( pModel );
}


#endif
