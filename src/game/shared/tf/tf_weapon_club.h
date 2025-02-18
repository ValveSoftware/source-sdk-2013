//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_CLUB_H
#define TF_WEAPON_CLUB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFClub C_TFClub
#endif

//=============================================================================
//
// Club class.
//
class CTFClub : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFClub, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFClub();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CLUB; }

private:

	CTFClub( const CTFClub & ) {}
};

#endif // TF_WEAPON_CLUB_H
