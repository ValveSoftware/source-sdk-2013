//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SWORD_H
#define TF_WEAPON_SWORD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#ifdef GAME_DLL
#include "GameEventListener.h"
#else
#include "tf_viewmodel.h"
#include "bone_setup.h"
#include "tf_wearable_weapons.h"
#endif

#ifdef CLIENT_DLL
#define CTFSword C_TFSword
#define CTFKatana C_TFKatana
#endif

//=============================================================================
//
// Decapitation melee weapon base class.
//

#ifdef GAME_DLL
class CTFDecapitationMeleeWeaponBase : public CTFWeaponBaseMelee, public CGameEventListener
#else
class CTFDecapitationMeleeWeaponBase : public CTFWeaponBaseMelee
#endif
{
public:

	DECLARE_CLASS( CTFDecapitationMeleeWeaponBase, CTFWeaponBaseMelee );
	
	CTFDecapitationMeleeWeaponBase();

	virtual void		Precache( void );
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SWORD; }
	virtual int			GetCustomDamageType() const			{ return TF_DMG_CUSTOM_DECAPITATION; }
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage );
	virtual Activity	TranslateViewmodelHandActivityInternal( Activity actBase );

	virtual bool		CanDecapitate( void );
	virtual void		SetupGameEventListeners( void );
	virtual void		OnDecapitation( CTFPlayer *pDeadPlayer ) {}

#ifdef GAME_DLL
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void		FireGameEvent( IGameEvent *event );
#endif

#ifdef CLIENT_DLL
	virtual void		UpdateAttachmentModels( void );
	virtual int			DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );
#endif // CLIENT_DLL

protected:
	bool				m_bHolstering;

private:
#ifdef CLIENT_DLL
	EHANDLE				m_hShield;
#endif // CLIENT_DLL
};

//=============================================================================
//
// Sword class. (Consumes heads for speed/health bonus.)
//
class CTFSword : public CTFDecapitationMeleeWeaponBase
{
public:

	DECLARE_CLASS( CTFSword, CTFDecapitationMeleeWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSword() {}
	virtual ~CTFSword() {}

	virtual void		WeaponReset( void );
	virtual float		GetSwordSpeedMod( void );
	virtual int			GetSwordHealthMod();
	virtual int			GetSwingRange( void );
	virtual void		OnDecapitation( CTFPlayer *pDeadPlayer );
	
	virtual bool		Deploy( void );

	float				GetProgress( void ) { return 0.f; }
	int					GetCount( void );
	const char*			GetEffectLabelText( void ) { return "#TF_BERZERK"; }

#ifdef CLIENT_DLL
	virtual void		WeaponIdle( void );
#endif

private:

	CTFSword( const CTFSword& ) {}

#ifdef CLIENT_DLL
	float				m_flNextIdleWavRoll;
	int					m_iPrevWavDecap;
#endif
};

//=============================================================================
//
// Sword class. (On head-take causes short-term soldier buff effects.)
//
class CTFKatana : public CTFDecapitationMeleeWeaponBase
{
public:

	DECLARE_CLASS( CTFKatana, CTFDecapitationMeleeWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKatana();
	virtual ~CTFKatana() {}

	virtual bool	Deploy( void );
	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage );
	virtual void	OnDecapitation( CTFPlayer *pDeadPlayer );

	virtual int		GetActivityWeaponRole() const OVERRIDE;

protected:
	virtual int		GetSkinOverride() const;
	
private:
	CNetworkVar( bool, m_bIsBloody );
};


#endif // TF_WEAPON_SWORD_H
