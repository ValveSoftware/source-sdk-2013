//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_WEAPON_BOTTLE_H
#define TF_WEAPON_BOTTLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBreakableMelee C_TFBreakableMelee
#define CTFBottle C_TFBottle
#define CTFBreakableSign C_TFBreakableSign
#define CTFStickBomb C_TFStickBomb
#endif

class CTFBreakableMelee : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBreakableMelee, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	CTFBreakableMelee();

	virtual void		Smack( void ) OVERRIDE;
	virtual void		WeaponReset( void ) OVERRIDE;
	virtual bool		DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt ) OVERRIDE;

	virtual void		SwitchBodyGroups( void );

	virtual bool		UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState ) OVERRIDE;

	virtual bool		IsBroken( void ) const OVERRIDE { return m_bBroken; }
	virtual void		SetBroken( bool bBroken ) OVERRIDE;

#ifdef CLIENT_DLL
	static void RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

protected:

	CNetworkVar( bool,	m_bBroken  );
};

//=============================================================================
//
// Bottle class.
//
class CTFBottle : public CTFBreakableMelee
{
public:

	DECLARE_CLASS( CTFBottle, CTFBreakableMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	virtual int			GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_BOTTLE; }
};

//=============================================================================
//
// Breakable Sign class.
//
class CTFBreakableSign : public CTFBreakableMelee
{
public:

	DECLARE_CLASS( CTFBreakableSign, CTFBreakableMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	virtual int			GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_BREAKABLE_SIGN; }
};

//=============================================================================
//
// StickBomb class.
//
class CTFStickBomb : public CTFBreakableMelee
{
public:

	DECLARE_CLASS( CTFStickBomb, CTFBreakableMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	CTFStickBomb();

	virtual void		Precache( void ) OVERRIDE;
	virtual int			GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_STICKBOMB; }
	virtual void		Smack( void ) OVERRIDE;
	virtual void		WeaponReset( void ) OVERRIDE;
	virtual void		WeaponRegenerate( void ) OVERRIDE;
	virtual void		SwitchBodyGroups( void ) OVERRIDE;
	virtual const char*	GetWorldModel( void ) const OVERRIDE;
#ifdef CLIENT_DLL
	virtual int			GetWorldModelIndex( void ) OVERRIDE;
#endif

	void				SetDetonated( int iVal ) { m_iDetonated = iVal; }
	int					GetDetonated( void ) { return m_iDetonated; }

private:

	CNetworkVar( int,	m_iDetonated ); // int, not bool so we can use a recv proxy
};

#endif // TF_WEAPON_BOTTLE_H
