//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Concussion Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_CONCUSSION_H
#define TF_WEAPON_GRENADE_CONCUSSION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeConcussion C_TFGrenadeConcussion
#endif

//=============================================================================
//
// TF Concussion Grenade
//
class CTFGrenadeConcussion : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeConcussion, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrenadeConcussion() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_CONCUSSION; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeConcussion( const CTFGrenadeConcussion & ) {}
};

//=============================================================================
//
// TF Concussion Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeConcussionProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeConcussionProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_CONCUSSION; }

	// Creation.
	static CTFGrenadeConcussionProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	virtual void	Detonate();

private:

	float m_flDetonateTime;
};

#endif

#endif // TF_WEAPON_GRENADE_CONCUSSION_H
