//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeapon_Manhack : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeapon_Manhack, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	void			Spawn( void );
	void			Precache( void );

	void			ItemPostFrame( void );
	void			PrimaryAttack( void );
	void			SecondaryAttack( void );

	float			m_flBladeYaw;
};

IMPLEMENT_SERVERCLASS_ST( CWeapon_Manhack, DT_Weapon_Manhack)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_manhack, CWeapon_Manhack );
PRECACHE_WEAPON_REGISTER(weapon_manhack);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeapon_Manhack )

	DEFINE_FIELD( m_flBladeYaw,			FIELD_FLOAT ),

END_DATADESC()

void CWeapon_Manhack::Spawn( )
{
	// Call base class first
	BaseClass::Spawn();

	Precache( );
	SetModel( GetViewModel() );

	FallInit();// get ready to fall down.

	m_flBladeYaw = NULL;
	AddSolidFlags( FSOLID_NOT_SOLID );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemPostFrame( void )
{
	WeaponIdle( );
}

void CWeapon_Manhack::Precache( void )
{
	BaseClass::Precache();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::PrimaryAttack()
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::SecondaryAttack()
{
}



