//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SYRINGEGUN_H
#define TF_WEAPON_SYRINGEGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFSyringeGun C_TFSyringeGun
#endif

enum syringe_weapontypes_t
{
	SYRINGE_DEFAULT = 0,
	SYRINGE_UBER_SCALES_SPEED,
};

//=============================================================================
//
// TF Weapon Syringe gun.
//
class CTFSyringeGun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSyringeGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFSyringeGun() {}
	~CTFSyringeGun() {}

	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SYRINGEGUN_MEDIC; }
	int				GetSyringeType( void ) const		{ int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual void RemoveProjectileAmmo( CTFPlayer *pPlayer );
	virtual bool HasPrimaryAmmo( void );

private:

	CTFSyringeGun( const CTFSyringeGun & ) {}
};

#endif // TF_WEAPON_SYRINGEGUN_H
