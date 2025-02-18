//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Gas Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_SMOKE_BOMB_H
#define TF_WEAPON_GRENADE_SMOKE_BOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeSmokeBomb C_TFGrenadeSmokeBomb
#endif

//=============================================================================
//
// TF Gas Grenade
//
class CTFGrenadeSmokeBomb : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeSmokeBomb, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	//	DECLARE_ACTTABLE();

	CTFGrenadeSmokeBomb() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_SMOKE_BOMB; }

	// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

	virtual bool ShouldDetonate( void );

#endif

	CTFGrenadeSmokeBomb( const CTFGrenadeSmokeBomb & ) {}
};

#endif // TF_WEAPON_GRENADE_SMOKE_BOMB_H
