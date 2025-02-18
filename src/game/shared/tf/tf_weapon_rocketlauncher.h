//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TF_WEAPON_ROCKETLAUNCHER_H
#define TF_WEAPON_ROCKETLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_sniperrifle.h"

#include "tf_flame.h"

#ifdef CLIENT_DLL
#include "particle_property.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFRocketLauncher C_TFRocketLauncher
#define CTFRocketLauncher_DirectHit C_TFRocketLauncher_DirectHit
#define CTFRocketLauncher_AirStrike C_TFRocketLauncher_AirStrike
#define CTFRocketLauncher_Mortar C_TFRocketLauncher_Mortar
#define CTFCrossbow C_TFCrossbow
#endif // CLIENT_DLL

//=============================================================================
//
// TF Weapon Rocket Launcher.
//
class CTFRocketLauncher : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFRocketLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFRocketLauncher();
	~CTFRocketLauncher();

#ifndef CLIENT_DLL
	virtual void	Precache();
#endif
	virtual void	ModifyEmitSoundParams( EmitSound_t &params );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }

	virtual void	Misfire( void );
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemPostFrame( void );
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

	virtual int		GetWeaponProjectileType( void ) const OVERRIDE;

	virtual bool	IsBlastImpactWeapon( void ) const { return !IsEnergyWeapon(); }

	virtual bool	CheckReloadMisfire( void ) OVERRIDE;

	virtual bool	ShouldBlockPrimaryFire() OVERRIDE;

#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
#endif

	virtual bool	CanInspect() const OVERRIDE;

private:
	float	m_flShowReloadHintAt;

	// Since the ammo in the clip can be predicted/networked out of order from when the reload sound happens
	// We need to keep track of this invividually on client and server to modify the pitch
	int		m_nReloadPitchStep;

#ifdef GAME_DLL
	int		m_iConsecutiveCrits;
	bool	m_bIsOverloading;
#endif

	CTFRocketLauncher( const CTFRocketLauncher & ) {}
};

// ------------------------------------------------------------------------------------------------------------------------
class CTFRocketLauncher_DirectHit : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFRocketLauncher_DirectHit, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT; }
};

// ------------------------------------------------------------------------------------------------------------------------
class CTFRocketLauncher_AirStrike : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFRocketLauncher_AirStrike, CTFRocketLauncher );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFRocketLauncher_AirStrike();

	virtual int		GetWeaponID( void ) const		{ return TF_WEAPON_ROCKETLAUNCHER; }
	const char*		GetEffectLabelText( void )		{ return "#TF_KILLS"; }
	virtual int		GetCount( void );

#ifdef GAME_DLL
	virtual void	OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info );
#endif
};

// ------------------------------------------------------------------------------------------------------------------------
class CTFRocketLauncher_Mortar : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFRocketLauncher_Mortar, CTFRocketLauncher );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	//CTFRocketLauncher_Mortar();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );

	virtual void	SecondaryAttack( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );

private:
	
	void			RedirectRockets();

#ifdef GAME_DLL
	CUtlVector< EHANDLE > m_vecRockets;
#endif // GAME_DLL

};

// ------------------------------------------------------------------------------------------------------------------------
class CTFCrossbow : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFCrossbow, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_CROSSBOW; }
	virtual void	SecondaryAttack( void );
	virtual float	GetProjectileSpeed( void );
	virtual float	GetProjectileGravity( void );
	virtual bool	IsViewModelFlipped( void );

	virtual void	ItemPostFrame( void );
	virtual void	ModifyProjectile( CBaseEntity* pProj );
	virtual void	WeaponRegenerate( void );

	float				GetProgress( void );
	const char*			GetEffectLabelText( void )					{ return "#TF_BOLT"; }

	CNetworkVar( float, m_flRegenerateDuration );
	CNetworkVar( float, m_flLastUsedTimestamp );

private:
	bool m_bMilkNextAttack;
};

#endif // TF_WEAPON_ROCKETLAUNCHER_H
