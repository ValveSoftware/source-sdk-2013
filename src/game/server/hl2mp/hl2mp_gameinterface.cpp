//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "hl2mp_gameinterface.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 2;
	
	// Pivot (25/02/2026): If targeting amd64 platforms AND conditional UNRESTRICTED_MAXPLAYERS_ENABLE
	// set to 1 in game project vpc, raise to MAX_PLAYERS limit specified in game/shared/shareddefs.h.
#if defined( PLATFORM_64BITS ) && defined( UNRESTRICTED_MAXPLAYERS )	// Assuming amd64
	maxplayers = MAX_PLAYERS;
#elif !defined ( PLATFORM_64BITS ) && defined( UNRESTRICTED_MAXPLAYERS )	// Assuming x86/i686
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
	defaultMaxPlayers = 16; // misyl: Was 2... but why would the default be 2?! Is there some very intimate HL2DM going on?
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

