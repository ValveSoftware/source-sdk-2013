//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_syringegun.h"
#include "tf_fx_shared.h"
#include "tf_weapon_medigun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Syringe Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSyringeGun, DT_WeaponSyringeGun )

BEGIN_NETWORK_TABLE( CTFSyringeGun, DT_WeaponSyringeGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSyringeGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_syringegun_medic, CTFSyringeGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_syringegun_medic );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFSyringeGun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon SyringeGun functions.
//
void CTFSyringeGun::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheParticleSystem( "nailtrails_medic_blue_crit" );
	PrecacheParticleSystem( "nailtrails_medic_blue" );
	PrecacheParticleSystem( "nailtrails_medic_red_crit" );
	PrecacheParticleSystem( "nailtrails_medic_red" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSyringeGun::Deploy()
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		ToTFPlayer( pOwner )->TeamFortress_SetSpeed();
	}
	
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSyringeGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		ToTFPlayer( pOwner )->TeamFortress_SetSpeed();
	}

	return BaseClass::Holster( pSwitchingTo );
}

void CTFSyringeGun::RemoveProjectileAmmo( CTFPlayer *pPlayer )
{
	float flUberChargeAmmoPerShot = UberChargeAmmoPerShot();
	if ( flUberChargeAmmoPerShot > 0.0f )
	{
#ifndef CLIENT_DLL
		if ( !pPlayer )
			return;

		CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		if ( !pMedigun )
			return;

		pMedigun->SubtractCharge( flUberChargeAmmoPerShot );
#endif
		return;
	}

	return BaseClass::RemoveProjectileAmmo( pPlayer );
}

bool CTFSyringeGun::HasPrimaryAmmo( void )
{
	float flUberChargeAmmoPerShot = UberChargeAmmoPerShot();
	if ( flUberChargeAmmoPerShot > 0.0f )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( !pPlayer )
			return false;

		CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		if ( !pMedigun )
			return false;

		float flCharge = pMedigun->GetChargeLevel();
		return flUberChargeAmmoPerShot <= flCharge;
	}

	return BaseClass::HasPrimaryAmmo();
}
