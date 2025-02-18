//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FLAG_H
#define TF_WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFFlag C_TFFlag
#endif

//=============================================================================
//
// Bottle class.
//
class CTFFlag : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFFlag, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlag();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_FLAG; }
	virtual void		SecondaryAttack();
	bool				Deploy( void );

	virtual bool		CanDrop( void ) { return true; }

private:

	CTFFlag( const CTFFlag & ) {}
};

#endif // TF_WEAPON_FLAG_H
