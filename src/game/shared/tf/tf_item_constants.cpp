//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds constants for tf item
//
//=============================================================================

#include "cbase.h"
#include "tf_item_constants.h"

static const char *s_loadout_position_names[] =
{
	// Weapons & Equipment
	"LOADOUT_POSITION_PRIMARY",
	"LOADOUT_POSITION_SECONDARY",
	"LOADOUT_POSITION_MELEE",
	"LOADOUT_POSITION_UTILITY",
	"LOADOUT_POSITION_BUILDING",
	"LOADOUT_POSITION_PDA",
	"LOADOUT_POSITION_PDA2",

	// Wearables. If you add new wearable slots, make sure you add them to IsWearableSlot() below this.
	"LOADOUT_POSITION_HEAD",
	"LOADOUT_POSITION_MISC",
	
	// other
	"LOADOUT_POSITION_ACTION",
	
	// More wearables, yay!
	"LOADOUT_POSITION_MISC2",
	
	// taunts
	"LOADOUT_POSITION_TAUNT",
	"LOADOUT_POSITION_TAUNT2",
	"LOADOUT_POSITION_TAUNT3",
	"LOADOUT_POSITION_TAUNT4",
	"LOADOUT_POSITION_TAUNT5",
	"LOADOUT_POSITION_TAUNT6",
	"LOADOUT_POSITION_TAUNT7",
	"LOADOUT_POSITION_TAUNT8",
	
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_loadout_position_names ) == CLASS_LOADOUT_POSITION_COUNT );

const char *GetLoadoutPositionName( loadout_positions_t iLoadout )
{
	if ( iLoadout == LOADOUT_POSITION_INVALID )
		return "LOADOUT_POSITION_INVALID";

	return s_loadout_position_names[ iLoadout ];
}

loadout_positions_t GetLoadoutPositionByName( const char *pszLoadoutPositionName )
{
	int iLoadoutPositionCount = ARRAYSIZE( s_loadout_position_names );
	for ( int i=0; i<iLoadoutPositionCount; ++i )
	{
		if ( FStrEq( pszLoadoutPositionName, s_loadout_position_names[i] ) )
		{
			return loadout_positions_t(i);
		}
	}

	return LOADOUT_POSITION_INVALID;
}
