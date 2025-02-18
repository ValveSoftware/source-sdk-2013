//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "tier0/icommandline.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	/// XXX(JohnS): We support up to 33 slots as of now to allow for sourcetv/replay, but previously 'advertised' 32
	///             slots, which server mods could easily override.  This has caused numerous bugs in the past, and
	///             servers that want to have the 33rd slot can trivially do so, so just don't clamp it beyond what we
	///             support.
	minplayers = 2;  // Force multiplayer.
#ifdef PLATFORM_64BITS
	maxplayers = MAX_PLAYERS;
#else
	if ( CommandLine()->HasParm("-unrestricted_maxplayers") )
	{
		static bool s_bWarned = false;
		if ( !s_bWarned )
		{
			Warning( "The use of -unrestricted_maxplayers is NOT supported and definitely NOT recommended and may be unstable.\n" );
			s_bWarned = true;
		}
		maxplayers = MAX_PLAYERS;
	}
	else
		maxplayers = 33;
#endif
	defaultMaxPlayers = 24;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

