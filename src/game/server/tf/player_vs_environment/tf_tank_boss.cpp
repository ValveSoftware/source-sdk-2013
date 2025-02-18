//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "tf_weaponbase.h"
#include "eventqueue.h"
#include "particle_parse.h"
#include "tf_tank_boss.h"
#include "tf_objective_resource.h"
#include "engine/IEngineSound.h"
#include "logicrelay.h"


#define TANK_DAMAGE_MODEL_COUNT 4
#define TANK_DEFAULT_HEALTH 1000


static const char *s_TankModel[ TANK_DAMAGE_MODEL_COUNT ] =
{
	"models/bots/boss_bot/boss_tank.mdl",
	"models/bots/boss_bot/boss_tank_damage1.mdl",
	"models/bots/boss_bot/boss_tank_damage2.mdl",
	"models/bots/boss_bot/boss_tank_damage3.mdl"
};

static const char *s_TankModelRome[ TANK_DAMAGE_MODEL_COUNT ] =
{
	"models/bots/tw2/boss_bot/boss_tank.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage1.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage2.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage3.mdl"
};

#define TANK_LEFT_TRACK_MODEL	"models/bots/boss_bot/tank_track_L.mdl"
#define TANK_RIGHT_TRACK_MODEL	"models/bots/boss_bot/tank_track_R.mdl"
#define TANK_BOMB				"models/bots/boss_bot/bomb_mechanism.mdl"
#define TANK_DESTRUCTION		"models/bots/boss_bot/boss_tank_part1_destruction.mdl"

#define TANK_LEFT_TRACK_MODEL_ROME	"models/bots/tw2/boss_bot/tank_track_L.mdl"
#define TANK_RIGHT_TRACK_MODEL_ROME	"models/bots/tw2/boss_bot/tank_track_R.mdl"
#define TANK_BOMB_ROME				"models/bots/boss_bot/bomb_mechanism.mdl"
#define TANK_DESTRUCTION_ROME		"models/bots/tw2/boss_bot/boss_tank_part1_destruction.mdl"


#define MVM_DESTROY_TANK_QUICKLY_TIME 25.0f

float CTFTankBoss::m_flLastTankAlert = 0.0f;


class CTFTankDestruction : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFTankDestruction, CBaseAnimating );
	DECLARE_DATADESC();

	CTFTankDestruction( void );

	virtual void Precache( void );
	virtual void Spawn( void );

	void AnimThink( void );

private:

	float m_flVanishTime;

public:

	bool m_bIsAtCapturePoint;
	int m_nDeathAnimPick;
	char m_szDeathPostfix[ 8 ];
};


LINK_ENTITY_TO_CLASS( tank_destruction, CTFTankDestruction );

PRECACHE_REGISTER( tank_destruction );

BEGIN_DATADESC( CTFTankDestruction )
	DEFINE_THINKFUNC( AnimThink ),
END_DATADESC();


CTFTankDestruction::CTFTankDestruction( void )
{
	m_bIsAtCapturePoint = false;
	m_nDeathAnimPick = 0;
	m_szDeathPostfix[ 0 ] = '\0';
}

void CTFTankDestruction::Precache( void )
{
	PrecacheModel( TANK_DESTRUCTION );
	PrecacheModel( TANK_DESTRUCTION_ROME );

	PrecacheParticleSystem( "explosionTrail_seeds_mvm" );
	PrecacheParticleSystem( "fluidSmokeExpl_ring_mvm" );

	PrecacheScriptSound( "MVM.TankExplodes" );

	BaseClass::Precache();
}

void CTFTankDestruction::Spawn( void )
{
	SetModel( TANK_DESTRUCTION );
	SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( TANK_DESTRUCTION ) );
	SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( TANK_DESTRUCTION_ROME ) );

	BaseClass::Spawn();

	int nDestroySequence = -1;

	int nDeathAnimPick = ( m_nDeathAnimPick != 0 ? m_nDeathAnimPick : RandomInt( 1, 3 ) );

	if ( m_bIsAtCapturePoint )
	{
		// Use map specific random
		nDestroySequence = LookupSequence( UTIL_VarArgs( "destroy_%s%i%s", gpGlobals->mapname.ToCStr(), nDeathAnimPick, m_szDeathPostfix ) );
	}

	if ( nDestroySequence == -1 )
	{
		// Fallback to default random
		nDestroySequence = LookupSequence( UTIL_VarArgs( "destroy%i", nDeathAnimPick ) );
	}

	if ( nDestroySequence != -1 )
	{
		SetSequence( nDestroySequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
	}

	DispatchParticleEffect( "explosionTrail_seeds_mvm", GetAbsOrigin(), GetAbsAngles() );
	DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", GetAbsOrigin(), GetAbsAngles() );

	StopSound( "MVM.TankEngineLoop" );

	CBroadcastRecipientFilter filter;
	const Vector originVector = GetAbsOrigin();
	CBaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM.TankExplodes", &originVector );
	CBaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM.TankEnd" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

	m_flVanishTime = gpGlobals->curtime + SequenceDuration();

	SetThink( &CTFTankDestruction::AnimThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( nDestroySequence == -1 )
	{
		SetThink( &CTFTankDestruction::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime );
	}
}

void CTFTankDestruction::AnimThink( void )
{
	if ( gpGlobals->curtime > m_flVanishTime )
	{
		SetThink( &CTFTankDestruction::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime );
		return;
	}

	StudioFrameAdvance();
	DispatchAnimEvents( this );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


LINK_ENTITY_TO_CLASS( tank_boss, CTFTankBoss );

PRECACHE_REGISTER( tank_boss );

IMPLEMENT_SERVERCLASS_ST( CTFTankBoss, DT_TFTankBoss)
	//SendPropVector(SENDINFO(m_StartColor), 8, 0, 0, 1),
END_SEND_TABLE()


BEGIN_DATADESC( CTFTankBoss )

	DEFINE_THINKFUNC( TankBossThink ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "DestroyIfAtCapturePoint", InputDestroyIfAtCapturePoint ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AddCaptureDestroyPostfix", InputAddCaptureDestroyPostfix ),

END_DATADESC()

//-----------------------------------------------------------------------------------------------------
void CMD_TankKill( void )
{
	CBasePlayer *player = UTIL_GetCommandClient();
	if ( !player )
		return;

	CBaseEntity *tank = NULL;
	while( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
	{
		CTakeDamageInfo info( player, player, 9999999.9f, DMG_CRUSH, TF_DMG_CUSTOM_NONE );
		tank->TakeDamage( info );
	}
}
static ConCommand tf_mvm_tank_kill( "tf_mvm_tank_kill", CMD_TankKill, "", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
void CMD_TankHealth( const CCommand& args )
{
	CBasePlayer *player = UTIL_GetCommandClient();
	if ( !player )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: %s <health to set all active tanks to>\n", args[0] );
		return;
	}

	CBaseEntity *tank = NULL;
	while( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
	{
		tank->SetMaxHealth( atoi( args[1] ) );
		tank->SetHealth( atoi( args[1] ) );
	}
}
static ConCommand tf_mvm_tank_health( "tf_mvm_tank_health", CMD_TankHealth, "", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
CTFTankBoss::CTFTankBoss()
{
	m_goalNode = NULL;
	m_body = new CTFTankBossBody( this );
	m_exhaustAttachment = -1;
	m_isSmoking = false;
	m_bIsPlayerKilled = true;
	m_bPlayedHalfwayAlert = false;
	m_bPlayedNearAlert = false;
	m_damageModelIndex = 0;
	m_pWaveSpawnPopulator = NULL;
	m_nDeathAnimPick = 0;
	m_szDeathPostfix[ 0 ] = '\0';
	m_flDroppingStart = 0.0f;
	m_flSpawnTime = 0.0f;
}


//--------------------------------------------------------------------------------------
CTFTankBoss::~CTFTankBoss()
{
	delete m_body;
}


//--------------------------------------------------------------------------------------
void CTFTankBoss::Precache( void )
{
	for( int i=0; i<TANK_DAMAGE_MODEL_COUNT; ++i )
	{
		PrecacheModel( s_TankModel[i] );
		PrecacheModel( s_TankModelRome[i] );
	}

	PrecacheModel( TANK_BOMB );
	PrecacheModel( TANK_LEFT_TRACK_MODEL );
	PrecacheModel( TANK_RIGHT_TRACK_MODEL );

	PrecacheModel( TANK_BOMB_ROME );
	PrecacheModel( TANK_LEFT_TRACK_MODEL_ROME );
	PrecacheModel( TANK_RIGHT_TRACK_MODEL_ROME );

	PrecacheParticleSystem( "smoke_train" );
	PrecacheParticleSystem( "bot_impact_light" );
	PrecacheParticleSystem( "bot_impact_heavy" );

	PrecacheScriptSound( "MVM.TankEngineLoop" );
	PrecacheScriptSound( "MVM.TankPing" );
	PrecacheScriptSound( "MVM.TankDeploy" );
	PrecacheScriptSound( "MVM.TankStart" );
	PrecacheScriptSound( "MVM.TankEnd" );
	PrecacheScriptSound( "MVM.TankSmash" );

	BaseClass::Precache();
}

//--------------------------------------------------------------------------------------
int CTFTankBoss::GetCurrencyValue( void )
{
	if ( m_goalNode == NULL && !m_bIsPlayerKilled )
	{
		return 0;
	}

	if ( m_pWaveSpawnPopulator )
	{
		return m_pWaveSpawnPopulator->GetCurrencyAmountPerDeath();
	}

	return BaseClass::GetCurrencyValue();
}

void CTFTankBoss::InputDestroyIfAtCapturePoint( inputdata_t &inputdata )
{
	m_nDeathAnimPick = inputdata.value.Int();

	if ( m_goalNode == NULL )
	{
		TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 9999999.9f, DMG_CRUSH, TF_DMG_CUSTOM_NONE ) );
	}
}

void CTFTankBoss::InputAddCaptureDestroyPostfix( inputdata_t &inputdata )
{
	V_strncpy( m_szDeathPostfix, inputdata.value.String(), ARRAYSIZE( m_szDeathPostfix ) );
}


//--------------------------------------------------------------------------------------
void CTFTankBoss::Spawn( void )
{
	if ( ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() ) && GetInitialHealth() == 0 )
	{
		if ( GetHealth() > 0 )
			SetInitialHealth( GetHealth() );
		else
			SetInitialHealth( TANK_DEFAULT_HEALTH );
	}

	BaseClass::Spawn();
	m_vCollisionMins.Init();
	m_vCollisionMaxs.Init();

	ChangeTeam( TF_TEAM_PVE_INVADERS );

	m_damageModelIndex = 0;
	SetModel( s_TankModel[ m_damageModelIndex ] );
	SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( s_TankModel[ m_damageModelIndex ] ) );
	SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( s_TankModelRome[ m_damageModelIndex ] ) );
	m_lastHealth = GetMaxHealth();

	AddGlowEffect();

	m_leftTracks = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_leftTracks )
	{
		m_leftTracks->SetModel( TANK_LEFT_TRACK_MODEL );
		m_leftTracks->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( TANK_LEFT_TRACK_MODEL ) );
		m_leftTracks->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( TANK_LEFT_TRACK_MODEL_ROME ) );

		// bonemerge into our model
		m_leftTracks->FollowEntity( this, true );

		int animSequence = m_leftTracks->LookupSequence( "forward" );
		if ( animSequence )
		{
			m_leftTracks->SetSequence( animSequence );
			m_leftTracks->SetPlaybackRate( 1.0f );
			m_leftTracks->SetCycle( 0 );
			m_leftTracks->ResetSequenceInfo();
		}

		m_lastLeftTrackPos = m_leftTracks->GetAbsOrigin();
	}		

	m_rightTracks = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_rightTracks )
	{
		m_rightTracks->SetModel( TANK_RIGHT_TRACK_MODEL );
		m_rightTracks->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( TANK_RIGHT_TRACK_MODEL ) );
		m_rightTracks->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( TANK_RIGHT_TRACK_MODEL_ROME ) );

		// bonemerge into our model
		m_rightTracks->FollowEntity( this, true );

		int animSequence = m_rightTracks->LookupSequence( "forward" );
		if ( animSequence )
		{
			m_rightTracks->SetSequence( animSequence );
			m_rightTracks->SetPlaybackRate( 1.0f );
			m_rightTracks->SetCycle( 0 );
			m_rightTracks->ResetSequenceInfo();
		}

		m_lastRightTrackPos = m_rightTracks->GetAbsOrigin();
	}		

	m_bomb = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_bomb )
	{
		m_bomb->SetModel( TANK_BOMB );
		m_bomb->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( TANK_BOMB ) );
		m_bomb->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( TANK_BOMB_ROME ) );

		// bonemerge into our model
		m_bomb->FollowEntity( this, true );
	}		

	GetBodyInterface()->StartSequence( "movement" );

	m_exhaustAttachment = LookupAttachment( "smoke_attachment" );

	if ( m_goalNode == NULL )
	{
		m_goalNode = dynamic_cast< CPathTrack * >( gEntList.FindEntityByClassname( NULL, "path_track" ) );

		if ( m_goalNode )
		{
			// find first node
			while( m_goalNode->GetPrevious() )
			{
				m_goalNode = m_goalNode->GetPrevious();
			}

			SetAbsOrigin( m_goalNode->WorldSpaceCenter() );
		}
	}
	else
	{
		SetAbsOrigin( m_goalNode->WorldSpaceCenter() );
	}

	// We've traveled nowhere if we're at the first node
	m_fTotalDistance = 0.0f;
	m_CumulativeDistances.AddToTail( m_fTotalDistance );

	// Remember starting node
	m_startNode = m_goalNode;
	m_endNode = m_startNode;
	m_nNodeNumber = 0;

	// Orient the Tank along the path
	if ( m_goalNode != NULL )
	{
		CPathTrack *pPrevNode = m_goalNode;
		CPathTrack *pNextNode = m_goalNode->GetNext();

		if ( pNextNode )
		{
			Vector along = pNextNode->GetAbsOrigin() - m_goalNode->GetAbsOrigin();

			QAngle angles;
			VectorAngles( along, angles );

			SetAbsAngles( angles );

			// Find last node and calculate cumulative distance
			while( pNextNode )
			{
				along = pNextNode->GetAbsOrigin() - pPrevNode->GetAbsOrigin();
				along.z = 0.0f;

				m_fTotalDistance += along.Length();
				m_CumulativeDistances.AddToTail( m_fTotalDistance );

				pPrevNode = pNextNode;
				pNextNode = pNextNode->GetNext();
			}
		}
	}

	SetBloodColor( DONT_BLEED );

	m_flLastPingTime = gpGlobals->curtime;

	CBroadcastRecipientFilter filter;
	EmitSound( filter, entindex(), "MVM.TankEngineLoop" );
	EmitSound( "MVM.TankStart" );

	if ( TFGameRules() )
	{
		int nTankCount = 0;

		CBaseEntity *tank = NULL;
		while( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
		{
			nTankCount++;
		}

		if ( nTankCount <= 1 )
		{
			if ( m_flLastTankAlert + 5.0f < gpGlobals->curtime )
			{
				CWave *pWave = g_pPopulationManager ? g_pPopulationManager->GetCurrentWave() : NULL;
				if ( pWave && pWave->NumTanksSpawned() > 1 )
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Another" );
				}
				else
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Spawn" );
				}

				m_flLastTankAlert = gpGlobals->curtime;
			}
		}
		else
		{
			// Don't worry about when the last alert was in this case because 2 tanks can spawn at once
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Multiple" );
			m_flLastTankAlert = gpGlobals->curtime;
		}

		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_CALLOUT, TF_TEAM_PVE_DEFENDERS );
	}

	m_isDroppingBomb = false;
	m_flDroppingStart = 0.0f;
	m_flSpawnTime = gpGlobals->curtime;

	SetThink( &CTFTankBoss::TankBossThink );
	SetNextThink( gpGlobals->curtime );
}


//--------------------------------------------------------------------------------------
void CTFTankBoss::UpdateOnRemove( void )
{
	StopSound( "MVM.TankEngineLoop" );

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->DecrementMannVsMachineWaveClassCount( MAKE_STRING( "tank" ), MVM_CLASS_FLAG_NORMAL | MVM_CLASS_FLAG_MINIBOSS );
	}

	BaseClass::UpdateOnRemove();
}


int CTFTankBoss::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	if ( static_cast< float >( GetHealth() ) / GetMaxHealth() > 0.3f )
	{
		DispatchParticleEffect( "bot_impact_light", rawInfo.GetDamagePosition(), vec3_angle );
	}
	else
	{
		DispatchParticleEffect( "bot_impact_heavy", rawInfo.GetDamagePosition(), vec3_angle );
	}
	
	// Calculate Final Damage values
	if ( BaseClass::OnTakeDamage_Alive( rawInfo ) && rawInfo.GetAttacker() )
	{
		// track who damaged us
		CTFPlayer *pTFPlayer = dynamic_cast< CTFPlayer* >( rawInfo.GetAttacker() );
		if ( pTFPlayer )
		{
			// is the attacker being healed by any Medic(s)?
			CUtlVector<CTFPlayer*> pTempPlayerQueue;
			pTFPlayer->AddConnectedPlayers( pTempPlayerQueue, pTFPlayer );

			for ( int i = 0 ; i < pTempPlayerQueue.Count() ; i++ )
			{
				EntityHistory_t newHist;
				newHist.hEntity = pTempPlayerQueue[i];
				newHist.flTimeDamage = gpGlobals->curtime;
				m_vecDamagers.InsertHistory( newHist );
			}

			// Report Tank dmg to Stats
			CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats )
			{
				pStats->PlayerEvent_DealtDamageToTanks( pTFPlayer, rawInfo.GetDamage() );
			}
		}

		return 1;
	}

	return 0;
}


//--------------------------------------------------------------------------------------
void CTFTankBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_bIsPlayerKilled = ( info.GetDamageType() & DMG_CRUSH ) == 0;

	Explode();

	// check for MvM achievement
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( FStrEq( "mvm_rottenburg", STRING( gpGlobals->mapname ) ) )
		{
			CLogicRelay *pLogicRelay = dynamic_cast< CLogicRelay* >( gEntList.FindEntityByName( NULL, "Barricade_Achievement_Check" ) );
			if ( pLogicRelay && !pLogicRelay->IsDisabled() )
			{
				CUtlVector<CTFPlayer *> playerVector;
				CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
				FOR_EACH_VEC( playerVector, i )
				{
					if ( !playerVector[i] )
						continue;

					if ( playerVector[i]->IsBot() )
						continue;

					playerVector[i]->AwardAchievement( ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_TANK );
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}


//--------------------------------------------------------------------------------------
void CTFTankBoss::SetStartingPathTrackNode( char *name )
{
	m_goalNode = dynamic_cast< CPathTrack * >( gEntList.FindEntityByName( NULL, name ) );
}


//-----------------------------------------------------------------------------------------------------
void CTFTankBoss::TankBossThink( void )
{
	// damage states
	if ( GetHealth() != m_lastHealth )
	{
		// health changed - potentially change damage model
		m_lastHealth = GetHealth();

		int healthPerModel = GetMaxHealth() / TANK_DAMAGE_MODEL_COUNT;
		int healthThreshold = GetMaxHealth() - healthPerModel;

		int desiredModelIndex;
		for( desiredModelIndex = 0; desiredModelIndex < TANK_DAMAGE_MODEL_COUNT; ++desiredModelIndex )
		{
			if ( GetHealth() > healthThreshold )
			{
				break;
			}

			healthThreshold -= healthPerModel;
		}

		if ( desiredModelIndex >= TANK_DAMAGE_MODEL_COUNT )
		{
			desiredModelIndex = TANK_DAMAGE_MODEL_COUNT-1;
		}

		if ( desiredModelIndex != m_damageModelIndex )
		{
			// update model
			const char *pchSequence = GetSequenceName( GetSequence() );
			float fCycle = GetCycle();

			m_damageModelIndex = desiredModelIndex;
			SetModel( s_TankModel[ m_damageModelIndex ] );
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( s_TankModel[ m_damageModelIndex ] ) );
			SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( s_TankModelRome[ m_damageModelIndex ] ) );

			int nAnimSequence = LookupSequence( pchSequence );
			if ( nAnimSequence > 0 )
			{
				SetSequence( nAnimSequence );
				SetPlaybackRate( 1.0f );
				ResetSequenceInfo();
				SetCycle( fCycle );
			}
			else
			{
				GetBodyInterface()->StartSequence( "movement" );
			}
		}
	}

	// left/right track speed
	const float trackMaxSpeed = 80.0f;
	const float trackOffset = 56.221f;

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	if ( m_leftTracks )
	{
		Vector trackCenter = GetAbsOrigin() - trackOffset * right;

		float speed = ( trackCenter - m_lastLeftTrackPos ).Length() / gpGlobals->frametime;

		if ( speed >= trackMaxSpeed )
		{
			m_leftTracks->SetPlaybackRate( 1.0f );
		}
		else
		{
			m_leftTracks->SetPlaybackRate( speed / trackMaxSpeed );
		}

		m_lastLeftTrackPos = trackCenter;
	}

	if ( m_rightTracks )
	{
		Vector trackCenter = GetAbsOrigin() + trackOffset * right;

		float speed = ( trackCenter - m_lastRightTrackPos ).Length() / gpGlobals->frametime;

		if ( speed >= trackMaxSpeed )
		{
			m_rightTracks->SetPlaybackRate( 1.0f );
		}
		else
		{
			m_rightTracks->SetPlaybackRate( speed / trackMaxSpeed );
		}

		m_lastRightTrackPos = trackCenter;
	}


	if ( m_goalNode != NULL )
	{
		Vector toGoal = m_goalNode->WorldSpaceCenter() - GetAbsOrigin();
		toGoal.z = 0.0f;
		float range = toGoal.NormalizeInPlace();

		if ( GetParent() )
		{
			// Track train might be closer
			toGoal = m_goalNode->WorldSpaceCenter() - GetParent()->GetAbsOrigin();
			toGoal.z = 0.0f;
			float flTempRange = toGoal.NormalizeInPlace();
			range = MIN( range, flTempRange );
		}

		if ( TFGameRules() )
		{
			if ( m_nNodeNumber <= 0 )
			{
				//TFGameRules()->SetBossNormalizedTravelDistance( 0.0f );
			}
			else
			{
				Assert( m_CumulativeDistances.IsValidIndex( m_nNodeNumber ) );
				float fBaseDistance = m_CumulativeDistances[ m_nNodeNumber - 1 ];
				float fDistanceFromPreviousToNext = m_CumulativeDistances[ m_nNodeNumber ] - fBaseDistance;
				float fDistanceTraveled = fBaseDistance + ( fDistanceFromPreviousToNext - range );
				float fDistancePercent = fDistanceTraveled / m_fTotalDistance;
				//TFGameRules()->SetBossNormalizedTravelDistance( fDistancePercent );

				if ( m_flLastTankAlert + 5.0f < gpGlobals->curtime )
				{
					if ( !m_bPlayedNearAlert && fDistancePercent > 0.75f)
					{
						TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Near_Hatch", 5.0f );
						m_flLastTankAlert = gpGlobals->curtime;
						m_bPlayedNearAlert = true;
					}
					else if ( !m_bPlayedHalfwayAlert && fDistancePercent > 0.5f)
					{
						int nTankCount = 0;

						CBaseEntity *tank = NULL;
						while( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
						{
							nTankCount++;
						}

						if ( nTankCount > 1 )
						{
							TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Halfway_Multiple", 5.0f );
						}
						else
						{
							TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Halfway", 5.0f );
						}

						m_flLastTankAlert = gpGlobals->curtime;
						m_bPlayedHalfwayAlert = true;
					}
				}
			}
		}

		if ( range < 20.0f )
		{
			// reached node
			inputdata_t dummyData;
			dummyData.pActivator = this;
			dummyData.pCaller = this;
			dummyData.nOutputID = 0;

			m_goalNode->InputPass( dummyData );

			m_goalNode = m_goalNode->GetNext();
			m_nNodeNumber++;

			if ( m_goalNode == NULL && m_bomb )
			{
				//DevMsg( "Tank's final position: %.2f %.2f %.2f", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );

				/*if ( TFGameRules() )
				{
					TFGameRules()->SetBossNormalizedTravelDistance( 1.0f );
				}*/

				// reached end of track - deploy the bomb
				int animSequence = m_bomb->LookupSequence( "deploy" );
				if ( animSequence )
				{
					m_bomb->SetSequence( animSequence );
					m_bomb->SetPlaybackRate( 1.0f );
					m_bomb->SetCycle( 0 );
					m_bomb->ResetSequenceInfo();
				}

				animSequence = LookupSequence( "deploy" );
				if ( animSequence )
				{
					SetSequence( animSequence );
					SetPlaybackRate( 1.0f );
					SetCycle( 0 );
					ResetSequenceInfo();
				}

				if ( m_flLastTankAlert + 5.0f < gpGlobals->curtime )
				{
					TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Deploying", 5.0f );
					m_flLastTankAlert = gpGlobals->curtime;
					m_bPlayedNearAlert = true;
				}

				m_isDroppingBomb = true;
				m_flDroppingStart = gpGlobals->curtime;

				StopSound( "MVM.TankEngineLoop" );

				EmitSound( "MVM.TankDeploy" );

				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_DEPLOYING, TF_TEAM_PVE_DEFENDERS );
			}
		}

		if ( m_goalNode )
		{
			Vector goal = m_goalNode->WorldSpaceCenter();

			GetLocomotionInterface()->SetDesiredSpeed( GetMaxSpeed() );
			GetLocomotionInterface()->Approach( goal );
			GetLocomotionInterface()->FaceTowards( goal );

			if ( m_rumbleTimer.IsElapsed() )
			{
				m_rumbleTimer.Start( 0.25f );

				// shake nearby players' screens.
				UTIL_ScreenShake( GetAbsOrigin(), 2.0f, 5.0f, 1.0f, 500.0f, SHAKE_START );
			}
		}
	}

	if ( m_isDroppingBomb && IsSequenceFinished() )
	{
		FirePopFileEvent( &m_onBombDroppedEventInfo );
		m_isDroppingBomb = false;

		TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Planted" );
	}

	// if the Tank is driving under something, shut off its smokestack
	if ( m_exhaustAttachment > 0 )
	{
		Vector smokePos;
		GetAttachment( m_exhaustAttachment, smokePos );

		trace_t result;
		UTIL_TraceLine( smokePos, smokePos + Vector( 0, 0, 300.0f ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &result );

		if ( result.DidHit() )
		{
			if ( m_isSmoking )
			{
				StopParticleEffects( this );
				m_isSmoking = false;
			}
		}
		else if ( !m_isSmoking )
		{
			DispatchParticleEffect( "smoke_train", PATTACH_POINT_FOLLOW, this, m_exhaustAttachment );
			m_isSmoking = true;
		}
	}

	// destroy things we drive into/over
	if ( m_crushTimer.IsElapsed() )
	{
		m_crushTimer.Start( 0.5f );

		const int maxCollectedEntities = 64;
		CBaseEntity	*intersectingEntities[ maxCollectedEntities ];
		int count = UTIL_EntitiesInBox( intersectingEntities, maxCollectedEntities,  
										GetAbsOrigin() + WorldAlignMins() * 0.75f,	// a little fudge room for players on the top or sides
										GetAbsOrigin() + WorldAlignMaxs() * 0.75f,
										FL_CLIENT | FL_OBJECT );

		for( int i = 0; i < count; ++i )
		{
			CBaseEntity *victim = intersectingEntities[i];

			if ( victim == NULL )
				continue;

			int damage = MAX( victim->GetMaxHealth(), victim->GetHealth() );

			CTakeDamageInfo info( this, this, 4 * damage, DMG_CRUSH, TF_DMG_CUSTOM_NONE );
			victim->TakeDamage( info );
		}
	}

	UpdatePingSound();

	BaseClass::BossThink();
}


//-----------------------------------------------------------------------------------------------------
void CTFTankBoss::ModifyDamage( CTakeDamageInfo *info ) const
{
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info->GetWeapon() );

	if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
	{
		// miniguns are crazy powerful when all bullets always hit
		const float minigunFactor = 0.25f;
		info->SetDamage( info->GetDamage() * minigunFactor );
	}
}

void CTFTankBoss::UpdateCollisionBounds( void )
{
	// Remember the initial bounds
	if ( m_vCollisionMins.IsZero() || m_vCollisionMaxs.IsZero() )
	{
		m_vCollisionMins = WorldAlignMins();
		m_vCollisionMaxs = WorldAlignMaxs();
	}

	// When the tank is at a diagonal angle we don't want the bounds to bloat too far
	float flDiagonalShrinkMultiplier = 1.0f - fabsf( sinf( DEG2RAD( GetAbsAngles().y ) * 2.0f ) ) * 0.4f;

	Vector vMins = m_vCollisionMins;
	vMins.x *= flDiagonalShrinkMultiplier;
	vMins.y *= flDiagonalShrinkMultiplier;

	Vector vMaxs = m_vCollisionMaxs;
	vMaxs.x *= flDiagonalShrinkMultiplier;
	vMaxs.y *= flDiagonalShrinkMultiplier;

	// Build new world aligned bounds based on how it's rotated
	VMatrix rot;
	MatrixFromAngles( GetAbsAngles(), rot );

	Vector vMinsOut, vMaxsOut;
	TransformAABB( rot.As3x4(), vMins, vMaxs, vMinsOut, vMaxsOut );
	CollisionProp()->SetCollisionBounds( vMinsOut, vMaxsOut );
}


//-----------------------------------------------------------------------------------------------------
void CTFTankBoss::FirePopFileEvent( EventInfo *eventInfo )
{
	if ( eventInfo && eventInfo->m_action.Length() > 0 )
	{
		CBaseEntity *targetEntity = gEntList.FindEntityByName( NULL, eventInfo->m_target );
		if ( !targetEntity )
		{
			Warning( "CTFTankBoss: Can't find target entity '%s' for event\n", eventInfo->m_target.Access() );
		}
		else
		{
			g_EventQueue.AddEvent( targetEntity, eventInfo->m_action, 0.0f, NULL, NULL );
		}
	}
}

void CTFTankBoss::Explode( void )
{
	StopSound( "MVM.TankEngineLoop" );

	FirePopFileEvent( &m_onKilledEventInfo );

	CTFTankDestruction *pDestruction = dynamic_cast< CTFTankDestruction* >( CreateEntityByName( "tank_destruction" ) );
	if ( pDestruction )
	{
		// Only do special capture point death if it was force killed by bomb drop
		pDestruction->m_bIsAtCapturePoint = ( m_goalNode == NULL && !m_bIsPlayerKilled );
		pDestruction->m_nDeathAnimPick = m_nDeathAnimPick;
		V_strncpy( pDestruction->m_szDeathPostfix, m_szDeathPostfix, ARRAYSIZE( pDestruction->m_szDeathPostfix ) );

		pDestruction->SetAbsOrigin( GetAbsOrigin() );
		pDestruction->SetAbsAngles( GetAbsAngles() );
		DispatchSpawn( pDestruction );
	}

	if ( m_bIsPlayerKilled )
	{
		TFGameRules()->BroadcastSound( 255, "Announcer.MVM_General_Destruction" );
		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_DEAD, TF_TEAM_PVE_DEFENDERS );

		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_tank_destroyed_by_players" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			// ACHIEVEMENT_TF_MVM_DESTROY_TANK_WHILE_DEPLOYING
			if ( m_isDroppingBomb )
			{
				// short delay so you only get the achievement if the bomb doors have opened/closed and it's ready to deploy
				if ( gpGlobals->curtime - m_flDroppingStart > 5.8f )
				{
					// anyone who has damaged the tank since the deploy anim began will get the achievement
					float flWindow = gpGlobals->curtime - m_flDroppingStart;

					for ( int i = 0; i < m_vecDamagers.Count(); i++ )
					{
						// get the achievement if you have damaged the tank since the deploy anim began
						if ( ( gpGlobals->curtime - m_vecDamagers[i].flTimeDamage ) < flWindow )
						{
							CTFPlayer *pTFPlayer = dynamic_cast< CTFPlayer* >( m_vecDamagers[i].hEntity.Get() );
							if ( pTFPlayer )
							{
								pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_MVM_DESTROY_TANK_WHILE_DEPLOYING );
							}
						}
					}
				}
			}

			// ACHIEVEMENT_TF_MVM_DESTROY_TANK_QUICKLY
			if ( ( gpGlobals->curtime - m_flSpawnTime ) < MVM_DESTROY_TANK_QUICKLY_TIME )
			{
				for ( int i = 0; i < m_vecDamagers.Count(); i++ )
				{
					// get the achievement if you have damaged the tank since the deploy anim began
					if ( ( gpGlobals->curtime - m_vecDamagers[i].flTimeDamage ) < MVM_DESTROY_TANK_QUICKLY_TIME )
					{
						CTFPlayer *pTFPlayer = dynamic_cast< CTFPlayer* >( m_vecDamagers[i].hEntity.Get() );
						if ( pTFPlayer )
						{
							pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_MVM_DESTROY_TANK_QUICKLY );
						}
					}
				}
			}

			// strange part credit? (this logic isn't so correct -- it'll try to grant the credit to the active
			// weapon of anyone who damaged the tank, *not* the weapon that actually did the damage, as we don't
			// track that)
			FOR_EACH_VEC( m_vecDamagers, i )
			{
				CTFPlayer *pTFPlayer = dynamic_cast< CTFPlayer* >( m_vecDamagers[i].hEntity.Get() );
				if ( !pTFPlayer )
					continue;

				EconEntity_OnOwnerKillEaterEventNoPartner( pTFPlayer->GetActiveTFWeapon(), pTFPlayer, kKillEaterEvent_TanksDestroyed );
			}
		}
	}
}
#define TANK_PING_TIME 5.0
void CTFTankBoss::UpdatePingSound( void )
{
	if( gpGlobals->curtime - m_flLastPingTime >= TANK_PING_TIME )
	{
		m_flLastPingTime = gpGlobals->curtime;
		EmitSound( "MVM.TankPing");
	}
}

