//========= Copyright Valve Corporation, All rights reserved. ============//
// Overwatch-style mod — shared definitions.
//=============================================================================
#ifndef OW_SHARED_DEFS_H
#define OW_SHARED_DEFS_H

#define OW_MAX_HEROES			16
#define OW_MAX_ABILITIES		4
#define OW_ULT_CHARGE_MAX		100.0f

enum OWHeroId_t
{
	OW_HERO_INVALID = -1,
	OW_HERO_SKIRMISHER = 0,
	OW_HERO_TANK,
	OW_HERO_BRUISER,
	OW_HERO_SNIPER,
	OW_HERO_HEALER,
	OW_HERO_BUILDER,
	OW_HERO_COUNT
};

enum OWAbilitySlot_t
{
	OW_ABILITY_SLOT_1 = 0,
	OW_ABILITY_SLOT_2,
	OW_ABILITY_SLOT_3,
	OW_ABILITY_SLOT_ULT,
};

enum OWAbilityType_t
{
	OW_ABILITY_COND = 0,
	OW_ABILITY_HEAL_AURA,
	OW_ABILITY_DAMAGE_BOOST,
	OW_ABILITY_SHIELD,
	OW_ABILITY_ULT_RAGE,
	OW_ABILITY_ULT_HEAL,
	OW_ABILITY_ULT_SNIPER,
};

enum OWGameModeType_t
{
	OW_MODE_AUTO = 0,
	OW_MODE_ESCORT,
	OW_MODE_CONTROL,
	OW_MODE_ASSAULT,
};

#endif // OW_SHARED_DEFS_H
