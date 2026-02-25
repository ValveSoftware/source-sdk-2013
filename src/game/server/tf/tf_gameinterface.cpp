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
	
	// Pivot (25/02/2026): If targeting amd64 platforms AND conditional UNRESTRICTED_MAXPLAYERS_ENABLE
	// set to 1 in game project vpc, raise to MAX_PLAYERS limit specified in game/shared/shareddefs.h.
#if defined( PLATFORM_64BITS ) && defined( UNRESTRICTED_MAXPLAYERS )	// Assuming amd64
	maxplayers = MAX_PLAYERS;
#elif !defined ( PLATFORM_64BITS ) && defined ( UNRESTRICTED_MAXPLAYERS )	// Assuming x86/i686
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
		maxplayers = 33;	// Accounting for one extra SourceTV client.
#else 
	maxplayers = MAX_PLAYERS;	// MAX_PLAYERS = 33
#endif
	defaultMaxPlayers = 24;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

