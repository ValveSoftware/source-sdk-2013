//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Energy Ball
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_energy_ball.h"
#include "soundent.h"
#include "tf_fx.h"
#include "props.h"
#include "baseobject_shared.h"
#include "SpriteTrail.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "collisionutils.h"
#include "bone_setup.h"
#include "decals.h"
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_gamerules.h"
#include "tf_weapon_rocketlauncher.h"

//=============================================================================
//
// TF Energy Ball Projectile functions (Server specific).
//
#define ENERGY_BALL_MODEL					"models/weapons/w_models/w_drg_ball.mdl"
#define ARROW_GRAVITY				0.3f

#define ENERGY_BALL_THINK_CONTEXT			"CTFProjectile_EnergyBallThink"

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_energy_ball, CTFProjectile_EnergyBall );
PRECACHE_WEAPON_REGISTER( tf_projectile_energy_ball );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )

BEGIN_NETWORK_TABLE( CTFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )
	SendPropBool( SENDINFO( m_bChargedShot ) ),
	SendPropVector( SENDINFO( m_vColor1 ), 8, 0, 0, 1 ),
	SendPropVector( SENDINFO( m_vColor2 ), 8, 0, 0, 1 )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_EnergyBall )
//DEFINE_THINKFUNC( ImpactThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyBall::CTFProjectile_EnergyBall()
{
	m_bChargedShot = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyBall::~CTFProjectile_EnergyBall()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyBall *CTFProjectile_EnergyBall::Create( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_EnergyBall *pBall = static_cast<CTFProjectile_EnergyBall*>( CBaseEntity::Create( "tf_projectile_energy_ball", vecOrigin, vecAngles, pOwner ) );
	if ( pBall )
	{
		pBall->InitEnergyBall( vecOrigin, vecAngles, fSpeed, fGravity, pOwner, pScorer );
	}

	return pBall;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::InitEnergyBall( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	// Initialize the owner.
	SetOwnerEntity( pOwner );

	// Set team.
	ChangeTeam( pOwner->GetTeamNumber() );

	// Spawn.
	Spawn();

	SetGravity( fGravity );

	SetCritical( true );

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = vecForward * fSpeed;
	
	SetAbsVelocity( vecVelocity );	
	SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	SetAbsAngles( angles );

	// Save the scoring player.
	SetScorer( pScorer );

	if ( pScorer )
	{
		SetTruceValidForEnt( pScorer->IsTruceValidForEnt() );
	}

	m_flInitTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Spawn()
{	
	SetModel( ENERGY_BALL_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Precache()
{
	PrecacheParticleSystem( "drg_cow_rockettrail_charged" );
	PrecacheParticleSystem( "drg_cow_rockettrail_charged_blue" );
	PrecacheParticleSystem( "drg_cow_rockettrail_normal" );
	PrecacheParticleSystem( "drg_cow_rockettrail_normal_blue" );

	PrecacheModel( ENERGY_BALL_MODEL );

	PrecacheScriptSound( "Weapon_CowMangler.Explode" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_EnergyBall::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::ImpactSound( const char *pszSoundName, bool bLoudForAttacker )
{
	CTFPlayer *pAttacker = ToTFPlayer( GetScorer() );
	if ( !pAttacker )
		return;

	if ( bLoudForAttacker )
	{
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = pszSoundName;
		params.m_pflSoundDuration = &soundlen;
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( ToTFPlayer(pAttacker) );
		EmitSound( filter, entindex(), params );

		CSingleUserRecipientFilter attackerFilter( ToTFPlayer(pAttacker) );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( pszSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::FadeOut( int iTime )
{
	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( vec3_origin	);
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	// Start remove timer.
	SetContextThink( &CTFProjectile_EnergyBall::RemoveThink, gpGlobals->curtime + iTime, "ENERGY_BALL_REMOVE_THINK" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::RemoveThink( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Arrow was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	ChangeTeam( pTFDeflector->GetTeamNumber() );
	SetLauncher( pTFDeflector->GetActiveWeapon() );

	CTFPlayer* pOldOwner = ToTFPlayer( GetOwnerEntity() );
	SetOwnerEntity( pTFDeflector );

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	if ( pTFDeflector->m_Shared.IsCritBoosted() )
	{
		SetCritical( true );
	}

	CTFWeaponBase::SendObjectDeflectedEvent( pTFDeflector, pOldOwner, GetWeaponID(), this );

	IncrementDeflected();
	SetScorer( pTFDeflector );

	// Change particle color data.
	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		m_vColor1 = TF_PARTICLE_WEAPON_BLUE_1;
		m_vColor2 = TF_PARTICLE_WEAPON_BLUE_2;
	}
	else
	{
		m_vColor1 = TF_PARTICLE_WEAPON_RED_1;
		m_vColor2 = TF_PARTICLE_WEAPON_RED_2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	if ( ShouldNotDetonate() )
	{
		Destroy( true );
		return;
	}

	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Particle (oriented)
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	QAngle angExplosion( 0.f, 0.f, 0.f );
	VectorAngles( pTrace->plane.normal, angExplosion );
	TE_TFParticleEffect( filter, 0.f, GetExplosionParticleName(), vecOrigin, pTrace->plane.normal, angExplosion, NULL );
	
	// Screenshake
	if ( m_bChargedShot )
	{
		UTIL_ScreenShake( WorldSpaceCenter(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	}

	// Sound
	ImpactSound( "Weapon_CowMangler.Explode" );
	CSoundEnt::InsertSound ( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	float flRadius = GetRadius();

	if ( pAttacker ) // No attacker, deal no damage. Otherwise we could potentially kill teammates.
	{
		CTFPlayer *pTarget = ToTFPlayer( GetEnemy() );
		if ( pTarget )
		{
			// Rocket Specialist
			CheckForStunOnImpact( pTarget );

			if ( pTarget->GetTeamNumber() != pAttacker->GetTeamNumber() )
			{
				RecordEnemyPlayerHit( pTarget, true );
			}
		}

		CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType(), GetDamageCustom() );
		CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, flRadius, NULL, m_bChargedShot ? TF_ROCKET_RADIUS_FOR_RJS*1.33 : TF_ROCKET_RADIUS_FOR_RJS );
		TFGameRules()->RadiusDamage( radiusinfo );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_EnergyBall::GetExplosionParticleName( void )
{
	if ( m_bChargedShot )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_explosioncore_charged" : "drg_cow_explosioncore_charged_blue";
	}
	else
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_explosioncore_normal" : "drg_cow_explosioncore_normal_blue";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyBall::GetDamage()
{
	return m_flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_EnergyBall::GetDamageType()
{
	int iDamageType;
	if ( m_bChargedShot )
	{
		iDamageType = DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD | DMG_IGNITE;
	}
	else
	{
		iDamageType = DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD;
		if ( m_bCritical )
		{
			iDamageType |= DMG_CRITICAL;
		}
	}

	return iDamageType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_EnergyBall::GetDamageCustom()
{
	return m_bChargedShot ? TF_DMG_CUSTOM_PLASMA_CHARGED : TF_DMG_CUSTOM_PLASMA;
}

