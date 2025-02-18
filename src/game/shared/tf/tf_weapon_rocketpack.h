//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_ROCKETPACK_H
#define TF_WEAPON_ROCKETPACK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_shareddefs.h"
#include "tf_viewmodel.h"

#ifdef CLIENT_DLL
#define CTFRocketPack C_TFRocketPack
#endif

class CTFRocketPack : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFRocketPack, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRocketPack();

	virtual void		WeaponReset( void ) OVERRIDE;
	virtual void		Precache( void ) OVERRIDE;
	virtual int			GetWeaponID( void ) const { return TF_WEAPON_ROCKETPACK; }
	virtual void		ItemPostFrame( void ) OVERRIDE;
	virtual void		PrimaryAttack( void ) OVERRIDE {}
	virtual void		SecondaryAttack( void ) OVERRIDE {}
	virtual bool		Deploy( void ) OVERRIDE;
	virtual void		StartHolsterAnim( void ) OVERRIDE;
	virtual bool		CanDeploy( void ) OVERRIDE { return true; }
	virtual bool		CanHolster( void ) const OVERRIDE;
	virtual bool		VisibleInWeaponSelection( void ) { return true; }
	virtual const CEconItemView *GetTauntItem() const OVERRIDE;
	virtual bool		CanInspect() const OVERRIDE { return false; }

	// IHasGenericMeter
	virtual float		GetChargeInterval() const OVERRIDE { return 50.f; }
	virtual void		OnResourceMeterFilled() OVERRIDE;

	bool				IsEnabled( void ) const { return m_bEnabled; }
	bool				CanFire( void ) const;
	bool				InitiateLaunch( void );
	bool				PreLaunch( void );
	bool				Launch( void );


#ifdef CLIENT_DLL
	virtual void		OnPreDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void		OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void		UpdateOnRemove( void ) OVERRIDE;
	virtual void		FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual bool		ShouldDraw() OVERRIDE;
#endif // CLIENT_DLL

	float				GetRefireTime( void ) { return m_flRefireTime; }

private:
	CTFRocketPack( const CTFRocketPack & ) {}

	void				ResetTransition( void );
	void				StartTransition( void );
	bool				IsInTransition( void ) const;
	bool				IsTransitionCompleted( void ) const;
	void				WaitToLaunch( void );

#ifdef GAME_DLL
	void				SetEnabled( bool bEnabled );
	void				PassengerDelayLaunchThink( void );
	void				RocketLaunchPlayer( CTFPlayer *pPlayer, const Vector& vecForce, bool bIsPassenger );
	Vector				CalcRocketForceFromPlayer( CTFPlayer *pPlayer );
#else
	void				CleanupParticles( void );
#endif // GAME_DLL

	CNetworkVar( float, m_flInitLaunchTime );
	CNetworkVar( float, m_flLaunchTime );
	CNetworkVar( float, m_flToggleEndTime );
	CNetworkVar( bool, m_bPassengerHookDeployed );
	CNetworkVar( bool, m_bEnabled );
	float				m_flRefireTime;
	bool				m_bLaunchedFromGround;

#ifdef GAME_DLL
	Vector				m_vecLaunchDir;
#else
	HPARTICLEFFECT		m_hLeftBlast;
	HPARTICLEFFECT		m_hRightBlast;
	HPARTICLEFFECT		m_hLeftTrail;
	HPARTICLEFFECT		m_hRightTrail;
	float				m_flOldInitLaunchTime;
	bool				m_bWasEnabled;
	bool				m_bWasPassengerHookDeployed;
	CHandle< C_TFPlayer > m_hOldPassenger;
	CHandle< C_RopeKeyframe > m_hRope;
#endif // CLIENT_DLL
};

#endif // TF_WEAPON_ROCKETPACK_H
