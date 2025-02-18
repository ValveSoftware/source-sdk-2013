//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_DRAGONS_FURY_H
#define TF_WEAPON_DRAGONS_FURY_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_flamethrower.h"

// Client specific.
#ifdef CLIENT_DLL

	#define CTFWeaponFlameBall C_TFWeaponFlameBall
#else
	#include "tf_projectile_rocket.h"
	#include "baseentity.h"
	#include "iscorer.h"
#endif



class CTFWeaponFlameBall : public CTFFlameThrower
{
public:
	DECLARE_CLASS( CTFWeaponFlameBall, CTFFlameThrower );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFWeaponFlameBall();

#ifndef CLIENT_DLL
	virtual void	Precache();
#endif

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAME_BALL; }

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );

	virtual void	PrimaryAttack( void ) OVERRIDE;
	virtual void	SecondaryAttack( void ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;

	bool HasFullCharge() const;

	virtual void	OnResourceMeterFilled() OVERRIDE;
	virtual float	GetMeterMultiplier() const OVERRIDE;

#ifdef GAME_DLL
	virtual float GetInitialAfterburnDuration() const OVERRIDE { return 0.f; }
	void RefundAmmo( int nAmmo );
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual bool	ShouldDrawMeter() const OVERRIDE;
	virtual void	GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );
	void			UpdatePoseParams();
#endif

private:

#ifdef GAME_DLL
	void StartPressureSound();
	void StopPressureSound();
#endif

	CNetworkVar( float, m_flRechargeScale );

	int m_nNeedlePoseParam = -1;
	int m_nBarrelPoseParam = -1;

#ifdef GAME_DLL
	CSoundPatch*	m_pSndPressure;
#endif
};


#endif // TF_WEAPON_FLAMETHROWER_H
