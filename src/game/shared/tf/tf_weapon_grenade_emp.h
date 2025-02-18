//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Emp Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_EMP_H
#define TF_WEAPON_GRENADE_EMP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeEmp C_TFGrenadeEmp
#endif

//=============================================================================
//
// TF Emp Grenade
//
class CTFGrenadeEmp : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeEmp, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
//	DECLARE_ACTTABLE();

	CTFGrenadeEmp() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_EMP; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeEmp( const CTFGrenadeEmp & ) {}
};

//=============================================================================
//
// TF Emp Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeEmpProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeEmpProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_EMP; }

	// Creation.
	static CTFGrenadeEmpProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	void			DetonateThink( void );

	DECLARE_DATADESC();

private:

	bool			m_bPlayedLeadIn;
};

#endif

#endif // TF_WEAPON_GRENADE_EMP_H
