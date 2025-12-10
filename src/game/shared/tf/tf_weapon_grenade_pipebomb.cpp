//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_grenadelauncher.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "IEffects.h"
#include "materialsystem/imaterialvar.h"
#include "functionproxy.h"
// Server specific.
#else
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "IEffects.h"
#include "props.h"
#include "func_respawnroom.h"
#include "tf_ammo_pack.h"
#include "takedamageinfo.h"
#include "tf_team.h"
#include "physics_collisionevent.h"
#ifdef TF_RAID_MODE
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#endif // TF_RAID_MODE
#include "tf_weapon_medigun.h"
#endif

#define TF_WEAPON_PIPEBOMB_TIMER		3.0f //Seconds

#define TF_WEAPON_PIPEBOMB_GRAVITY		0.5f
#define TF_WEAPON_PIPEBOMB_FRICTION		0.8f
#define TF_WEAPON_PIPEBOMB_ELASTICITY	0.45f

#define TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION		0.6

extern ConVar tf_grenadelauncher_max_chargetime;
ConVar tf_grenadelauncher_chargescale( "tf_grenadelauncher_chargescale", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenadelauncher_livetime( "tf_grenadelauncher_livetime", "0.8", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
extern ConVar tf_sticky_radius_ramp_time;
extern ConVar tf_sticky_airdet_radius;

#ifndef CLIENT_DLL

ConVar tf_grenadelauncher_min_contact_speed( "tf_grenadelauncher_min_contact_speed", "100", FCVAR_DEVELOPMENTONLY );
extern ConVar tf_obj_gib_velocity_min;
extern ConVar tf_obj_gib_velocity_max;
extern ConVar tf_obj_gib_maxspeed;
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )

BEGIN_NETWORK_TABLE( CTFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )
#ifdef CLIENT_DLL
RecvPropInt( RECVINFO( m_bTouched ) ),
RecvPropInt( RECVINFO( m_iType ) ),
RecvPropEHandle( RECVINFO( m_hLauncher ) ),
RecvPropBool( RECVINFO( m_bDefensiveBomb ) ),
#else
SendPropBool( SENDINFO( m_bTouched ) ),
SendPropInt( SENDINFO( m_iType ), 3 ),
SendPropEHandle( SENDINFO( m_hLauncher ) ),
SendPropBool( SENDINFO( m_bDefensiveBomb ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
static string_t s_iszTrainName;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::CTFGrenadePipebombProjectile()
{
	m_bTouched = false;
	m_flChargeTime = 0.0f;
	m_bDetonateOnPulse = false;
#ifdef GAME_DLL
	s_iszTrainName  = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
	m_flDeflectedTime = 0.0f;
	m_bWallShatter = false;
	m_bDefensiveBomb = false;
	m_bSendPlayerDestroyedEvent = true;
	m_bCanTakeDamage = true;
#else
	pEffectTrail = NULL;
	pEffectCrit = NULL;
	m_iCachedDeflect = 0;
	m_bHighlight = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::~CTFGrenadePipebombProjectile()
{
#ifdef CLIENT_DLL

	if ( pEffectTrail )
	{
		ParticleProp()->StopEmission( pEffectTrail );
	}
	if ( pEffectCrit )
	{
		ParticleProp()->StopEmission( pEffectCrit );
	}
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::GetWeaponID( void ) const
{
	if ( m_iType == TF_GL_MODE_CANNONBALL )
	{
		return TF_WEAPON_CANNON;
	}

	return ( HasStickyEffects() ? TF_WEAPON_GRENADE_PIPEBOMB : TF_WEAPON_GRENADE_DEMOMAN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGrenadePipebombProjectile::GetDamageType( void )
{
	int iDmgType = BaseClass::GetDamageType();

	// If we're a pipebomb, we do distance based damage falloff for just the first few seconds of our life
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( gpGlobals->curtime - m_flCreationTime < 5.0 )
		{
			iDmgType |= DMG_USEDISTANCEMOD;
		}
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrenadePipebombProjectile::ShouldMiniCritOnReflect() const
{
	return GetType() == TF_GL_MODE_REGULAR;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( m_hLauncher.Get() );

	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Client specific).
//

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CTFGrenadePipebombProjectile::GetTrailParticleName( void )
{
	int iTeamNumber = GetTeamNumber();

	if ( GetDeflected() && m_iType != TF_GL_MODE_REMOTE_DETONATE  )
	{
		CTFPlayer *pOwner =  ToTFPlayer( GetDeflectOwner() );

		if ( pOwner )
		{
			iTeamNumber = pOwner->GetTeamNumber();
		}
	}

	if ( HasStickyEffects() )
	{
		if ( iTeamNumber == TF_TEAM_BLUE )
		{
			return "stickybombtrail_blue";
		}
		else
		{
			return "stickybombtrail_red";
		}
	}
	else
	{
		if ( iTeamNumber == TF_TEAM_BLUE )
		{
			return "pipebombtrail_blue";
		}
		else
		{
			return "pipebombtrail_red";
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;
		m_bPulsed = false;

		CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( m_hLauncher.Get() );

		if ( pLauncher )
		{
			pLauncher->AddPipeBomb( this );
		}

		if ( m_bDefensiveBomb && C_BasePlayer::GetLocalPlayer() == GetThrower() )
		{
			if ( GetTeamNumber() == TF_TEAM_RED )
			{
				m_pGlowEffect = new CGlowObject( this, Vector( 150, 0, 0 ), 1.0, true );
			}
			else
			{
				m_pGlowEffect = new CGlowObject( this, Vector( 0, 0, 150 ), 1.0, true );
			}
		}

		CreateTrailParticles();
	}
	else if ( m_bTouched )
	{
		//ParticleProp()->StopEmission();
	}

	if ( m_iCachedDeflect != GetDeflected() )
	{
		CreateTrailParticles();
	}

	m_iCachedDeflect = GetDeflected();
}

void CTFGrenadePipebombProjectile::CreateTrailParticles( void )
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

	if ( GetDeflected() && m_iType != TF_GL_MODE_REMOTE_DETONATE )
	{
		CTFPlayer *pOwner =  ToTFPlayer( GetDeflectOwner() );

		if ( pOwner )
		{
			iTeamNumber = pOwner->GetTeamNumber();
		}
	}

	if ( m_bCritical )
	{
		switch( iTeamNumber )
		{
		case TF_TEAM_BLUE:

			if ( HasStickyEffects() )
			{
				pEffectCrit = ParticleProp()->Create( "critical_grenade_blue", PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				pEffectCrit = ParticleProp()->Create( "critical_pipe_blue", PATTACH_ABSORIGIN_FOLLOW );
			}
			break;
		case TF_TEAM_RED:

			if ( HasStickyEffects() )
			{
				pEffectCrit = ParticleProp()->Create( "critical_grenade_red", PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				pEffectCrit = ParticleProp()->Create( "critical_pipe_red", PATTACH_ABSORIGIN_FOLLOW );
			}
			break;
		default:
			break;
		}
	}

}


extern ConVar tf_grenadelauncher_livetime;

void CTFGrenadePipebombProjectile::Simulate( void )
{
	BaseClass::Simulate();

	if ( !HasStickyEffects() )
		return;

	if ( m_bPulsed == false )
	{
		if ( (gpGlobals->curtime - m_flCreationTime) >= GetLiveTime() )
		{
			if ( GetTeamNumber() == TF_TEAM_RED )
			{
				ParticleProp()->Create( "stickybomb_pulse_red", PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				ParticleProp()->Create( "stickybomb_pulse_blue", PATTACH_ABSORIGIN_FOLLOW );
			}

			m_bPulsed = true;

			if ( m_bDetonateOnPulse )
			{
				Detonate();
			}
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}

#else

//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Server specific).
//
#define TF_WEAPON_PIPEGRENADE_MODEL		"models/weapons/w_models/w_grenade_grenadelauncher.mdl"
#define TF_WEAPON_CANNONBALL_MODEL		"models/weapons/w_models/w_cannonball.mdl"
#define TF_WEAPON_PIPEBOMB_MODEL		"models/weapons/w_models/w_stickybomb.mdl"
#define TF_WEAPON_PIPEBOMB2_MODEL		"models/weapons/w_models/w_stickybomb2.mdl"
#define TF_WEAPON_PIPEBOMBD_MODEL		"models/weapons/w_models/w_stickybomb_d.mdl"
#define TF_WEAPON_PIPEBOMB_BOUNCE_SOUND	"Weapon_Grenade_Pipebomb.Bounce"
#define TF_WEAPON_CANNON_IMPACT_SOUND	"Weapon_LooseCannon.BallImpact"
#define TF_WEAPON_GRENADE_DETONATE_TIME		2.0f
#define TF_WEAPON_GRENADE_XBOX_DAMAGE 112

BEGIN_DATADESC( CTFGrenadePipebombProjectile )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_pipe_remote, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe_remote );

LINK_ENTITY_TO_CLASS( tf_projectile_pipe, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFGrenadePipebombProjectile::GetPipebombClass( int iPipeBombType )
{
	switch ( iPipeBombType )
	{
	case TF_GL_MODE_REGULAR:
		return "tf_projectile_pipe";
	case TF_GL_MODE_REMOTE_DETONATE:
	case TF_GL_MODE_REMOTE_DETONATE_PRACTICE:
		return "tf_projectile_pipe_remote";
	default:
		return "tf_projectile_pipe";
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile* CTFGrenadePipebombProjectile::Create( const Vector &position, const QAngle &angles, 
																    const Vector &velocity, const AngularImpulse &angVelocity, 
																    CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, 
																	int iPipeBombType, float flMultDmg )
{
	// Translate a projectile type into a pipebomb type.
	int iPipeBombDetonateType;
	switch ( iPipeBombType )
	{
	case TF_PROJECTILE_PIPEBOMB_REMOTE:
		{
			iPipeBombDetonateType = TF_GL_MODE_REMOTE_DETONATE;
		}
		break;
	case TF_PROJECTILE_PIPEBOMB_PRACTICE:
		{
			iPipeBombDetonateType = TF_GL_MODE_REMOTE_DETONATE_PRACTICE;
		}
		break;
	case TF_PROJECTILE_CANNONBALL:
		{
			iPipeBombDetonateType = TF_GL_MODE_CANNONBALL;
		}
		break;
	default:
		iPipeBombDetonateType = TF_GL_MODE_REGULAR;
	}
	
	const char* pszBombClass = GetPipebombClass( iPipeBombDetonateType );
	CTFGrenadePipebombProjectile *pGrenade = static_cast<CTFGrenadePipebombProjectile*>( CBaseEntity::CreateNoSpawn( pszBombClass, position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly
		pGrenade->SetPipebombMode( iPipeBombDetonateType );
		DispatchSpawn( pGrenade );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->SetDamage( pGrenade->GetDamage() * flMultDmg );
		pGrenade->SetFullDamage( pGrenade->GetDamage() );

		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			// Some hackery here. Reduce the damage, so that if we explode on timeout,
			// we'll do less damage. If we explode on contact, we'll restore this to full damage.
			pGrenade->SetDamage( pGrenade->GetDamage() * TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION );
		}

		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );

		if ( pOwner )
		{
			pGrenade->SetTruceValidForEnt( pOwner->IsTruceValidForEnt() );
		}
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Spawn()
{
	if ( HasStickyEffects() )
	{
		// Set this to max, so effectively they do not self-implode.

		if ( m_iType == TF_GL_MODE_REMOTE_DETONATE_PRACTICE )
		{
			SetModel( TF_WEAPON_PIPEBOMB2_MODEL );
		}
		else
		{
			SetModel( TF_WEAPON_PIPEBOMB_MODEL );
		}
		SetDetonateTimerLength( FLT_MAX );
		SetContextThink( &CTFGrenadePipebombProjectile::PreArmThink, gpGlobals->curtime + 0.001f, "PRE_ARM_THINK" ); // Next frame.
		SetTouch( &CTFGrenadePipebombProjectile::StickybombTouch );
	}
	else
	{
		if ( m_iType == TF_GL_MODE_CANNONBALL )
		{
			SetModel( TF_WEAPON_CANNONBALL_MODEL );
		}
		else
		{
			SetModel( TF_WEAPON_PIPEGRENADE_MODEL );
		}
		SetDetonateTimerLength( TF_WEAPON_GRENADE_DETONATE_TIME );
		SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );
	}

	SetCustomPipebombModel();

	BaseClass::Spawn();

	m_bTouched = false;
	m_flCreationTime = gpGlobals->curtime;

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );

	m_flMinSleepTime = 0;
	AddFlag( FL_GRENADE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Precache()
{
	int iModel = PrecacheModel( TF_WEAPON_PIPEBOMB_MODEL );
	PrecacheGibsForModel( iModel );

	iModel = PrecacheModel( TF_WEAPON_PIPEBOMB2_MODEL );
	PrecacheGibsForModel( iModel );

	iModel = PrecacheModel( TF_WEAPON_PIPEBOMBD_MODEL );
	PrecacheGibsForModel( iModel );

	iModel = PrecacheModel( TF_WEAPON_PIPEGRENADE_MODEL );
	PrecacheGibsForModel( iModel );

	iModel = PrecacheModel( TF_WEAPON_CANNONBALL_MODEL );
	PrecacheGibsForModel( iModel );

	// Must add All custom Models here
	iModel = PrecacheModel( "models/workshop/weapons/c_models/c_kingmaker_sticky/w_kingmaker_stickybomb.mdl" );
	iModel = PrecacheModel( "models/workshop/weapons/c_models/c_quadball/w_quadball_grenade.mdl" );

	PrecacheParticleSystem( "stickybombtrail_blue" );
	PrecacheParticleSystem( "stickybombtrail_red" );

	PrecacheScriptSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );
	PrecacheScriptSound( TF_WEAPON_CANNON_IMPACT_SOUND );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::SetPipebombMode( int iPipebombMode /* = TF_GL_MODE_REGULAR */ )
{
	m_iType.Set( iPipebombMode );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::BounceSound( void )
{
	EmitSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Detonate()
{
	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		if ( GetLauncher() )
		{
			float flFizzle = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetLauncher(), flFizzle, stickybomb_fizzle_time );
			if ( flFizzle )
			{
				Fizzle();
			}
		}
	}

	if ( m_bFizzle )
	{
		g_pEffects->Sparks( GetAbsOrigin(), 1, 2 );
		Destroy( false );

		if ( HasStickyEffects() )
		{
			CreatePipebombGibs();
		}

		return;
	}

	BaseClass::Detonate();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGrenadePipebombProjectile::DetonateStickies()
{
	if ( !GetLauncher() )
		return false;

	bool bDetonateSticky = false;

	Vector vecOrigin = GetAbsOrigin();
	const int maxEntities = 64;
	CBaseEntity	*pObjects[ maxEntities ];
	int count = UTIL_EntitiesInSphere( pObjects, maxEntities, vecOrigin, GetDamageRadius(), FL_GRENADE );

	int iStickiesRemoved = 0;

	trace_t tr;
	for ( int i = 0; i < count; i++ )
	{
		if ( pObjects[i]->GetTeamNumber() == GetLauncher()->GetTeamNumber() )
			continue;

		CTFGrenadePipebombProjectile *pGrenade = dynamic_cast < CTFGrenadePipebombProjectile*> ( pObjects[i] );
		if ( !pGrenade )
			continue;

		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
			continue;

		if ( pGrenade->m_bFizzle )
			continue;

		UTIL_TraceLine( vecOrigin, pGrenade->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0 )
			continue; // No line of sight to the bomb.

		pGrenade->Fizzle();
		pGrenade->Detonate();

		iStickiesRemoved++;

		bDetonateSticky = true;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetThrower() );
	if ( iStickiesRemoved && pOwner )
	{
		pOwner->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_DESTROY_X_STICKYBOMBS, iStickiesRemoved );
	}

	return bDetonateSticky;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::CreatePipebombGibs( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "CheapBreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Fizzle( void )
{
	m_bFizzle = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::StickybombTouch( CBaseEntity *pOther )
{
#ifdef GAME_DLL
#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( dynamic_cast< CBossAlpha * >( pOther ) != NULL )
		{
			// stickies stick to the boss
			m_bTouched = true;
			VPhysicsGetObject()->EnableMotion( false );

			SetParent( pOther );
		}
	}
#endif
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::PipebombTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vOrigin = GetAbsOrigin();
	Vector vecSpot = vOrigin - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// PASSTIME always explode when it hits the ball
	// fixme find a non-strcmp way to do this
	if ( !V_strcmp( pOther->GetClassname(), "passtime_ball" ) )
	{
		Explode( &pTrace, GetDamageType() );
		return;
	}

	//If we already touched a surface then we're not exploding on contact anymore.
	if ( m_bTouched == true )
		return;

	bool bExploded = false;

	// Blow up if we hit an enemy we can damage
	if ( pOther->GetTeamNumber() && pOther->GetTeamNumber() != GetTeamNumber() && pOther->m_takedamage != DAMAGE_NO )
	{
		// Check to see if this is a respawn room.
		if ( !pOther->IsPlayer() )
		{
			CFuncRespawnRoom *pRespawnRoom = dynamic_cast<CFuncRespawnRoom*>( pOther );
			if ( pRespawnRoom )
			{
				if ( !pRespawnRoom->PointIsWithin( vOrigin ) )
					return;
			}
		}
		
		if ( m_iType == TF_GL_MODE_CANNONBALL )
		{
			// Damage the player to push them back
			CBaseEntity *pAttacker = GetThrower();
			if ( pAttacker && ( pOther->IsPlayer() || pOther->IsBaseObject() ) )
			{
				// check if we already penetrate through this victim
				if ( !m_penetratedEntities.HasElement( pOther ) )
				{
					// Impact damage scales with distance
					float flDistanceSq = (pOther->GetAbsOrigin() - pAttacker->GetAbsOrigin()).LengthSqr();
					float flImpactDamage = RemapValClamped( flDistanceSq, 512 * 512, 1024 * 1024, 50, 25 );

					CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vOrigin, flImpactDamage, GetDamageType(), TF_DMG_CUSTOM_CANNONBALL_PUSH );
					pOther->TakeDamage( info );

					CTFPlayer *pVictim = ToTFPlayer( pOther );
					if ( pVictim )
					{
						// apply airblast - Apply stun if they are effectively grounded so we can knock them up
						if ( !pVictim->m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) )
						{
							pVictim->m_Shared.StunPlayer( 0.5, 1.f, TF_STUN_MOVEMENT, ToTFPlayer( pAttacker ) );
						}

						Vector vecToTarget = pVictim->WorldSpaceCenter() - pAttacker->WorldSpaceCenter();
						VectorNormalize( vecToTarget );
						vecToTarget *= 400;
						vecToTarget.z += 350;	// Mimic Flamethrower AirBlast
						pVictim->ApplyGenericPushbackImpulse( vecToTarget, ToTFPlayer( pAttacker ) );
					}

					m_penetratedEntities.AddToTail( pOther );

					EmitSound( TF_WEAPON_CANNON_IMPACT_SOUND );

					// Add this guy to our donk list.  If this grenade explodes and hits anyone on our launcher's
					// donk list, they get minicrit
					CTFGrenadeLauncher* pLauncher =  dynamic_cast<CTFGrenadeLauncher*>( GetLauncher() );
					if( pLauncher )
					{
						pLauncher->AddDonkVictim( pOther );
					}
				}
				return;
			}
		}

		// Save this entity as enemy, they will take 100% damage.
		m_hEnemy = pOther;	

		// Restore damage. See comment in CTFGrenadePipebombProjectile::Create() above to understand this.
		m_flDamage = m_flFullDamage;
		Explode( &pTrace, GetDamageType() );
		bExploded = true;
	}

	// Train hack!
	if ( !bExploded && pOther->GetModelName() == s_iszTrainName && ( pOther->GetAbsVelocity().LengthSqr() > 1.0f ) )
	{
		Explode( &pTrace, GetDamageType() );
		bExploded = true;
	}

	// Explode on contact with a Boss, too
	if ( !bExploded && TFGameRules()->GetActiveBoss() && pOther == TFGameRules()->GetActiveBoss() )
	{
		Explode( &pTrace, GetDamageType() );
		bExploded = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern bool PropDynamic_CollidesWithGrenades( CBaseEntity* pBaseEntity );

void CTFGrenadePipebombProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;

	if ( m_bWallShatter )
	{
		Fizzle();
		Detonate();
		return;
	}

	if ( m_iType == TF_GL_MODE_REGULAR || m_iType == TF_GL_MODE_CANNONBALL )
	{
		if ( PropDynamic_CollidesWithGrenades( pHitEntity) )
		{
			if ( m_bTouched == false )
			{
				SetThink( &CTFGrenadePipebombProjectile::Detonate );
				SetNextThink( gpGlobals->curtime );
			}
		}
		// Blow up if we hit an enemy we can damage
		else if ( pHitEntity->GetTeamNumber() && pHitEntity->GetTeamNumber() != GetTeamNumber() && pHitEntity->m_takedamage != DAMAGE_NO )
		{
			SetThink( &CTFGrenadePipebombProjectile::Detonate );
			SetNextThink( gpGlobals->curtime );
		}

		if ( m_bTouched == false )
		{
			SetDamage( GetDamageScaleOnWorldContact() * GetDamage() );

			int iNoBounce = 0;
			if ( GetLauncher() )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( GetLauncher(), iNoBounce, grenade_no_bounce )
				if ( iNoBounce )
				{
					Vector velocity;
					AngularImpulse angularVelocity;
					VPhysicsGetObject()->GetVelocity( &velocity, &angularVelocity );
					velocity *= 0.1f;
					VPhysicsGetObject()->SetVelocity( &velocity, &angularVelocity );
				}
			}
		}
		
		m_bTouched = true;
		return;
	}

	// Handle hitting skybox (disappear).
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops->game.material == 'X' )
	{
		// uncomment to destroy grenade upon hitting sky brush
		//SetThink( &CTFGrenadePipebombProjectile::SUB_Remove );
		//SetNextThink( gpGlobals->curtime );
		return;
	}

	bool bIsDynamicProp = ( NULL != dynamic_cast<CDynamicProp *>( pHitEntity ) );

	// Temp: Don't stick to the saw blades in sawmill.
	// We should make the saws their own entity type for networking.
	if ( FStrEq( pHitEntity->m_iParent.ToCStr(), "sawmovelinear01" ) ||
		 FStrEq( pHitEntity->m_iParent.ToCStr(), "sawmovelinear02" ) ||
		  PropDynamic_CollidesWithGrenades( pHitEntity) )
	{
		bIsDynamicProp = false;
	}

	// Pipebombs stick to the world when they touch it
	if ( pHitEntity && ( pHitEntity->IsWorld() || bIsDynamicProp ) && gpGlobals->curtime > m_flMinSleepTime )
	{
		m_bTouched = true;

		g_PostSimulationQueue.QueueCall( VPhysicsGetObject(), &IPhysicsObject::EnableMotion, false );

		// Save impact data for explosions.
		m_bUseImpactNormal = true;
		pEvent->pInternalData->GetSurfaceNormal( m_vecImpactNormal );
		m_vecImpactNormal.Negate();
		m_flTouchedTime = gpGlobals->curtime;

		float flFizzle = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetLauncher(), flFizzle, stickybomb_fizzle_time );
		if ( flFizzle > 0 )
		{
			SetDetonateTimerLength( flFizzle );
		}
	}
}

ConVar tf_grenade_forcefrom_bullet( "tf_grenade_forcefrom_bullet", "2.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_forcefrom_buckshot( "tf_grenade_forcefrom_buckshot", "0.75", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_forcefrom_blast( "tf_grenade_forcefrom_blast", "0.15", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_force_sleeptime( "tf_grenade_force_sleeptime", "1.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );	// How long after being shot will we re-stick to the world.
ConVar tf_pipebomb_force_to_move( "tf_pipebomb_force_to_move", "1500.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_pipebomb_deflect_reset_time( "tf_pipebomb_deflect_reset_time", "10.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !info.GetAttacker() )
	{
		Assert( !info.GetAttacker() );
		return 0;
	}

	bool bSameTeam = ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() );
	if ( !bSameTeam && CanTakeDamage() )
	{
		if ( m_bTouched && HasStickyEffects() && ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_BLAST|DMG_SONIC|DMG_MELEE) ) )
		{
			Vector vecForce = info.GetDamageForce();

			bool bBreakPipes = false;

			if ( info.GetDamageType() & (DMG_BULLET|DMG_MELEE) )
			{
				vecForce *= tf_grenade_forcefrom_bullet.GetFloat();
				bBreakPipes = true;
			}
			if ( info.GetDamageType() & DMG_SONIC )
			{
				vecForce *= tf_grenade_forcefrom_bullet.GetFloat();
			}
			else if ( info.GetDamageType() & DMG_BUCKSHOT )
			{
				vecForce *= tf_grenade_forcefrom_buckshot.GetFloat();
				bBreakPipes = true;
			}
			else if ( info.GetDamageType() & DMG_BLAST )
			{
				// if we're also supposed to ignite then just destroy the sticky bomb (Cow Mangler alt-fire)
				if ( info.GetDamageType() & DMG_IGNITE )
				{
					bBreakPipes = true;
				}
				vecForce *= tf_grenade_forcefrom_blast.GetFloat();
			}

			if ( bBreakPipes == true )
			{
				// we might get multiple calls for the same pipe when shooting it with a shotgun,
				// so make sure it only sends the player_destroyed_pipebomb event once
				if ( m_bSendPlayerDestroyedEvent )
				{
					if ( info.GetAttacker()->IsPlayer() )
					{
						CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
						if ( pPlayer )
						{
							IGameEvent * event = gameeventmanager->CreateEvent( "player_destroyed_pipebomb" );
							if ( event )
							{
								event->SetInt( "userid", pPlayer->GetUserID() );
								gameeventmanager->FireEvent( event );
							}

							if ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
							{
								// If we are near a building, award achievement progress.
								CTFTeam *pTeam = pPlayer->GetTFTeam();
								if ( pTeam )
								{
									for ( int i=0; i<pTeam->GetNumObjects(); i++ )
									{
										CBaseObject *pObject = pTeam->GetObject(i);
										if ( pObject && pObject->GetAbsOrigin().DistTo( GetAbsOrigin() ) < 100 &&
											pObject->ObjectType() != OBJ_ATTACHMENT_SAPPER )
										{
											pPlayer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_DESTROY_STICKIES, 1 );
											break; // Only one award per sticky.
										}
									}
								}
							}
						}

						m_bSendPlayerDestroyedEvent = false;
					}
				}

				Fizzle();
				Detonate();
			}

			// If the force is sufficient, detach & move the pipebomb
			float flForce = tf_pipebomb_force_to_move.GetFloat();
			if ( vecForce.LengthSqr() > (flForce*flForce) )
			{
				if ( VPhysicsGetObject() )
				{
					VPhysicsGetObject()->EnableMotion( true );
				}

				CTakeDamageInfo newInfo = info;
				newInfo.SetDamageForce( vecForce );

				VPhysicsTakeDamage( newInfo );

				// The pipebomb will re-stick to the ground after this time expires
				m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
				m_bTouched = false;

				// It has moved the data is no longer valid.
				m_bUseImpactNormal = false;
				m_vecImpactNormal.Init();

				return 1;
			}
		}
	}

	return 0;
}

void CTFGrenadePipebombProjectile::IncrementDeflected( void )
{
	BaseClass::IncrementDeflected();

	if ( GetDeflected() && HasStickyEffects() )
	{
		m_flDeflectedTime = gpGlobals->curtime + tf_pipebomb_deflect_reset_time.GetFloat();
	}

	int iTeamNumber = GetTeamNumber();

	CTFPlayer *pOwner =  ToTFPlayer( GetDeflectOwner() );

	if ( pOwner )
	{
		iTeamNumber = pOwner->GetTeamNumber();
	}

	if ( !HasStickyEffects() )
	{
		m_nSkin = ( iTeamNumber == TF_TEAM_BLUE ) ? 1 : 0;
	}
}

void CTFGrenadePipebombProjectile::DetonateThink( void )
{
	BaseClass::DetonateThink();

	if ( m_flDeflectedTime <= gpGlobals->curtime && HasStickyEffects() )
	{
		ResetDeflected();
		SetDeflectOwner( NULL );
	}

	// If we received our crit via a medic, make sure they still exist.
	if ( m_CritMedics.Count()  )
	{
		if ( TFGameRules() && ( TFGameRules()->InSetup() || TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS ) )
		{
			bool bRemove = true;

			FOR_EACH_VEC( m_CritMedics, i )
			{
				if ( m_CritMedics[i] && m_CritMedics[i]->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
				{
					bRemove = false;
					break;
				}
			}

			// No medic(s)
			if ( bRemove )
			{
				Fizzle();
				Detonate();
				return;
			}
		}
		else
		{
			// Clear the vector when the game starts
			m_CritMedics.RemoveAll();
		}
	}
}

void CTFGrenadePipebombProjectile::PreArmThink( void )
{
	SetContextThink( &CTFGrenadePipebombProjectile::ArmThink, gpGlobals->curtime + GetLiveTime(), "ARM_THINK" );
}

void CTFGrenadePipebombProjectile::ArmThink( void )
{
	// When between waves in MvM, players sometimes switch to medic just so demos can place crit stickies, 
	// and then switch back.  This code removes the sticky if the medic switches ( in DetonateThink() )
	if ( IsCritical() && HasStickyEffects() && TFGameRules() && ( TFGameRules()->InSetup() || TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS ) )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetThrower() );
		if ( pOwner && pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED ) && !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_USER_BUFF ) )
		{
			// Find the medic(s)
			for ( int i = 0; i < pOwner->m_Shared.GetNumHealers(); i++ )
			{
				CTFPlayer *pMedic = ToTFPlayer( pOwner->m_Shared.GetHealerByIndex( i ) );
				if ( !pMedic )
					continue;

				CWeaponMedigun *pMedigun = dynamic_cast <CWeaponMedigun*>( pMedic->GetActiveTFWeapon() );
				if ( pMedigun && pMedigun->IsReleasingCharge() && pMedigun->GetChargeType() == MEDIGUN_CHARGE_CRITICALBOOST )
				{
					m_CritMedics.AddToTail( pMedic );
				}
			}

			// We didn't find the medic.  What provided TF_COND_CRITBOOSTED?
			Assert( m_CritMedics.Count() );
		}
	}

	if ( m_bDetonateOnPulse )
	{
		Detonate();
	}
}

#endif

//------------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGrenadePipebombProjectile::GetLiveTime( void )
{
	float flLiveTime = tf_grenadelauncher_livetime.GetFloat();

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetLauncher(), flLiveTime, sticky_arm_time );

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetThrower() );

		if ( pOwner )
		{
			if ( pOwner->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
			{
				flLiveTime *= 0.5f;
			}
			else if ( pOwner->m_Shared.GetCarryingRuneType() == RUNE_KING || pOwner->m_Shared.InCond( TF_COND_KING_BUFFED ) )
			{
				flLiveTime *= 0.75f;
			}
		}
	}

	return flLiveTime;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Grenade was deflected.
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Deflected( CBaseEntity *pDeflectedBy, Vector& vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	CTFPlayer* pOldOwner = NULL;
	if ( HasStickyEffects() )
	{
		CTakeDamageInfo info;

		float flForceMultiplier = 1.0f;
		ITFChargeUpWeapon *pWeapon = dynamic_cast<ITFChargeUpWeapon*>( pTFDeflector->GetActiveWeapon() );
		if ( pWeapon )
		{
			flForceMultiplier = RemapValClamped( ( gpGlobals->curtime - pWeapon->GetChargeBeginTime() ),
												 0.0f,
												 pWeapon->GetChargeMaxTime(),
												 1.0f,
												 2.0f );
		}
		Vector vecForce = vecDir * flForceMultiplier * CTFWeaponBase::DeflectionForce( WorldAlignSize(), 90, 12.0f );
		
		pOldOwner = ToTFPlayer( GetThrower() );
		info.SetAttacker( pDeflectedBy );
		info.SetDamageForce( vecForce );
		info.SetDamageType( DMG_SONIC );
		info.SetWeapon( pTFDeflector->GetActiveTFWeapon() );
		OnTakeDamage( info );
	}
	else
	{
		ChangeTeam( pTFDeflector->GetTeamNumber() );
		SetLauncher( pTFDeflector->GetActiveWeapon() );
		pOldOwner = ToTFPlayer( GetThrower() );
		SetThrower( pTFDeflector );

		if ( pTFDeflector->m_Shared.IsCritBoosted() )
		{
			SetCritical( true );
		}
	}

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	CTFWeaponBase::SendObjectDeflectedEvent( pTFDeflector, pOldOwner, GetWeaponID(), this );

	SetDeflectOwner( pTFDeflector );
	IncrementDeflected();
}
#endif


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Highlight FX
//-----------------------------------------------------------------------------
class CProxyStickybombGlowColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		CTFGrenadePipebombProjectile *pGrenade = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		// default to [1 1 1]
		Vector vResult = Vector( 1, 1, 1 );

		pGrenade = dynamic_cast<CTFGrenadePipebombProjectile*>( pEntity );
		if ( pGrenade )
		{
			if ( pGrenade->IsHighlighted() )
			{
				int iTeamNumber = pGrenade->GetTeamNumber();
				if ( iTeamNumber == TF_TEAM_RED )
				{
					vResult = Vector ( 100.f, 0.f, 0.f );
					if ( pGrenade->m_pGlowEffect )
					{
						pGrenade->m_pGlowEffect->SetColor( Vector( 250, 0, 0 ) );
					}
				}
				else
				{
					vResult = Vector ( 0.f, 0.f, 100.f );
					if ( pGrenade->m_pGlowEffect )
					{
						pGrenade->m_pGlowEffect->SetColor( Vector( 0, 0, 250 ) );
					}
				}
			}
			else
			{
				int iTeamNumber = pGrenade->GetTeamNumber();
				if ( iTeamNumber == TF_TEAM_RED )
				{
					if ( pGrenade->m_pGlowEffect )
					{
						pGrenade->m_pGlowEffect->SetColor( Vector( 200, 100, 100 ) );
					}
				}
				else
				{
					if ( pGrenade->m_pGlowEffect )
					{
						pGrenade->m_pGlowEffect->SetColor( Vector( 100, 100, 200 ) );
					}
				}
			}
		}
		m_pResult->SetVecValue( vResult.x, vResult.y, vResult.z );
	}
};
EXPOSE_INTERFACE( CProxyStickybombGlowColor, IMaterialProxy, "StickybombGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#if GAME_DLL
int CTFGrenadePipebombProjectile::GetDamageCustom()
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( !m_bTouched )
		{
			return TF_DMG_CUSTOM_AIR_STICKY_BURST;
		}
		else if ( m_bDefensiveBomb )
		{
			return TF_DMG_CUSTOM_DEFENSIVE_STICKY;
		}
		else
		{
			return TF_DMG_CUSTOM_STANDARD_STICKY;
		}
	}
	else if ( m_iType == TF_GL_MODE_REMOTE_DETONATE_PRACTICE )
	{
		return TF_DMG_CUSTOM_PRACTICE_STICKY;
	}

	return BaseClass::GetDamageCustom();
}


float CTFGrenadePipebombProjectile::GetDamageScaleOnWorldContact()
{
	float flGrenadeDamageScaleOnWorldContact = 1.f;
	if ( GetLauncher() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetLauncher(),flGrenadeDamageScaleOnWorldContact, grenade_damage_reduction_on_world_contact );
	}
	return flGrenadeDamageScaleOnWorldContact;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::UpdateTransmitState()
{
	if ( m_bDefensiveBomb )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	return BaseClass::UpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( m_bDefensiveBomb )
	{
		return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}

#endif

//-----------------------------------------------------------------------------
float CTFGrenadePipebombProjectile::GetDamageRadius() 
{
	float flRadiusMod = 1.0f;

#ifdef GAME_DLL
	// winbomb prevention.
	// Air Det
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( m_bTouched == false )
		{
			float flArmTime = tf_grenadelauncher_livetime.GetFloat();
			flRadiusMod *= RemapValClamped( gpGlobals->curtime - m_flCreationTime, flArmTime, flArmTime + tf_sticky_radius_ramp_time.GetFloat(), tf_sticky_airdet_radius.GetFloat(), 1.0 );
		}
	}
#endif // GAME_DLL
	return BaseClass::GetDamageRadius() * flRadiusMod;
}
