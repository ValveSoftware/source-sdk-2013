//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_LUNCHBOX_H
#define TF_WEAPON_LUNCHBOX_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFLunchBox C_TFLunchBox
#define CTFLunchBox_Drink C_TFLunchBox_Drink
#endif

enum lunchbox_weapontypes_t
{
	LUNCHBOX_STANDARD = 0,		// Careful, can be the Scout BONK drink, or the Heavy sandvich.
	LUNCHBOX_CHOCOLATE_BAR,
	LUNCHBOX_ADDS_MINICRITS,
	LUNCHBOX_STANDARD_ROBO,
	LUNCHBOX_STANDARD_FESTIVE,
	LUNCHBOX_ADDS_AMMO,
	LUNCHBOX_BANANA,
	LUNCHBOX_FISHCAKE,
};

#define TF_SANDWICH_REGENTIME	30
#define TF_CHOCOLATE_BAR_REGENTIME	10

//=============================================================================
//
// TF Weapon Lunchbox.
//
class CTFLunchBox : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFLunchBox, CTFWeaponBase );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFLunchBox();

	virtual void	UpdateOnRemove( void ) OVERRIDE;
	virtual void	Precache() OVERRIDE;
	virtual int		GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_LUNCHBOX; }
	virtual void	PrimaryAttack() OVERRIDE;
	virtual void	SecondaryAttack() OVERRIDE;
	virtual void	WeaponReset( void ) OVERRIDE;
	virtual bool	UsesPrimaryAmmo() OVERRIDE;

	virtual bool	DropAllowed( void );
	int				GetLunchboxType( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };

	void			DrainAmmo( bool bForceCooldown = false );

	virtual void	Detach( void ) OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;

#ifdef GAME_DLL
	void			ApplyBiteEffects( CTFPlayer *pPlayer );
	virtual void	OnResourceMeterFilled() OVERRIDE;
#endif

	virtual void		SwitchBodyGroups( void );
	virtual bool		UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState ) OVERRIDE;
	virtual bool		IsBroken( void ) const OVERRIDE { return m_bBroken; }
	virtual void		SetBroken( bool bBroken ) OVERRIDE;

#ifdef CLIENT_DLL
	static void RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

private:
	CTFLunchBox( const CTFLunchBox & ) {}

	// Prevent spamming with resupply cabinets: only 1 thrown at a time
	EHANDLE		m_hThrownPowerup;
	
	CNetworkVar( bool,	m_bBroken  );
};

//=============================================================================
//
// TF Weapon Energy drink.
//
class CTFLunchBox_Drink : public CTFLunchBox
{
public:

	DECLARE_CLASS( CTFLunchBox_Drink, CTFLunchBox );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFLunchBox_Drink();

	bool				AllowTaunts( void ) { return false; }

	virtual bool		DropAllowed( void ) { return false; }

	const char*			GetEffectLabelText( void )	{ return "#TF_ENERGYDRINK"; }
	float				GetProgress( void ) { return GetEffectBarProgress(); }
	virtual float		InternalGetEffectBarRechargeTime( void ) { return TF_SANDWICH_REGENTIME; }

#ifdef CLIENT_DLL
	virtual const char* ModifyEventParticles( const char* token );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
#endif
};

#endif // TF_WEAPON_LUNCHBOX_H
