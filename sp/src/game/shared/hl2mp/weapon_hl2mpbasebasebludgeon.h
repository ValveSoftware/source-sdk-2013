//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		The class from which all bludgeon melee
//				weapons are derived. 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifndef BASEBLUDGEONWEAPON_H
#define BASEBLUDGEONWEAPON_H

#ifdef _WIN32
#pragma once
#endif


#if defined( CLIENT_DLL )
#define CBaseHL2MPBludgeonWeapon C_BaseHL2MPBludgeonWeapon
#endif

//=========================================================
// CBaseHLBludgeonWeapon 
//=========================================================
class CBaseHL2MPBludgeonWeapon : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CBaseHL2MPBludgeonWeapon, CBaseHL2MPCombatWeapon );
public:
	CBaseHL2MPBludgeonWeapon();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual	void	Spawn( void );
	virtual	void	Precache( void );
	
	//Attack functions
	virtual	void	PrimaryAttack( void );
	virtual	void	SecondaryAttack( void );
	
	virtual void	ItemPostFrame( void );

	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity( void )	{	return	ACT_VM_HITCENTER;	}
	virtual Activity	GetSecondaryAttackActivity( void )	{	return	ACT_VM_HITCENTER2;	}

	virtual	float	GetFireRate( void )								{	return	0.2f;	}
	virtual float	GetRange( void )								{	return	32.0f;	}
	virtual	float	GetDamageForActivity( Activity hitActivity )	{	return	1.0f;	}

	CBaseHL2MPBludgeonWeapon( const CBaseHL2MPBludgeonWeapon & );

protected:
	virtual	void	ImpactEffect( trace_t &trace );

private:
	bool			ImpactWater( const Vector &start, const Vector &end );
	void			Swing( int bIsSecondary );
	void			Hit( trace_t &traceHit, Activity nHitActivity );
	Activity		ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );
};

#endif