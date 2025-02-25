//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_COMPOUND_BOW_H
#define TF_WEAPON_COMPOUND_BOW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_pipebomblauncher.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFCompoundBow C_TFCompoundBow
#else
class CTFProjectile_Arrow;
#endif

//=============================================================================
//
// TF Weapon Bow
//
class CTFCompoundBow : public CTFPipebombLauncher
{
public:

	DECLARE_CLASS( CTFCompoundBow, CTFPipebombLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFCompoundBow();
	~CTFCompoundBow() {}

	virtual void	Precache( void );

	virtual void	WeaponReset( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_COMPOUND_BOW; }

	virtual void	PrimaryAttack();
	virtual void	LaunchGrenade( void );

	virtual bool	CalcIsAttackCriticalHelper();

	virtual float	GetChargeMaxTime( void );
	virtual float	GetCurrentCharge( void );
	virtual float	GetProjectileDamage( void );
	virtual float	GetProjectileSpeed( void );
	virtual float	GetProjectileGravity( void );

	virtual void	AddPipeBomb( CTFGrenadePipebombProjectile *pBomb );
	virtual bool	DetonateRemotePipebombs( bool bFizzle );
	virtual void	SecondaryAttack( void );
	virtual void	LowerBow( void );

	virtual bool	Reload( void );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual bool	SendWeaponAnim( int iActivity );
	virtual void	ItemPostFrame( void );

	virtual float	GetChargeForceReleaseTime( void ) { return 5.0f; }
	virtual void	ForceLaunchGrenade( void );
	virtual bool	ShouldDoMuzzleFlash( void ) { return false; }

	virtual void	GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f );
	void			ApplyRefireSpeedModifications( float &flBaseRef );

	// The bow doesn't actually reload, it instead uses the AE_WPN_INCREMENTAMMO anim event in the fire to reload the clip.
	virtual bool	CanReload( void ){ return false; }

	virtual bool	CanPickupOtherWeapon() const { return GetInternalChargeBeginTime() == 0.f; }
	
#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	UpdateOnRemove( void );
#else
	virtual float	GetInitialAfterburnDuration() const OVERRIDE;
#endif

	void			SetArrowAlight( bool bAlight );

	bool			OwnerCanJump( void );

private:
#ifdef CLIENT_DLL
	virtual void	StartBurningEffect( void );
	virtual void	StopBurningEffect( void );
#else


#endif

	float		m_flLastDenySoundTime;
	CNetworkVar( bool, m_bNoFire );
	CNetworkVar( bool, m_bArrowAlight );

#ifdef CLIENT_DLL
	EHANDLE		   m_hParticleEffectOwner;
	HPARTICLEFFECT m_pBurningArrowEffect;
#endif 

	CTFCompoundBow( const CTFCompoundBow & ) {}
};

#endif // TF_WEAPON_COMPOUND_BOW_H
