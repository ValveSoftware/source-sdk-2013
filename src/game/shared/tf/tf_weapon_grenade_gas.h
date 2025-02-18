//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Gas Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_GAS_H
#define TF_WEAPON_GRENADE_GAS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeGas C_TFGrenadeGas
#endif

//=============================================================================
//
// TF Gas Grenade
//
class CTFGrenadeGas : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeGas, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
//	DECLARE_ACTTABLE();

	CTFGrenadeGas() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_GAS; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeGas( const CTFGrenadeGas & ) {}
};

#ifdef CLIENT_DLL
	#define CTFGasGrenadeEffect C_TFGasGrenadeEffect
#endif

class CTFGasGrenadeEffect : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFGasGrenadeEffect, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifndef CLIENT_DLL

	virtual int UpdateTransmitState( void );

#else

	CTFGasGrenadeEffect()
	{
		m_pGasEffect = NULL;
	}

	virtual void OnDataChanged( DataUpdateType_t updateType );

	CNewParticleEffect *m_pGasEffect;

#endif

};

//=============================================================================
//
// TF Gase Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeGasProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeGasProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_DATADESC();

	~CTFGrenadeGasProjectile();

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_GAS; }

	// Creation.
	static CTFGrenadeGasProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	DetonateThink( void );

	void Think_Emit( void );
	void Think_Fade( void );

private:
	int	m_nPulses;

	CHandle<CTFGasGrenadeEffect> m_hGasEffect;
};

#endif

#endif // TF_WEAPON_GRENADE_GAS_H
