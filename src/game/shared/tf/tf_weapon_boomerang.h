//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BOOMERANG_H
#define TF_WEAPON_BOOMERANG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBoomerang C_TFBoomerang
#endif

//=============================================================================
//
// Boomerang class.
//
class CTFBoomerang : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBoomerang, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBoomerang();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BOOMERANG; }

private:

	CTFBoomerang( const CTFBoomerang & ) {}
};

#endif // TF_WEAPON_BOOMERANG_H
