//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Laser Pointer
//
//=============================================================================//
#ifndef TF_WEAPON_LASER_POINTER_H
#define TF_WEAPON_LASER_POINTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_sniperrifle.h"
#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CTFLaserPointer C_TFLaserPointer
#define CLaserDot C_LaserDot
#endif

//=============================================================================
//
// Laser Dot
//
class CLaserDot : public CSniperDot
{
public:
	DECLARE_CLASS( CLaserDot, CSniperDot );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	// Creation/Destruction.
	CLaserDot( void );
	~CLaserDot( void );

	static CLaserDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );
#ifdef CLIENT_DLL
	virtual int DrawModel( int flags );
	virtual bool ShouldDraw( void ) { return false; }
#endif
};

//=============================================================================
//
// Laser Pointer
//
class CTFLaserPointer : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFLaserPointer, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFLaserPointer();
	~CTFLaserPointer();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_LASER_POINTER; }

	virtual void Precache();
	virtual bool Deploy( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void ItemPostFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );

	virtual void WeaponIdle( void );

	virtual bool UsesClipsForAmmo1( void ) const { return false; }
	virtual bool UsesClipsForAmmo2( void ) const { return false; }

#ifdef GAME_DLL
	CLaserDot*	GetLaserDot() { return m_hLaserDot.Get(); }
	bool		HasLaserDot() { return GetLaserDot()?true:false; }
#endif

private:

	void CreateLaserDot( void );
	void DestroyLaserDot( void );
	void UpdateLaserDot( void );

private:

#ifdef GAME_DLL
	CHandle<CLaserDot>		m_hLaserDot;
#endif

	float					m_flNextAttack;
	float					m_flStartedFiring;
	bool					m_bDoHandIdle;

	bool					m_bDeployed;

	CTFLaserPointer( const CTFLaserPointer & );
};

#endif // TF_WEAPON_LASER_POINTER_H
