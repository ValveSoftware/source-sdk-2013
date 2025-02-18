//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Nail Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_NAIL_H
#define TF_WEAPON_GRENADE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeNail C_TFGrenadeNail
#endif

//=============================================================================
//
// TF Nail Grenade
//
class CTFGrenadeNail : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeNail, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
//	DECLARE_ACTTABLE();

	CTFGrenadeNail() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NAIL; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

public:

	CTFGrenadeNail( const CTFGrenadeNail & ) {}
};

//=============================================================================
//
// TF Nail Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CNailGrenadeController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();
public:
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

public:
	void SetDesiredPosAndOrientation( Vector pos, QAngle orientation );

private:
	Vector m_vecDesiredPosition;
	QAngle m_angDesiredOrientation;

	bool m_bReachedPos;
	bool m_bReachedOrientation;
};

class CTFGrenadeNailProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	~CTFGrenadeNailProjectile();

	DECLARE_CLASS( CTFGrenadeNailProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_DATADESC();

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NAIL; }

	// Creation.
	static CTFGrenadeNailProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void StartEmittingNails( void );
	void EmitNails( void );

	CNailGrenadeController		m_GrenadeController;
	IPhysicsMotionController	*m_pMotionController;

public:
	bool m_bActivated;
	float m_flNailAngle;
	int m_iNumNailBurstsLeft;
};

#endif // GAME_DLL

#endif // TF_WEAPON_GRENADE_NAIL_H
