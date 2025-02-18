//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon Base Melee 
//
//=============================================================================

#ifndef TF_WEAPONBASE_MELEE_H
#define TF_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseMelee C_TFWeaponBaseMelee
extern ConVar cl_crosshair_file;
#endif

//=============================================================================
//
// Weapon Base Melee Class
//
class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseMelee, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseMelee();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }
	virtual void	Precache();
	virtual void	ItemPreFrame();
	virtual void	ItemPostFrame();
	virtual void	Spawn();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_NONE; }
	virtual int		GetSwingRange( void );
	virtual void	WeaponReset( void );
	virtual bool	CanHolster( void ) const;

	virtual bool	CalcIsAttackCriticalHelper( void );
	virtual bool	CalcIsAttackCriticalHelperNoCrits( void );

	virtual void	DoViewModelAnimation( void );

	virtual bool	DoSwingTrace( trace_t &trace );
	virtual void	Smack( void );
	virtual float	GetSmackTime( int iWeaponMode );
	virtual void	DoMeleeDamage( CBaseEntity* ent, trace_t& trace );
	virtual void	DoMeleeDamage( CBaseEntity* ent, trace_t& trace, float flDamageMod );

	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage );

#ifndef CLIENT_DLL
	virtual float	GetForceScale( void );
	virtual int		GetDamageCustom( void ) { return TF_DMG_CUSTOM_NONE; }
#endif

	// Call when we hit an entity. Use for special weapon effects on hit.
	virtual void	OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info );

	virtual void	SendPlayerAnimEvent( CTFPlayer *pPlayer );

	bool			ConnectedHit( void ) { return m_bConnected; }

	virtual char const		*GetShootSound( int iIndex ) const;

public:	

	CTFWeaponInfo	*m_pWeaponInfo;

protected:

	virtual void	Swing( CTFPlayer *pPlayer );
	virtual void	PlaySwingSound( void );

protected:

	float	m_flSmackTime;
	bool	m_bConnected;
	bool	m_bMiniCrit;

#ifdef GAME_DLL
	CUtlVector< CHandle< CTFPlayer > > m_potentialVictimVector;
#endif

private:
	bool DoSwingTraceInternal( trace_t &trace, bool bCleave, CUtlVector< trace_t >* pTargetTraceVector );
	bool OnSwingHit( trace_t &trace );

	CTFWeaponBaseMelee( const CTFWeaponBaseMelee & ) {}
};

#endif // TF_WEAPONBASE_MELEE_H
