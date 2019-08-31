//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_PROFICIENCY_H
#define WEAPON_PROFICIENCY_H

#if defined( _WIN32 )
#pragma once
#endif

struct WeaponProficiencyInfo_t
{
	float	spreadscale;
	float	bias;
};

enum WeaponProficiency_t
{
#ifdef MAPBASE
	// For the override
	WEAPON_PROFICIENCY_INVALID = -1,
#endif

	WEAPON_PROFICIENCY_POOR = 0,
	WEAPON_PROFICIENCY_AVERAGE,
	WEAPON_PROFICIENCY_GOOD,
	WEAPON_PROFICIENCY_VERY_GOOD,
	WEAPON_PROFICIENCY_PERFECT,
};

const char *GetWeaponProficiencyName( WeaponProficiency_t proficiency );


#endif // WEAPON_PROFICIENCY_H
