//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "weapon_proficiency.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *g_ProficiencyNames[] = 
{
	"Poor",
	"Average",
	"Good",
	"Very Good",
	"Perfect"
};

const char *GetWeaponProficiencyName( WeaponProficiency_t proficiency )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE(g_ProficiencyNames) == WEAPON_PROFICIENCY_PERFECT + 1 ); // Hey, update that there table!

	if ( proficiency < 0 || proficiency > WEAPON_PROFICIENCY_PERFECT )
		return "<<Invalid>>";
	return g_ProficiencyNames[proficiency];
}
