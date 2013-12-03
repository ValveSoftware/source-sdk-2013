//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Weapon that throws things from the hand 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPON_BRICKBAT_H
#define	WEAPON_BRICKBAT_H

#ifdef _WIN32
#pragma once
#endif


#include "basehlcombatweapon.h"

enum BrickbatAmmo_t
{
	BRICKBAT_ROCK = 0,
	BRICKBAT_BOTTLE,
	
	NUM_BRICKBAT_AMMO_TYPES
};

class CGrenade_Brickbat;

class CWeaponBrickbat : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponBrickbat, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

public:
	int					m_iCurrentAmmoType;

	void				Precache( void );
	void				Spawn( void );
	bool				Deploy( void );

	virtual const char *GetViewModel( int viewmodelindex =0 );
	virtual const char *GetWorldModel( void );

	void				DrawAmmo( void );

	virtual	int			WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	bool		WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);

	void				SetPickupTouch( void );
	void 				BrickbatTouch( CBaseEntity *pOther );	// default weapon touch
	void				TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );	
	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	bool				ObjectInWay( void );

	void				ThrowBrickbat( Vector vecSrc, Vector vecVelocity, float damage);
	void				ItemPostFrame( void );
	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				Throw( void );

	void				Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_DATADESC();
	DECLARE_ACTTABLE();

	CWeaponBrickbat(void);

private:
	bool				m_bNeedDraw;
	bool				m_bNeedThrow;
	int					m_iThrowBits;				// Save the current throw bits state
	float				m_fNextThrowCheck;			// When to check throw ability next
	Vector				m_vecTossVelocity;

	int					m_nAmmoCount[NUM_BRICKBAT_AMMO_TYPES];			// How much ammo of each type do I own?
};

#endif	//WEAPON_BRICKBAT_H
