//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Napalm Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_napalm.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#endif

//=============================================================================
//
// TF Napalm Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeNapalm, DT_TFGrenadeNapalm )

BEGIN_NETWORK_TABLE( CTFGrenadeNapalm, DT_TFGrenadeNapalm )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeNapalm )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_napalm, CTFGrenadeNapalm );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_napalm );

//=============================================================================
//
// TF Napalm Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeNapalm )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeNapalm::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	return CTFGrenadeNapalmProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

//=============================================================================
//
// TF Napalm Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_napalm.mdl"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_napalm_projectile, CTFGrenadeNapalmProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_napalm_projectile );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeNapalmProjectile* CTFGrenadeNapalmProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeNapalmProjectile *pGrenade = static_cast<CTFGrenadeNapalmProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_napalm_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Nail.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();

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

#endif
