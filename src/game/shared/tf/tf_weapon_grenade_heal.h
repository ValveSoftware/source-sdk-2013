//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Heal Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_HEAL_H
#define TF_WEAPON_GRENADE_HEAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeHeal C_TFGrenadeHeal
#endif

//=============================================================================
//
// TF Heal Grenade
//
class CTFGrenadeHeal : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeHeal, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	//	DECLARE_ACTTABLE();

	CTFGrenadeHeal() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_HEAL; }

	// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeHeal( const CTFGrenadeHeal & ) {}
};

//=============================================================================
//
// TF Heal Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeHealProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeHealProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_HEAL; }

	// Creation.
	static CTFGrenadeHealProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
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

#endif // TF_WEAPON_GRENADE_HEAL_H
