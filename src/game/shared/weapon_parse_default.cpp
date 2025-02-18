//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Default implementation for games that don't add custom data to the weapon scripts.
FileWeaponInfo_t* CreateWeaponInfo()
{
	return new FileWeaponInfo_t;
}



