//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Mirv Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_MIRV_H
#define TF_WEAPON_GRENADE_MIRV_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeMirv C_TFGrenadeMirv
#define CTFGrenadeMirv_Demoman C_TFGrenadeMirv_Demoman
#endif

//=============================================================================
//
// TF Mirv Grenade
//
class CTFGrenadeMirv : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeMirv, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrenadeMirv() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRV; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeMirv( const CTFGrenadeMirv & ) {}
};

// Demoman version calls different models
class CTFGrenadeMirv_Demoman : public CTFGrenadeMirv
{
public:
	DECLARE_CLASS( CTFGrenadeMirv_Demoman, CTFGrenadeMirv );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const		{ return TF_WEAPON_GRENADE_MIRV_DEMOMAN; }
};

//=============================================================================
//
// TF Mirv Grenade Projectile and Bombs (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeMirvProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeMirvProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRV; }

	// Creation.
	static CTFGrenadeMirvProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                     const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	void			DetonateThink( void );

	DECLARE_DATADESC();

private:

	bool			m_bPlayedLeadIn;
};

class CTFGrenadeMirvBomb : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeMirvBomb, CTFWeaponBaseGrenadeProj );

	// Creation.
	static CTFGrenadeMirvBomb *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                               const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, float timer );

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRVBOMB; }

	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
};

#endif

#endif // TF_WEAPON_GRENADE_MIRV_H
