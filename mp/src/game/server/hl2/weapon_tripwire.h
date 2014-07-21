//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		TRIPWIRE 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONTRIPWIRE_H
#define	WEAPONTRIPWIRE_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"

enum TripwireState_t
{
	TRIPWIRE_TRIPMINE_READY,
	TRIPWIRE_SATCHEL_THROW,
	TRIPWIRE_SATCHEL_ATTACH,
};

class CWeapon_Tripwire : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_Tripwire, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	bool				m_bNeedReload;
	bool				m_bClearReload;
	bool				m_bAttachTripwire;

	void				Spawn( void );
	void				Precache( void );

	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				WeaponIdle( void );
	void				WeaponSwitch( void );
	
	void				SetPickupTouch( void );
	void				TripwireTouch( CBaseEntity *pOther );	// default weapon touch
	void				ItemPostFrame( void );	
	bool				Reload( void );
	bool				CanAttachTripwire(void);		// In position where can attach TRIPWIRE?
	void				StartTripwireAttach( void );
	void				TripwireAttach( void );

	bool				Deploy( void );
	bool				Holster( CBaseCombatWeapon *pSwitchingTo = NULL );


	CWeapon_Tripwire();

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};


#endif	//WEAPONTRIPWIRE_H
