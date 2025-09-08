//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_WRENCH_H
#define TF_WEAPON_WRENCH_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_item_wearable.h"

#ifdef CLIENT_DLL
#define CTFWrench C_TFWrench
#define CTFRobotArm C_TFRobotArm
#define CTFWearableRobotArm C_TFWearableRobotArm
#endif

//=============================================================================
//
// Wrench class.
//
class CTFWrench : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFWrench, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFWrench();

	virtual void		Spawn();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_WRENCH; }
	virtual void		Smack( void );

	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );

	bool				IsPDQ( void ) { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, wrench_builds_minisentry ); return iMode==1; };
	float				GetConstructionValue( void );
	float				GetRepairAmount( void );
#ifdef GAME_DLL
	virtual void		Equip( CBaseCombatCharacter *pOwner );
	virtual void		Detach();

	void				ApplyBuildingHealthUpgrade( void );

	void				OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector hitLoc );
#else
	virtual void		ItemPostFrame();
#endif


private:
	bool				m_bReloadDown;
	CTFWrench( const CTFWrench & ) {}
};

//=============================================================================
//
// Robot Arm class.
//
class CTFRobotArm : public CTFWrench
{
public:
	DECLARE_CLASS( CTFRobotArm, CTFWrench );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFRobotArm();

	virtual void		Precache();

#ifdef GAME_DLL
	virtual void		Equip( CBaseCombatCharacter *pOwner );
	virtual void		Drop( const Vector &vecVelocity );
	virtual void		UpdateOnRemove( void );
	void				RemoveRobotArm();
	virtual void		OnActiveStateChanged( int iOldState );
	virtual int			GetDamageCustom();
	virtual float		GetForceScale( void );
	virtual bool 		HideAttachmentsAndShowBodygroupsWhenPerformingWeaponIndependentTaunt() const OVERRIDE { return false; }
#endif

	virtual void		PrimaryAttack();

	virtual void		Smack( void );
	virtual void		WeaponIdle( void );

	virtual void		DoViewModelAnimation( void );

private:
	CNetworkHandle( CTFWearable, m_hRobotArm );

	int					m_iComboCount;
	float				m_flLastComboHit;
	bool				m_bBigIdle;
	bool				m_bBigHit;
};

class CTFWearableRobotArm : public CTFWearable
{
public:
	DECLARE_CLASS( CTFWearableRobotArm, CTFWearable );
	DECLARE_NETWORKCLASS();
};

#endif // TF_WEAPON_WRENCH_H
