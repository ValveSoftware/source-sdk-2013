//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_REVOLVER_H
#define TF_WEAPON_REVOLVER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRevolver C_TFRevolver
#define CTFRevolver_Secondary C_TFRevolver_Secondary
#endif

//=============================================================================
//
// TF Weapon Revolver.
//
class CTFRevolver : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFRevolver, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFRevolver();
	~CTFRevolver() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_REVOLVER; }
	virtual int		GetDamageType( void ) const;

	virtual bool	CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget = NULL ) OVERRIDE;

	virtual void	PrimaryAttack( void );
	virtual	float	GetWeaponSpread( void );

	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

	bool			CanHeadshot( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return (iMode == 1); };

	bool			SapperKillsCollectCrits( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, sapper_kills_collect_crits ); return (iMode == 1); };

	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool		Deploy( void );
	
	int					GetCount( void );
	const char*			GetEffectLabelText( void );
	float				GetProgress( void ) { return 0.f; }

#ifdef CLIENT_DLL
	virtual void	GetWeaponCrosshairScale( float &flScale );
#else
	virtual void	Detach();
	virtual float	GetProjectileDamage( void );
#endif

private:

	CTFRevolver( const CTFRevolver & ) {}

	float			m_flLastAccuracyCheck;
	float			m_flAccuracyCheckTime;
};


//=============================================================================
// Secondary Revolver (Engy)
class CTFRevolver_Secondary : public CTFRevolver
{
public:
	DECLARE_CLASS( CTFRevolver_Secondary, CTFRevolver );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

#endif // TF_WEAPON_REVOLVER_H
