//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Emp Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_emp.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "particle_parse.h"
#include "beam_shared.h"
#endif

#define GRENADE_EMP_TIMER	3.0f //Seconds
#define	GRENADE_EMP_LEADIN	2.0f 

//=============================================================================
//
// TF Emp Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeEmp, DT_TFGrenadeEmp )

BEGIN_NETWORK_TABLE( CTFGrenadeEmp, DT_TFGrenadeEmp )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeEmp )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_emp, CTFGrenadeEmp );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_emp );

//=============================================================================
//
// TF Emp Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeEmp )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeEmp::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	return CTFGrenadeEmpProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

//=============================================================================
//
// TF Emp Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_emp.mdl"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_emp_projectile, CTFGrenadeEmpProjectile );
PRECACHE_REGISTER( tf_weapon_grenade_emp_projectile );

BEGIN_DATADESC( CTFGrenadeEmpProjectile )
DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeEmpProjectile* CTFGrenadeEmpProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeEmpProjectile *pGrenade = static_cast<CTFGrenadeEmpProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_emp_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Spawn()
{
	Precache();
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();

	m_bPlayedLeadIn = false;

	SetThink( &CTFGrenadeEmpProjectile::DetonateThink );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheScriptSound( "Weapon_Grenade_Emp.LeadIn" );
	PrecacheModel( "sprites/physcannon_bluelight1b.vmt" );
	PrecacheParticleSystem( "emp_shockwave" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Emp.Bounce" );
}

extern ConVar tf_grenade_show_radius;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// Explosion effect on client
	SendDispatchEffect();

	float flRadius = 180;
	float flDamage = 1;

	if ( tf_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	// Apply some amount of EMP damage to every entity in the radius. They will calculate 
	// their own damage based on how much ammo they have or some other wacky calculation.

	CTakeDamageInfo info( this, GetThrower(), vec3_origin, GetAbsOrigin(), flDamage, DMG_EMP | DMG_PREVENT_PHYSICS_FORCE );

	CBaseEntity *pEntityList[100];
	int nEntityCount = UTIL_EntitiesInSphere( pEntityList, 100, GetAbsOrigin(), flRadius, 0 );
	int iEntity;
	for ( iEntity = 0; iEntity < nEntityCount; ++iEntity )
	{
		CBaseEntity *pEntity = pEntityList[iEntity];

		if ( pEntity == this )
			continue;

		if ( pEntity && pEntity->IsPlayer() )
			continue;

		if ( pEntity && ( pEntity->m_takedamage == DAMAGE_YES || pEntity->m_takedamage == DAMAGE_EVENTS_ONLY ) )
		{
			pEntity->TakeDamage( info );

			//if ( pEntity->IsPlayer() /* || is ammo box || is enemy object */ )
			{
				CBeam *pBeam = CBeam::BeamCreate( "sprites/physcannon_bluelight1b.vmt", 3.0 );
				if ( !pBeam )
					return;

				pBeam->PointsInit( GetAbsOrigin(), pEntity->WorldSpaceCenter() );

				pBeam->SetColor( 255, 255, 255 );
				pBeam->SetBrightness( 128 );
				pBeam->SetNoise( 12.0f );
				pBeam->SetEndWidth( 3.0f );
				pBeam->SetWidth( 3.0f );
				pBeam->LiveForTime( 0.5f );	// Fail-safe
				pBeam->SetFrameRate( 25.0f );
				pBeam->SetFrame( random->RandomInt( 0, 2 ) );
			}
		}
	}

	DispatchParticleEffect( "emp_shockwave", GetAbsOrigin(), vec3_angle );

	UTIL_Remove( this );

#if 0
	// Tell the bots an HE grenade has exploded
	CTFPlayer *pPlayer = ToTFPlayer( GetThrower() );
	if ( pPlayer )
	{
		KeyValues *pEvent = new KeyValues( "tf_weapon_grenade_detonate" );
		pEvent->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEventServerOnly( pEvent );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::DetonateThink( void )
{
	if ( !m_bPlayedLeadIn && gpGlobals->curtime > GetDetonateTime() - GRENADE_EMP_LEADIN )
	{
		Vector soundPosition = GetAbsOrigin() + Vector( 0, 0, 5 );
		CPASAttenuationFilter filter( soundPosition );

		EmitSound( filter, entindex(), "Weapon_Grenade_Emp.LeadIn" );
		m_bPlayedLeadIn = true;
	}

	BaseClass::DetonateThink();
}

#endif
