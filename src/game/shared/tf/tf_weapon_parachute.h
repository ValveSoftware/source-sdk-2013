//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_PARACHUTE_H
#define TF_WEAPON_PARACHUTE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_buff_item.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFParachute C_TFParachute
#define CTFParachute_Primary C_TFParachute_Primary
#define CTFParachute_Secondary C_TFParachute_Secondary
#endif

//=============================================================================
//
// Parachute
//
class CTFParachute : public CTFBuffItem
{
public:

	DECLARE_CLASS( CTFParachute, CTFBuffItem );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFParachute();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PARACHUTE; }

	virtual bool	VisibleInWeaponSelection( void )				{ return false; }
	virtual bool	CanBeSelected( void )							{ return false; }
	virtual bool	CanDeploy( void )								{ return false; }
	virtual void	CreateBanner();

#ifdef CLIENT_DLL
	virtual void	ClientThink( void );
	void			ParachuteAnimThink( void );
#endif // CLIENT_DLL

private:
#ifdef CLIENT_DLL
	int							m_iParachuteAnimState;
	float						m_flParachuteToIdleTime;
#endif // CLIENT_DLL
};

//=============================================================================
// Parachute Primary (demo)
class CTFParachute_Primary : public CTFParachute
{
public:
	DECLARE_CLASS( CTFParachute_Primary, CTFParachute );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};
//=============================================================================
// Parachute Secondary (soldier)
class CTFParachute_Secondary : public CTFParachute
{
public:
	DECLARE_CLASS( CTFParachute_Secondary, CTFParachute );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};
#endif // TF_WEAPON_BUFF_ITEM_H
