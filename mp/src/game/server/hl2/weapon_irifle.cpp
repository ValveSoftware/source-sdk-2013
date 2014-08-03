//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: This is the incendiary rifle.
//
//=============================================================================


#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "soundent.h"
#include "player.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "weapon_flaregun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//###########################################################################
//	>> CWeaponIRifle
//###########################################################################

class CWeaponIRifle : public CBaseHLCombatWeapon
{
public:

	CWeaponIRifle();

	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CWeaponIRifle, CBaseHLCombatWeapon );

	void	Precache( void );
	bool	Deploy( void );

	void	PrimaryAttack( void );
	virtual float GetFireRate( void ) { return 1; };

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponIRifle, DT_WeaponIRifle)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_irifle, CWeaponIRifle );
PRECACHE_WEAPON_REGISTER(weapon_irifle);

//---------------------------------------------------------
// Activity table
//---------------------------------------------------------
acttable_t	CWeaponIRifle::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_ML, true },
};
IMPLEMENT_ACTTABLE(CWeaponIRifle);

//---------------------------------------------------------
// Constructor
//---------------------------------------------------------
CWeaponIRifle::CWeaponIRifle()
{
	m_bReloadsSingly = true;

	m_fMinRange1		= 65;
	m_fMinRange2		= 65;
	m_fMaxRange1		= 200;
	m_fMaxRange2		= 200;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWeaponIRifle::Precache( void )
{
	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWeaponIRifle::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	m_iClip1 = m_iClip1 - 1;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pOwner->m_flNextAttack = gpGlobals->curtime + 1;

	CFlare *pFlare = CFlare::Create( pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION );

	if ( pFlare == NULL )
		return;

	Vector forward;
	pOwner->EyeVectors( &forward );

	pFlare->SetAbsVelocity( forward * 1500 );

	WeaponSound( SINGLE );
}

//---------------------------------------------------------
// BUGBUG - don't give ammo here.
//---------------------------------------------------------
bool CWeaponIRifle::Deploy( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner)
	{
		pOwner->GiveAmmo( 90, m_iPrimaryAmmoType);
	}
	return BaseClass::Deploy();
}
