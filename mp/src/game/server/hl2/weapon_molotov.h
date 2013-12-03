//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Molotov weapon
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "basehlcombatweapon.h"

#ifndef	WEAPON_MOLOTOV_H
#define	WEAPON_MOLOTOV_H

class CGrenade_Molotov;

class CWeaponMolotov : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponMolotov, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

private:
	int					m_nNumAmmoTypes;
	bool				m_bNeedDraw;
	int					m_iThrowBits;				// Save the current throw bits state
	float				m_fNextThrowCheck;			// When to check throw ability next
	Vector				m_vecTossVelocity;

public:

	void				Precache( void );
	void				Spawn( void );

	void				DrawAmmo( void );

	virtual	int			WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	bool		WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);

	void				SetPickupTouch( void );
	void				MolotovTouch( CBaseEntity *pOther );	// default weapon touch
	
	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	bool				ObjectInWay( void );

	void				ThrowMolotov( const Vector &vecSrc, const Vector &vecVelocity);
	void				ItemPostFrame( void );
	void				PrimaryAttack( void );
	void				SecondaryAttack( void );

	void				Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	CWeaponMolotov(void);
};

#endif	//WEAPON_MOLOTOV_H
