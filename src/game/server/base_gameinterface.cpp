//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CServerGameClients::GetPlayerLimits(int& minplayers, int& maxplayers, int& defaultMaxPlayers) const
{
	minplayers = defaultMaxPlayers = 1;

	// Pivot (25/02/2026): If targeting amd64 platforms AND conditional UNRESTRICTED_MAXPLAYERS_ENABLE
	// set to 1 in game project vpc, raise to MAX_PLAYERS limit specified in game/shared/shareddefs.h.

	// Code adapted from game/server/tf/tf_gameinterface.cpp
#ifdef UNRESTRICTED_MAXPLAYERS
#ifdef PLATFORM_64BITS	// Assuming amd64
	maxplayers = MAX_PLAYERS;
#else	// Assuming x86/i686
	if (CommandLine()->HasParm("-unrestricted_maxplayers"))
	{
		static bool s_bWarned = false;
		if (!s_bWarned)
		{
			Warning("The use of -unrestricted_maxplayers is NOT supported and definitely NOT recommended and may be unstable.\n");
			s_bWarned = true;
		}
		maxplayers = MAX_PLAYERS;
	}
	else
		maxplayers = 33;	// Accounting for one extra SourceTV client.
#endif
#else 
	maxplayers = MAX_PLAYERS;	// MAX_PLAYERS = 33
#endif
}


// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities(const char* pMapEntities)
{
}
