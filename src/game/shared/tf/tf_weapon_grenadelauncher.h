//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_GRENADELAUNCHER_H
#define TF_WEAPON_GRENADELAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeLauncher	C_TFGrenadeLauncher
#define CTFCannon			C_TFCannon
#endif

#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFGrenadeLauncher();
	~CTFGrenadeLauncher();

	virtual void	Spawn( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADELAUNCHER; }
	virtual void	SecondaryAttack();

	virtual void FireFullClipAtOnce( void );
	//virtual CBaseEntity* FirePipeBomb( CTFPlayer *pPlayer, int iPipeBombType );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	PrimaryAttack( void );
	virtual void	ItemPostFrame( void );
	virtual void	Misfire( void );
	virtual void	WeaponIdle( void );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual void	WeaponReset( void );
	virtual float	GetProjectileSpeed( void );

	int			  GetDetonateMode( void ) const;

	virtual bool	Reload( void );

	virtual int		GetMaxClip1( void ) const;
	virtual int		GetDefaultClip1( void ) const;

	virtual bool	IsBlastImpactWeapon( void ) const { return true; }

	// ITFChargeUpWeapon
	virtual bool CanCharge( void );
	virtual float GetChargeBeginTime( void );
	virtual float GetChargeMaxTime( void );

	void LaunchGrenade( void );

	void AddDonkVictim( const CBaseEntity* pVictim );
	bool IsDoubleDonk( const CBaseEntity* pVictim ) const;

private:

	CTFGrenadeLauncher( const CTFGrenadeLauncher & ) {}
	int m_nLauncherSlot;

	void FireProjectileInternal( CTFPlayer* pTFPlayer );
	void PostFire();

	void ResetDetonateTime();
	float GetMortarDetonateTimeLength();

	CNetworkVar( float, m_flDetonateTime );

	// Barrel rotation needs to be in sync
	CNetworkVar( int, m_iCurrentTube );	// Which tube is the one we just fired out of
	CNetworkVar( int, m_iGoalTube );	// Which tube is the one we would like to fire out of next?

	int		m_iBarrelBone;
	float	m_flBarrelRotateBeginTime;	// What time did we begin the animation to rotate to the next barrel?
	float	m_flBarrelAngle;	// What is the current rotation of the barrel?
	bool	m_bCurrentAndGoalTubeEqual;


#ifdef CLIENT_DLL
	void StartChargeEffects();
	void StopChargeEffects();

	CNewParticleEffect			*m_pCannonFuseSparkEffect;
	CNewParticleEffect			*m_pCannonCharge;

	// Barrel spinning (cribbed from Minigun)
	virtual CStudioHdr *OnNewModel( void );
	virtual void		StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	virtual	void		OnDataChanged( DataUpdateType_t type );

	void				UpdateBarrelMovement( void );

	virtual void		ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
#endif // CLIENT_DLL

	virtual void		ItemPreFrame( void );

	struct Donks_t
	{
		CHandle <CBaseEntity> m_hVictim;
		float m_flExpireTime;
	};

	CUtlVector< Donks_t > m_vecDonkVictims;

};


class CTFCannon : public CTFGrenadeLauncher
{
public:
	DECLARE_CLASS( CTFCannon, CTFGrenadeLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_CANNON; }
};

#endif // TF_WEAPON_GRENADELAUNCHER_H
