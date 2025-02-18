//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_DECOY_H
#define TF_WEAPON_DECOY_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_jar.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFDecoy C_TFDecoy
#endif

//=============================================================================
//
// Spy Decoy weapon.
//
class CTFDecoy : public CTFJar
{
public:
	DECLARE_CLASS( CTFDecoy, CTFJar );
	DECLARE_NETWORKCLASS(); 

/*
// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
*/

	CTFDecoy();
	~CTFDecoy() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_LIFELINE; }	// TODO: Need real weapon ID

	virtual void	PrimaryAttack( void );

	virtual bool	ShouldDrawCrosshair( void )						{ return false; }
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }

private:
	CTFDecoy( const CTFDecoy & ) {}
};


#endif // TF_WEAPON_DECOY_H
