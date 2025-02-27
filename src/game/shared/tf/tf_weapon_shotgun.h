//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#define CTFScatterGun C_TFScatterGun
#define CTFShotgun_Revenge C_TFShotgun_Revenge
#define CTFSodaPopper C_TFSodaPopper
#define CTFPEPBrawlerBlaster C_TFPEPBrawlerBlaster
#define CTFShotgunBuildingRescue C_TFShotgunBuildingRescue
#define CTFLeech C_TFLeech
#endif

// Reload Modes
enum
{
	TF_WEAPON_SHOTGUN_RELOAD_START = 0,
	TF_WEAPON_SHOTGUN_RELOAD_SHELL,
	TF_WEAPON_SHOTGUN_RELOAD_CONTINUE,
	TF_WEAPON_SHOTGUN_RELOAD_FINISH
};

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PRIMARY; }
	virtual void	PrimaryAttack();
	virtual void	PlayWeaponShootSound( void );

#ifdef GAME_DLL
	virtual CDmgAccumulator	*GetDmgAccumulator( void ) { return &m_Accumulator; }
#endif // GAME_DLL
	
protected:

	void		Fire( CTFPlayer *pPlayer );
	void		UpdatePunchAngles( CTFPlayer *pPlayer );

private:

	CTFShotgun( const CTFShotgun & ) {}

#ifdef GAME_DLL
	CDmgAccumulator m_Accumulator;
#endif // GAME_DLL
};

// Engineer's revenge shotgun.
class CTFShotgun_Revenge : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Revenge, CTFShotgun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFShotgun_Revenge();

	virtual int			GetWeaponID( void ) const		{ return TF_WEAPON_SENTRY_REVENGE; }

	void				Precache();
	virtual void		PrimaryAttack();
	virtual void		SentryKilled( int iKills );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool		Deploy( void );
	virtual int         GetCustomDamageType() const;
	int					GetCount( void );
	float				GetProgress( void ) { return 0.f; }
	const char*			GetEffectLabelText( void ) { return "#TF_REVENGE"; }

#ifdef CLIENT_DLL
	virtual void		SetWeaponVisible( bool visible );
	virtual int			GetWorldModelIndex( void );
#else
	virtual void		Detach();
#endif
};

// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
	virtual bool	Reload( void );
	virtual void	FireBullet( CTFPlayer *pPlayer );
	virtual void	ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer );
	virtual void	FinishReload( void );
	virtual bool	HasKnockback( void );

#ifdef GAME_DLL
	virtual void	Equip( CBaseCombatCharacter *pOwner );
#endif // GAME_DLL
};

class CTFShotgun_Soldier : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Soldier, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_SOLDIER; }
};

// Secondary version. Different weapon slot, different ammo
class CTFShotgun_HWG : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_HWG, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_HWG; }
};

class CTFShotgun_Pyro : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Pyro, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PYRO; }
};

class CTFSodaPopper : public CTFScatterGun
{
public:
	DECLARE_CLASS( CTFSodaPopper, CTFScatterGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SODA_POPPER; }

	virtual void	ItemBusyFrame( void );
	virtual void	SecondaryAttack( void );

	const char*		GetEffectLabelText( void )			{ return "#TF_HYPE"; }
	float			GetProgress( void );
};

class CTFPEPBrawlerBlaster : public CTFScatterGun
{
public:
	DECLARE_CLASS( CTFPEPBrawlerBlaster, CTFScatterGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PEP_BRAWLER_BLASTER; }

	const char*		GetEffectLabelText( void )			{ return "#TF_Boost"; }
	float			GetProgress( void );
};

class CTFShotgunBuildingRescue : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgunBuildingRescue, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_BUILDING_RESCUE; }
	virtual float	GetProjectileSpeed( void );
	virtual float   GetProjectileGravity( void );
	virtual bool	IsViewModelFlipped( void );
};

#endif // TF_WEAPON_SHOTGUN_H
