//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#include "ai_grenade.h"


int COMBINE_AE_BEGIN_ALTFIRE;
int COMBINE_AE_ALTFIRE;

ConVar ai_grenade_always_drop( "ai_grenade_always_drop", "0", FCVAR_NONE, "Causes non-Combine grenade user NPCs to be allowed to drop grenades, alt-fire items, etc. *IF* the keyvalue has not already been set in Hammer. This is useful for debugging purposes or if a mod which was developed before this feature was introduced wants its NPCs to drop grenades and beyond without recompiling all of the maps." );
