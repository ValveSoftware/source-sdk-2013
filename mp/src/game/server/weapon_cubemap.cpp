//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponCubemap : public CBaseCombatWeapon
{
public:

	DECLARE_CLASS( CWeaponCubemap, CBaseCombatWeapon );

	void	Precache( void );

	bool	HasAnyAmmo( void )	{ return true; }

	void	Spawn( void );

	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( weapon_cubemap, CWeaponCubemap );

IMPLEMENT_SERVERCLASS_ST( CWeaponCubemap, DT_WeaponCubemap )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCubemap::Precache( void )
{
	BaseClass::Precache();
}

void CWeaponCubemap::Spawn( void )
{
	BaseClass::Spawn();

	//Hack to fix the cubemap weapon not being held by the player.
	//Problem is the model has huge bounds so the new pickup code that checks if the player can see the model fails cause half the entity's bounds are inside the ground.
	//Since this is just a dev tool I made this quick hack so level designers can use it again asap. - Adrian
	UTIL_SetSize( this, Vector( -16, -16, -16 ),  Vector( 16, 16, 16 ) );
}
