//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bat.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_basedoor.h"
#include "c_tf_player.h"
#include "IEffects.h"
#include "bone_setup.h"
#include "c_tf_gamestats.h"
// Server specific.
#else
#include "doors.h"
#include "tf_player.h"
#include "tf_ammo_pack.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "collisionutils.h"
#include "particle_parse.h"
#include "tf_projectile_base.h"
#include "tf_gamerules.h"
#endif

const float DEFAULT_ORNAMENT_EXPLODE_RADIUS = 50.0f;
const float DEFAULT_ORNAMENT_EXPLODE_DAMAGE_MULT = 0.9f;

//=============================================================================
//
// Weapon Bat tables.
//

// TFBat --
IMPLEMENT_NETWORKCLASS_ALIASED( TFBat, DT_TFWeaponBat )

BEGIN_NETWORK_TABLE( CTFBat, DT_TFWeaponBat )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat, CTFBat );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat );
// -- TFBat


// TFBat_Fish --
IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Fish, DT_TFWeaponBat_Fish )

BEGIN_NETWORK_TABLE( CTFBat_Fish, DT_TFWeaponBat_Fish )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat_Fish )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_fish, CTFBat_Fish );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_fish );
// -- TFBat_Fish


// TFBat_Wood --
IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Wood, DT_TFWeaponBat_Wood )

BEGIN_NETWORK_TABLE( CTFBat_Wood, DT_TFWeaponBat_Wood )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat_Wood )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_wood, CTFBat_Wood );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_wood );
// -- TFBat_Wood


// CTFBat_Giftwrap --
IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Giftwrap, DT_TFWeaponBat_Giftwrap )

BEGIN_NETWORK_TABLE( CTFBat_Giftwrap, DT_TFWeaponBat_Giftwrap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat_Giftwrap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_giftwrap, CTFBat_Giftwrap );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_giftwrap );
// -- CTFBat_Giftwrap


// TFStunBall --
IMPLEMENT_NETWORKCLASS_ALIASED( TFStunBall, DT_TFProjectile_StunBall )
BEGIN_NETWORK_TABLE( CTFStunBall, DT_TFProjectile_StunBall )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_stun_ball, CTFStunBall );
PRECACHE_WEAPON_REGISTER( tf_projectile_stun_ball );

#define TF_WEAPON_STUNBALL_VM_MODEL			"models/weapons/v_models/v_baseball.mdl"
#define TF_WEAPON_STUNBALL_MODEL			"models/weapons/w_models/w_baseball.mdl"

#if defined( GAME_DLL )
ConVar tf_scout_stunball_base_duration( "tf_scout_stunball_base_duration", "6.0", FCVAR_DEVELOPMENTONLY );
ConVar tf_scout_stunball_base_speed( "tf_scout_stunball_base_speed", "3000", FCVAR_DEVELOPMENTONLY );
ConVar sv_proj_stunball_damage( "sv_proj_stunball_damage", "15", FCVAR_DEVELOPMENTONLY );
#endif
// -- TFStunBall


// CTFBall_Ornament --
IMPLEMENT_NETWORKCLASS_ALIASED( TFBall_Ornament, DT_TFProjectileBall_Ornament )
BEGIN_NETWORK_TABLE( CTFBall_Ornament, DT_TFProjectileBall_Ornament )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_ball_ornament, CTFBall_Ornament );
PRECACHE_WEAPON_REGISTER( tf_projectile_ball_ornament );

#define TF_WEAPON_BALL_ORNAMENT_VM_MODEL		"models/weapons/c_models/c_xms_festive_ornament.mdl"
#define TF_WEAPON_BALL_ORNAMENT_MODEL			"models/weapons/c_models/c_xms_festive_ornament.mdl"
// -- CTFBall_Ornament



static string_t s_iszTrainName;

//=============================================================================
#define STUNBALL_TRAIL_ALPHA						128


//=============================================================================
//
// CTFBat
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBat::CTFBat()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat::Smack( void )
{
	BaseClass::Smack();

#ifdef GAME_DLL
	if ( BatDeflects() )
	{
#ifdef TF_RAID_MODE
		if ( TFGameRules()->IsRaidMode() )
		{
		}
		else
#endif // TF_RAID_MODE
		{
			DeflectProjectiles();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat::PlayDeflectionSound( bool bPlayer )
{
	WeaponSound( MELEE_HIT_WORLD );
}

//=============================================================================
//
// CTFBat_Wood
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBat_Wood::CTFBat_Wood()
{
	m_iEnemyBallID = 0;
#ifdef CLIENT_DLL
	m_hStunBallVM = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ConVar tf_scout_bat_launch_delay( "tf_scout_bat_launch_delay", "0.1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::LaunchBallThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	LaunchBall();

#ifdef GAME_DLL
	pPlayer->SpeakWeaponFire( MP_CONCEPT_BAT_BALL );
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::SecondaryAttackAnim( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
}

// SERVER ONLY --
#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Calculate the ball's initial position, angle, and velocity.
//-----------------------------------------------------------------------------
void CTFBat_Wood::GetBallDynamics( Vector& vecLoc, QAngle& vecAngles, Vector& vecVelocity, AngularImpulse& angImpulse, CTFPlayer* pPlayer )
{
	Vector vecForward, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, NULL, &vecUp );
	vecLoc    = pPlayer->GetAbsOrigin() + pPlayer->GetModelScale() * ( Vector( 0, 0, 50 ) + vecForward * 32.f );
	vecAngles = pPlayer->GetAbsAngles();

	// Calculate the initial impulse on the item.
	vecVelocity = Vector( 0.0f, 0.0f, 0.0f );
	vecVelocity += vecForward * 10;
	vecVelocity += vecUp * 1;
	VectorNormalize( vecVelocity );
	vecVelocity *= tf_scout_stunball_base_speed.GetInt();

	angImpulse = AngularImpulse( 0, random->RandomFloat( 0, 100 ), 0 );
}

// -- SERVER ONLY
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Do we have any balls? If so, use them.
	int iBallCount = pPlayer->GetAmmoCount( TF_AMMO_GRENADES1 );
	if ( (iBallCount > 0) && CanCreateBall( pPlayer ) )
	{
		SecondaryAttackAnim( pPlayer );
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );

		SetContextThink( &CTFBat_Wood::LaunchBallThink, gpGlobals->curtime + tf_scout_bat_launch_delay.GetFloat(), "LAUNCH_BALL_THINK" );

		m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;

#ifdef GAME_DLL
		if ( pPlayer->m_Shared.IsStealthed() )
		{
			pPlayer->RemoveInvisibility();
		}
#endif // GAME_DLL

		pPlayer->m_Shared.OnAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client Only. Show the stunball view model if necessary.
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CTFBat_Wood::SetWeaponVisible( bool visible )
{
	BaseClass::SetWeaponVisible( visible );

	if ( !m_hStunBallVM )
		return;

	if ( visible )
	{
		m_hStunBallVM->RemoveEffects( EF_NODRAW );
	}
	else
	{
		m_hStunBallVM->AddEffects( EF_NODRAW );
	}
}
#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	bool bLocalPlayerAmmo = true;
	if ( GetPlayerOwner() == C_BasePlayer::GetLocalPlayer() )
	{
		bLocalPlayerAmmo = GetPlayerOwner()->GetAmmoCount( TF_AMMO_GRENADES1 ) > 0;
	}

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) && bLocalPlayerAmmo == true )
	{
		AddBallChild();
	}
	else 
	{
		RemoveBallChild();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client Only. Show the stunball view model if necessary.
//-----------------------------------------------------------------------------
void CTFBat_Wood::AddBallChild( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( m_hStunBallVM )
		return;

	CTFViewModel* pBall = new class CTFViewModel();
	if ( pBall != NULL )
	{
		pBall->InitializeAsClientEntity( GetBallViewModelName(), RENDER_GROUP_OPAQUE_ENTITY );
		pBall->SetAbsOrigin( pPlayer->GetViewModel()->GetAbsOrigin() );
		pBall->SetModel( GetBallViewModelName() );
		pBall->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

		CStudioHdr *pStudioHdr = pPlayer->GetViewModel()->GetModelPtr();
		if ( pStudioHdr )
		{
			int iAttachment = Studio_FindAttachment( pStudioHdr, "weapon_bone_L" ) + 1;
			pBall->SetParent( pPlayer->GetViewModel(), iAttachment );
		}
		pBall->AddEffects( EF_BONEMERGE );
		pBall->SetMoveType( MOVETYPE_NONE );
		pBall->AddSolidFlags( FSOLID_NOT_SOLID );
		m_hStunBallVM.Set( pBall );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	RemoveBallChild();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::WeaponReset( void )
{
	RemoveBallChild();

	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::UpdateOnRemove( void )
{
	RemoveBallChild();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::RemoveBallChild()
{
	if ( m_hStunBallVM )
	{
		m_hStunBallVM->Remove();
		m_hStunBallVM = NULL;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Determines if there is space to create a ball.
//-----------------------------------------------------------------------------
bool CTFBat_Wood::CanCreateBall( CTFPlayer* pPlayer )
{
	int iWeaponMod = 0;
	CALL_ATTRIB_HOOK_INT( iWeaponMod, set_weapon_mode );
	if ( iWeaponMod == 0 )
		return false;

	if ( pPlayer->GetWaterLevel() == WL_Eyes )
		return false;

	Vector vecForward, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, NULL, &vecUp );
	Vector vecBallStart = pPlayer->GetAbsOrigin() + Vector( 0, 0, 50 );
	Vector vecBallEnd   = vecBallStart + vecForward * 32.f;
	
	// Trace out and see if we hit a wall.
	trace_t trace;
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecBallStart, vecBallEnd, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );
	if ( trace.DidHitWorld() || trace.startsolid )
		return false;
	else
	{
		if ( trace.m_pEnt )
		{
			// Don't let the player bat through doors.
			CBaseDoor *pDoor = dynamic_cast<CBaseDoor*>( trace.m_pEnt );
			if ( pDoor )
				return false;
		}
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::LaunchBall( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

#if GAME_DLL
	// Make a ball.
	CBaseEntity* pBall = CreateBall();
	if ( !pBall )
		return;

	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	WeaponSound( SPECIAL2 );
	pPlayer->RemoveAmmo( 1, TF_AMMO_GRENADES1 );
#endif

	StartEffectBarRegen();
}

// SERVER ONLY --
#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: The wooden bat creates a baseball that stuns whomever it hits.
//-----------------------------------------------------------------------------
CBaseEntity* CTFBat_Wood::CreateBall( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return NULL;

	// Do another check here, as the player may have moved to an invalid position
	// since the first check (0.1 seconds ago).  This fixes the ball sometimes
	// going through thin geometry, such as windows and spawn blockers.
	if ( !CanCreateBall( pPlayer ) )
		return NULL;

	// Determine the ball's initial location, angles, and velocity.
	Vector vecLocation, vecVelocity;
	QAngle vecAngles;
	AngularImpulse angImpulse;
	GetBallDynamics( vecLocation, vecAngles, vecVelocity, angImpulse, pPlayer );

	// Create a stun ball.
	CTFStunBall* pBall = CTFStunBall::Create( vecLocation, vecAngles, pPlayer );
	Assert( pBall );
	if ( !pBall )
		return NULL;

	CalcIsAttackCritical();

	pBall->m_iOriginalOwnerID = m_iEnemyBallID;
	m_iEnemyBallID = 0;

	pBall->SetCritical( IsCurrentAttackACrit() );
	pBall->InitGrenade( vecVelocity, angImpulse, pPlayer, GetTFWpnData() );
	pBall->SetLauncher( this );
	pBall->SetOwnerEntity( pPlayer );
	pBall->SetInitialSpeed( tf_scout_stunball_base_speed.GetInt() );

	return pBall;
}

// -- SERVER ONLY
#endif

//-----------------------------------------------------------------------------
// Purpose: Play pickup anim when we grab a new ball.
//-----------------------------------------------------------------------------
void CTFBat_Wood::PickedUpBall( void )
{
	if ( WeaponState() == WEAPON_IS_ACTIVE )
	{
		SendWeaponAnim( ACT_VM_PULLBACK_SPECIAL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
bool CTFBat_Wood::SendWeaponAnim( int iActivity )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( iActivity );

	if ( pPlayer->GetAmmoCount( TF_AMMO_GRENADES1 ) > 0 )
	{
		switch ( iActivity )
		{
		case ACT_VM_DRAW:
			iActivity = ACT_VM_DRAW_SPECIAL;
			break;
		case ACT_VM_HOLSTER:
			iActivity = ACT_VM_HOLSTER_SPECIAL;
			break;
		case ACT_VM_IDLE:
			iActivity = ACT_VM_IDLE_SPECIAL;
			break;
		case ACT_VM_PULLBACK:
			iActivity = ACT_VM_PULLBACK_SPECIAL;
			break;
		case ACT_VM_PRIMARYATTACK:
			iActivity = ACT_VM_PRIMARYATTACK_SPECIAL;
			break;
		case ACT_VM_SECONDARYATTACK:
			iActivity = ACT_VM_PRIMARYATTACK_SPECIAL;
			break;
		case ACT_VM_HITCENTER:
			iActivity = ACT_VM_HITCENTER_SPECIAL;
			break;
		case ACT_VM_SWINGHARD:
			iActivity = ACT_VM_SWINGHARD_SPECIAL;
			break;
		case ACT_VM_IDLE_TO_LOWERED:
			iActivity = ACT_VM_IDLE_TO_LOWERED_SPECIAL;
			break;
		case ACT_VM_IDLE_LOWERED:
			iActivity = ACT_VM_IDLE_LOWERED_SPECIAL;
			break;
		case ACT_VM_LOWERED_TO_IDLE:
			iActivity = ACT_VM_LOWERED_TO_IDLE_SPECIAL;
			break;
		default:
			break;
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//=============================================================================
//
// CTFStunBall
//

// SERVER ONLY --
#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStunBall::CTFStunBall()
{
	s_iszTrainName = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
	m_iOriginalOwnerID = 0;
	m_pBallTrail = NULL;
	m_flBallTrailLife = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Static entity factory.
//-----------------------------------------------------------------------------
CTFStunBall* CTFStunBall::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFStunBall* pBall = static_cast<CTFStunBall*>( CBaseAnimating::CreateNoSpawn( "tf_projectile_stun_ball", vecOrigin, vecAngles, pOwner ) );
	if ( pBall )
	{
		DispatchSpawn( pBall );
	}

	return pBall;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStunBall::Precache( void )
{
	PrecacheModel( GetBallModelName() );
	PrecacheModel( GetBallViewModelName() );
	PrecacheModel( "effects/baseballtrail_red.vmt" );
	PrecacheModel( "effects/baseballtrail_blu.vmt" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
const char *CTFStunBall::GetBallModelName( void ) const
{
	return TF_WEAPON_STUNBALL_MODEL;
}


//-----------------------------------------------------------------------------
const char *CTFStunBall::GetBallViewModelName( void ) const
{
	return TF_WEAPON_STUNBALL_VM_MODEL;
}


//-----------------------------------------------------------------------------
// Purpose: Sets up initial properties.
//-----------------------------------------------------------------------------
void CTFStunBall::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( GetBallModelName() );
	VPhysicsDestroyObject();
	VPhysicsInitNormal( SOLID_BBOX, 0, false );

	AddSolidFlags( FSOLID_TRIGGER );
	AddFlag( FL_GRENADE );

	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	m_takedamage = DAMAGE_NO;

	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 15, "DieContext" );

	// Draw the trail for the Baseball on spawn
	if ( !m_pBallTrail )
	{
		const char *pTrailTeamName = ( GetTeamNumber() == TF_TEAM_RED ) ? "effects/baseballtrail_red.vmt" : "effects/baseballtrail_blu.vmt";
		CSpriteTrail *pTempTrail = NULL;

		pTempTrail = CSpriteTrail::SpriteTrailCreate( pTrailTeamName, GetAbsOrigin(), true );
		pTempTrail->FollowEntity( this );
		pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, STUNBALL_TRAIL_ALPHA, kRenderFxNone );
		pTempTrail->SetStartWidth( 9 );
		pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
		pTempTrail->SetLifeTime( 0.4 );
		pTempTrail->TurnOn();
		pTempTrail->SetAttachment( this, 0 );
		m_pBallTrail = pTempTrail;
		SetContextThink( &CTFStunBall::RemoveBallTrail, gpGlobals->curtime + 3, "FadeBallTrail");
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStunBall::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( !IsAllowedToExplode() )
		return;

	BaseClass::Explode( pTrace, bitsDamageType );
}

//-----------------------------------------------------------------------------
// Purpose: Stun the person we smashed into.
//-----------------------------------------------------------------------------
#define FLIGHT_TIME_TO_MAX_STUN	0.8f
void CTFStunBall::ApplyBallImpactEffectOnVictim( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsPlayer() )
		return;

	CTFPlayer* pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( m_bTouched )
		return;

	// Can't stun an invul player.
	if ( pPlayer->m_Shared.IsInvulnerable() || pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
		return;

	// We have a more intense stun based on our travel time.
	float flLifeTime = Min( gpGlobals->curtime - m_flCreationTime, FLIGHT_TIME_TO_MAX_STUN );
	float flLifeTimeRatio = flLifeTime / FLIGHT_TIME_TO_MAX_STUN;
	if ( flLifeTimeRatio > 0.1f )
	{
		bool bMax = flLifeTimeRatio >= 1.f;
		int iStunFlags = ( bMax ) ? TF_STUN_SPECIAL_SOUND | TF_STUN_MOVEMENT : TF_STUN_SOUND | TF_STUN_MOVEMENT;
		float flStunAmount = 0.5f;
		float flStunDuration = Max( 2.f, tf_scout_stunball_base_duration.GetFloat() * flLifeTimeRatio );
		if ( bMax )
		{
			flStunDuration += 1.0;
		}

		// MvM bots
		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() && pPlayer->IsBot() )
		{
			// Distance mod
			flStunAmount = ( bMax ) ? 1.f : RemapValClamped( flLifeTimeRatio, 0.1f, 0.99f, 0.5f, 0.75 );

			bool bBoss = TFGameRules() && TFGameRules()->GameModeUsesMiniBosses() && ( pPlayer->IsMiniBoss() || pPlayer->GetModelScale() > 1.0f );
			if ( bMax && !bBoss )
			{
				iStunFlags |= TF_STUN_CONTROLS; 
			}
		}

		CTF_GameStats.Event_PlayerStunBall( pOwner, ( bMax ) ? true : false );

		if ( pPlayer->GetWaterLevel() != WL_Eyes )
		{
			pPlayer->m_Shared.StunPlayer( flStunDuration, flStunAmount, iStunFlags, pOwner );

			if ( pPlayer->GetUserID() == m_iOriginalOwnerID )
			{
				// We just stunned a scout with their own ball.
				// Give the player an achievement for this.
				if ( pOwner->IsPlayerClass( TF_CLASS_SCOUT ) )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_SCOUT_STUN_SCOUT_WITH_THEIR_BALL );
				}
			}
		}
	}

	// Give 'em a love tap.
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

	CBaseEntity *pInflictor = GetLauncher();
	CTakeDamageInfo info;
	info.SetAttacker( GetOwnerEntity() );
	info.SetInflictor( pInflictor ); 
	info.SetWeapon( pInflictor );
	info.SetDamage( ( flLifeTimeRatio >= 1.f ) ? GetDamage() * 1.5f : GetDamage() );
	info.SetDamageCustom( TF_DMG_CUSTOM_BASEBALL );
	info.SetDamageForce( GetDamageForce() );
	info.SetDamagePosition( GetAbsOrigin() );
	int iDamageType = GetDamageType();
	if ( IsCritical() )
		iDamageType |= DMG_CRITICAL;
	info.SetDamageType( iDamageType );

	// Hurt 'em.
	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );
	pPlayer->DispatchTraceAttack( info, dir, pNewTrace );
	ApplyMultiDamage();

	// Make this ball fade faster now that it's hit something.
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 4, "DieContext" );

	m_bTouched = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFStunBall::GetDamage( void )
{
	return sv_proj_stunball_damage.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFStunBall::GetDamageForce( void )
{
	Vector vecVelocity = GetAbsVelocity();
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->GetVelocity( &vecVelocity, NULL );
		VectorNormalize( vecVelocity );
	}

	return (vecVelocity * GetDamage());
}

//-----------------------------------------------------------------------------
// Purpose: We hit something.
//-----------------------------------------------------------------------------
void CTFStunBall::PipebombTouch( CBaseEntity *pOther )
{
	if ( !ShouldBallTouch( pOther ) )
		return;

	CTFPlayer* pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	// Ignore things that aren't players.
	if ( !pOther->IsPlayer() )
		return;

	// If we hit a scout, pickup as ammo
	if ( m_bTouched )
	{
		CTFPlayer* pPlayer = ToTFPlayer( pOther );
		if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_SCOUT ) &&
			(pPlayer->GetAmmoCount( TF_AMMO_GRENADES1 ) < pPlayer->GetMaxAmmo( TF_AMMO_GRENADES1 )) )
		{
			pPlayer->GiveAmmo( 1, TF_AMMO_GRENADES1 );
			RemoveBallTrail();
			UTIL_Remove( this );

			CTFBat_Wood *pBat = (CTFBat_Wood *) pPlayer->Weapon_OwnsThisID( TF_WEAPON_BAT_WOOD );
			if ( pBat )
			{
				// If this ball came from an enemy scout, remember who they were...
				if ( pPlayer->GetTeamNumber() != GetTeamNumber() )
				{
					if ( pOwner )
					{
						pBat->m_iEnemyBallID = pOwner->GetUserID();
					}
				}

				// If we have the bat up, we need to play the correct anim.
				pBat->PickedUpBall();
			}

			// Say something.
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_GRAB_BALL, (pOther->GetTeamNumber() == GetTeamNumber()) ? "my_team:1" : "my_team:0" );
		}
		return;
	}

	if ( pOther == GetThrower() )
		return;

	if ( !InSameTeam( pOther ) && pOther->m_takedamage != DAMAGE_NO )
	{
		ApplyBallImpactEffectOnVictim( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose: We hit something.
//-----------------------------------------------------------------------------
void CTFStunBall::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	CTFPlayer* pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bWasTouched = m_bTouched;
	BaseClass::VPhysicsCollision( index, pEvent );
	if ( pOwner && !bWasTouched && m_bTouched )
	{
		pOwner->SpeakConceptIfAllowed( MP_CONCEPT_BALL_MISSED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fade and kill the trail
//-----------------------------------------------------------------------------
void CTFStunBall::RemoveBallTrail( void )
{
	if (!m_pBallTrail)
		return;

	if (m_pBallTrail)
	{
		if (m_flBallTrailLife <= 0)
		{
			UTIL_Remove( m_pBallTrail);
			m_flBallTrailLife = 1.0f;
		}
		else	
		{
			float fAlpha = STUNBALL_TRAIL_ALPHA * m_flBallTrailLife;

			CSpriteTrail *pTempTrail = dynamic_cast< CSpriteTrail*>( m_pBallTrail.Get() );

			if ( pTempTrail )
			{
				pTempTrail->SetBrightness( int(fAlpha) );
			}

			m_flBallTrailLife = m_flBallTrailLife - 0.1f;
			SetContextThink( &CTFStunBall::RemoveBallTrail, gpGlobals->curtime + 0.05, "FadeBallTrail");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Basic touch screening
//-----------------------------------------------------------------------------
bool CTFStunBall::ShouldBallTouch( CBaseEntity *pOther )
{
	CTFPlayer* pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	Assert( pOther );
	if ( !pOther ||
		 !pOther->IsSolid() ||
		 pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) ||
		 pOther->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
	{
		pOwner->SpeakConceptIfAllowed( MP_CONCEPT_BALL_MISSED );
		return false;
	}

	if ( pOther->IsFuncLOD() || pOther->IsBaseProjectile() )
		return false;

	// Go away if we hit the skybox.
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return false;
	}

	// Pass through ladders
	if ( pTrace->surface.flags & CONTENTS_LADDER )
		return false;

	if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
		return false;

	// Go away if we're hit by a moving train.
	if ( pOther->GetModelName() == s_iszTrainName && ( pOther->GetAbsVelocity().LengthSqr() > 1.0f ) )
	{
		UTIL_Remove( this );
		return false;
	}

	return true;
}

// -- SERVER ONLY
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFStunBall::GetTrailParticleName( void )
{
	int iTeamNumber = GetTeamNumber();

	if ( GetDeflected() )
	{
		CTFPlayer *pOwner =  ToTFPlayer( GetDeflectOwner() );

		if ( pOwner )
		{
			iTeamNumber = pOwner->GetTeamNumber();
		}
	}
	if ( iTeamNumber == TF_TEAM_BLUE )
	{
		return "stunballtrail_blue";
	}
	else
	{
		return "stunballtrail_red";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::CreateTrailParticles( void )
{
	if ( pEffectTrail )
	{
		ParticleProp()->StopEmission( pEffectTrail );
	}
	if ( pEffectCrit )
	{
		ParticleProp()->StopEmission( pEffectCrit );
	}
	pEffectTrail = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );
	int iTeamNumber = GetTeamNumber();

	if ( GetDeflected() )
	{
		CTFPlayer *pOwner =  ToTFPlayer( GetDeflectOwner() );

		if ( pOwner )
		{
			iTeamNumber = pOwner->GetTeamNumber();
		}
	}
	if ( m_bCritical )
	{
		if ( iTeamNumber == TF_TEAM_BLUE )
		{
			pEffectCrit = ParticleProp()->Create( "stunballtrail_blue_crit", PATTACH_ABSORIGIN_FOLLOW );
			
		}
		else
		{
			pEffectCrit = ParticleProp()->Create( "stunballtrail_red_crit", PATTACH_ABSORIGIN_FOLLOW );
		}
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Giftwrap::Spawn( void )
{
	BaseClass::Spawn();

	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFBat_Giftwrap::CreateBall( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return NULL;

	// Do another check here, as the player may have moved to an invalid position
	// since the first check (0.1 seconds ago).  This fixes the ball sometimes
	// going through thin geometry, such as windows and spawn blockers.
	if ( !CanCreateBall( pPlayer ) )
		return NULL;

	// Determine the ball's initial location, angles, and velocity.
	Vector vecLocation, vecVelocity;
	QAngle vecAngles;
	AngularImpulse angImpulse;
	GetBallDynamics( vecLocation, vecAngles, vecVelocity, angImpulse, pPlayer );

	// Create the ornament ball.
	CTFBall_Ornament *pBall = CTFBall_Ornament::Create( vecLocation, vecAngles, pPlayer );
	Assert( pBall );
	if ( !pBall )
		return NULL;

	CalcIsAttackCritical();

	pBall->m_iOriginalOwnerID = m_iEnemyBallID;
	m_iEnemyBallID = 0;

	pBall->SetCritical( IsCurrentAttackACrit() );
	pBall->InitGrenade( vecVelocity, angImpulse, pPlayer, GetTFWpnData() );
	pBall->SetLauncher( this );
	pBall->SetOwnerEntity( pPlayer );
	pBall->SetInitialSpeed( tf_scout_stunball_base_speed.GetInt() );
	pBall->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	return pBall;
}


//-----------------------------------------------------------------------------
void CTFBall_Ornament::Precache( void )
{
	PrecacheScriptSound( "BallBuster.OrnamentImpactRange" );
	PrecacheScriptSound( "BallBuster.OrnamentImpact" );
	PrecacheScriptSound( "BallBuster.HitBall" );
	PrecacheScriptSound( "BallBuster.HitFlesh" );
	PrecacheScriptSound( "BallBuster.HitWorld" );
	PrecacheScriptSound( "BallBuster.DrawCatch" );
	PrecacheScriptSound( "BallBuster.Ornament_DrawCatch" );
	PrecacheScriptSound( "BallBuster.Ball_HitWorld" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
CTFBall_Ornament *CTFBall_Ornament::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFBall_Ornament* pBall = static_cast< CTFBall_Ornament * >( CBaseAnimating::CreateNoSpawn( "tf_projectile_ball_ornament", vecOrigin, vecAngles, pOwner ) );
	if ( pBall )
	{
		DispatchSpawn( pBall );
	}

	return pBall;
}


//-----------------------------------------------------------------------------
const char *CTFBall_Ornament::GetBallModelName( void ) const
{
	return TF_WEAPON_BALL_ORNAMENT_MODEL;
}


//-----------------------------------------------------------------------------
const char *CTFBall_Ornament::GetBallViewModelName( void ) const
{
	return TF_WEAPON_BALL_ORNAMENT_VM_MODEL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBall_Ornament::ApplyBallImpactEffectOnVictim( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsPlayer() )
		return;

	CTFPlayer* pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( m_bTouched )
		return;

	// Can't bleed an invul player.
	if ( pPlayer->m_Shared.IsInvulnerable() || pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
		return;

	bool bIsCriticalHit = IsCritical();
	float flBleedTime = 5.0f;
	bool bIsLongRangeHit = false;

	// long distance hit is always a crit
	float flLifeTime = gpGlobals->curtime - m_flCreationTime;
	if ( flLifeTime >= FLIGHT_TIME_TO_MAX_STUN )
	{
		bIsCriticalHit = true;
		bIsLongRangeHit = true;
	}

	// just do the bleed effect directly since the bleed
	// attribute comes from the inflictor, which is the bat.
	pPlayer->m_Shared.MakeBleed( pOwner, (CTFBat_Giftwrap *)GetLauncher(), flBleedTime );

	// Apply particle effect to victim (the remaining effects happen inside Explode)
	DispatchParticleEffect( "xms_ornament_glitter", PATTACH_POINT_FOLLOW, pPlayer, "head" );

	// Give 'em a love tap.
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

	CBaseEntity *pInflictor = GetLauncher();
	CTakeDamageInfo info;
	info.SetAttacker( GetOwnerEntity() );
	info.SetInflictor( pInflictor ); 
	info.SetWeapon( pInflictor );
	info.SetDamage( GetDamage() );
	info.SetDamageCustom( TF_DMG_CUSTOM_BASEBALL );
	info.SetDamageForce( GetDamageForce() );
	info.SetDamagePosition( GetAbsOrigin() );
	int iDamageType = GetDamageType();
	if ( bIsCriticalHit )
		iDamageType |= DMG_CRITICAL;
	info.SetDamageType( iDamageType );

	// Hurt 'em.
	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );
	pPlayer->DispatchTraceAttack( info, dir, pNewTrace );
	ApplyMultiDamage();

	// the ball shatters
	UTIL_Remove( this );

	m_bTouched = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBall_Ornament::PipebombTouch( CBaseEntity *pOther )
{
	if ( !ShouldBallTouch( pOther ) )
		return;

	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pOther == GetThrower() )
		return;

	// Explode (does radius damage, triggers particles and sound effects).
	Explode( &pTrace, DMG_BLAST|DMG_PREVENT_PHYSICS_FORCE );

	if ( !InSameTeam( pOther ) && pOther->m_takedamage != DAMAGE_NO )
	{
		ApplyBallImpactEffectOnVictim( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBall_Ornament::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;

	// Break if we hit the world.
	if ( pHitEntity->IsWorld() )
	{
		// Explode immediately next frame. (Can't explode in the collision callback.)
		m_vCollisionVelocity = pEvent->preVelocity[index];
		SetContextThink( &CTFBall_Ornament::VPhysicsCollisionThink, gpGlobals->curtime, "OrnamentCollisionThink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBall_Ornament::VPhysicsCollisionThink( void )
{
	trace_t pTrace;
	Vector velDir = m_vCollisionVelocity;
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 16;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 32, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	Explode( &pTrace, DMG_BLAST|DMG_PREVENT_PHYSICS_FORCE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBall_Ornament::Explode( trace_t *pTrace, int bitsDamageType )
{
	// Create smashed glass particles when we explode
	CTFPlayer* pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner && pOwner->GetTeamNumber() == TF_TEAM_RED )
	{
		DispatchParticleEffect( "xms_ornament_smash_red", GetAbsOrigin(), GetAbsAngles() );
	}
	else
	{
		DispatchParticleEffect( "xms_ornament_smash_blue", GetAbsOrigin(), GetAbsAngles() );
	}

	Vector vecOrigin = GetAbsOrigin();

	// sound effects
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pflSoundDuration = 0;
	params.m_pSoundName = "BallBuster.OrnamentImpact";
	CPASFilter filter( vecOrigin );
	filter.RemoveRecipient( pOwner );
	EmitSound( filter, entindex(), params );
	CSingleUserRecipientFilter attackerFilter( pOwner );
	EmitSound( attackerFilter, pOwner->entindex(), params );

	// Explosion damage is some fraction of our base damage
	float flExplodeDamage = GetDamage() * DEFAULT_ORNAMENT_EXPLODE_DAMAGE_MULT;

	// Do radius damage
	Vector vecBlastForce(0.0f, 0.0f, 0.0f);
	CTakeDamageInfo info( this, GetThrower(), m_hLauncher, vecBlastForce, GetAbsOrigin(), flExplodeDamage, bitsDamageType, TF_DMG_CUSTOM_BASEBALL, &vecOrigin );
	CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, DEFAULT_ORNAMENT_EXPLODE_RADIUS, nullptr, 0.0f, 0.0f );
	TFGameRules()->RadiusDamage( radiusinfo );

	UTIL_Remove( this );
}

#endif

