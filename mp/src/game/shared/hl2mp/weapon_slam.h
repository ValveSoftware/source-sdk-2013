//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		SLAM 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONSLAM_H
#define	WEAPONSLAM_H

#include "basegrenade_shared.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

enum
{
	SLAM_TRIPMINE_READY,
	SLAM_SATCHEL_THROW,
	SLAM_SATCHEL_ATTACH,
};

#ifdef CLIENT_DLL
#define CWeapon_SLAM C_Weapon_SLAM
#endif

class CWeapon_SLAM : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_SLAM, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CNetworkVar( int,	m_tSlamState );
	CNetworkVar( bool,				m_bDetonatorArmed );
	CNetworkVar( bool,				m_bNeedDetonatorDraw);
	CNetworkVar( bool,				m_bNeedDetonatorHolster);
	CNetworkVar( bool,				m_bNeedReload);
	CNetworkVar( bool,				m_bClearReload);
	CNetworkVar( bool,				m_bThrowSatchel);
	CNetworkVar( bool,				m_bAttachSatchel);
	CNetworkVar( bool,				m_bAttachTripmine);
	float				m_flWallSwitchTime;

	void				Spawn( void );
	void				Precache( void );

	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				WeaponIdle( void );
	void				Weapon_Switch( void );
	void				SLAMThink( void );
	
	void				SetPickupTouch( void );
	void				SlamTouch( CBaseEntity *pOther );	// default weapon touch
	void				ItemPostFrame( void );	
	bool				Reload( void );
	void				SetSlamState( int newState );
	bool				CanAttachSLAM(void);		// In position where can attach SLAM?
	bool				AnyUndetonatedCharges(void);
	void				StartTripmineAttach( void );
	void				TripmineAttach( void );

	void				StartSatchelDetonate( void );
	void				SatchelDetonate( void );
	void				StartSatchelThrow( void );
	void				StartSatchelAttach( void );
	void				SatchelThrow( void );
	void				SatchelAttach( void );
	bool				Deploy( void );
	bool				Holster( CBaseCombatWeapon *pSwitchingTo = NULL );


	CWeapon_SLAM();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

private:
	CWeapon_SLAM( const CWeapon_SLAM & );
};


#endif	//WEAPONSLAM_H
