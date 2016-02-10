//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_CSBASE_GUN_H
#define WEAPON_CSBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


// This is the base class for pistols and rifles.
#if defined( CLIENT_DLL )

	#define CWeaponCSBaseGun C_WeaponCSBaseGun

#else
#endif


class CWeaponCSBaseGun : public CWeaponCSBase
{
public:
	
	DECLARE_CLASS( CWeaponCSBaseGun, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponCSBaseGun();

	virtual void PrimaryAttack();
	virtual void Spawn();
	virtual bool Deploy();
	virtual bool Reload();
	virtual void WeaponIdle();


	// Derived classes call this to fire a bullet.
	bool CSBaseGunFire( float flSpread, float flCycleTime, bool bPrimaryMode );

	// Usually plays the shot sound. Guns with silencers can play different sounds.
	virtual void DoFireEffects();
	virtual void ItemPostFrame();

protected: 
	float m_zoomFullyActiveTime;

private:

	CWeaponCSBaseGun( const CWeaponCSBaseGun & );
};


#endif // WEAPON_CSBASE_GUN_H
