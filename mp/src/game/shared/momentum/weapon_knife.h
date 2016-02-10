//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_KNIFE_H
#define WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CKnife C_Knife

#endif


// ----------------------------------------------------------------------------- //
// CKnife class definition.
// ----------------------------------------------------------------------------- //

class CKnife : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CKnife, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	#ifndef CLIENT_DLL
		DECLARE_DATADESC();
	#endif

	
	CKnife();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool HasPrimaryAmmo();
	virtual bool CanBeSelected();
	
	virtual void Precache();

	void Spawn();
	void Smack();
	//void Smack( trace_t *pTr, float delay );
	bool SwingOrStab( bool bStab );
	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponAnimation( int iAnimation );

	virtual void ItemPostFrame( void );


	bool Deploy();
	void Holster( int skiplocal = 0 );
	bool CanDrop();

	void WeaponIdle();

	virtual CSWeaponID GetWeaponID( void ) const		{ return WEAPON_KNIFE; }

public:
	
	trace_t m_trHit;
	EHANDLE m_pTraceHitEnt;

	CNetworkVar( float, m_flSmackTime );
	bool	m_bStab;

private:
	CKnife( const CKnife & ) {}
};


#endif // WEAPON_KNIFE_H
