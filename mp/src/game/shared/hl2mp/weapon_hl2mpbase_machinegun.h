//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifdef SecobMod__PLAYER_MOVEMENT_CAMERA_BOB
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
#else
	#include "weapon_hl2mpbase.h"
#endif //SecobMod__PLAYER_MOVEMENT_CAMERA_BOB

#ifndef BASEHLCOMBATWEAPON_H
#define BASEHLCOMBATWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
	#define CHL2MPMachineGun C_HL2MPMachineGun
#endif

//=========================================================
// Machine gun base class
//=========================================================
#ifdef SecobMod__PLAYER_MOVEMENT_CAMERA_BOB
	class CHL2MPMachineGun : public CBaseHL2MPCombatWeapon
#else
	class CHL2MPMachineGun : public CWeaponHL2MPBase
#endif //SecobMod__PLAYER_MOVEMENT_CAMERA_BOB
{
public:
	DECLARE_CLASS( CHL2MPMachineGun, CWeaponHL2MPBase );
	DECLARE_DATADESC();

	CHL2MPMachineGun();
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	PrimaryAttack( void );

	// Default calls through to m_hOwner, but plasma weapons can override and shoot projectiles here.
	virtual void	ItemPostFrame( void );
	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual bool	Deploy( void );

	virtual const Vector &GetBulletSpread( void );

	int				WeaponSoundRealtime( WeaponSound_t shoot_type );

	// utility function
	static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

private:
	
	CHL2MPMachineGun( const CHL2MPMachineGun & );

protected:

	int	m_nShotsFired;	// Number of consecutive shots fired

	float	m_flNextSoundTime;	// real-time clock of when to make next sound
};

#endif // BASEHLCOMBATWEAPON_H
