//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base rich presence implementation for Xbox360
//
//=====================================================================================//

#include "cbase.h"
#include "basepresence.h"
#include "cdll_client_int.h"
#include "ixboxsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Default global instance.  Mods should override this.
static CBasePresence s_basePresence;
IPresence *presence = NULL;

//-----------------------------------------------------------------------------
// Purpose: Init
//-----------------------------------------------------------------------------
bool CBasePresence::Init( void )
{
	if ( !presence )
	{
		// Mod didn't override, default to base implementation
		presence = &s_basePresence;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Shutdown
//-----------------------------------------------------------------------------
void CBasePresence::Shutdown( void )
{
	// Do nothing
}


//-----------------------------------------------------------------------------
// Purpose: Per-frame update
//-----------------------------------------------------------------------------
void CBasePresence::Update( float frametime )
{
	// Do nothing
}


//-----------------------------------------------------------------------------
// Contexts are strings that describe the current state of the game.
//-----------------------------------------------------------------------------
void CBasePresence::UserSetContext( unsigned int nUserIndex, unsigned int nContextId, unsigned int nContextValue, bool bAsync )
{
	if ( !xboxsystem->UserSetContext( nUserIndex, nContextId, nContextValue, bAsync ) )
	{
		Warning( "CBasePresence: UserSetContext failed.\n" );
	}
}


//-----------------------------------------------------------------------------
// Properties are (usually) numeric values that can be insterted into context strings.
//-----------------------------------------------------------------------------
void CBasePresence::UserSetProperty( unsigned int nUserIndex, unsigned int nPropertyId, unsigned int nBytes, const void *pvValue, bool bAsync )
{
 	if ( !xboxsystem->UserSetProperty( nUserIndex, nPropertyId, nBytes, pvValue, bAsync ) )
 	{
 		Warning( "CBasePresence: UserSetProperty failed.\n" );
 	}
}

//-----------------------------------------------------------------------------
// Get game session properties from matchmaking.
//-----------------------------------------------------------------------------
void CBasePresence::SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties )
{
	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Convert a string to a presence ID.
//-----------------------------------------------------------------------------
uint CBasePresence::GetPresenceID( const char *pIdName )
{
	Assert( 0 );
	return 0;
}

//-----------------------------------------------------------------------------
// Convert a presence ID to a string.
//-----------------------------------------------------------------------------
const char *CBasePresence::GetPropertyIdString( const uint id )
{
	Assert( 0 );
	return NULL;
}

//-----------------------------------------------------------------------------
// Get display string for a game property.
//-----------------------------------------------------------------------------
void CBasePresence::GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes )
{
	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Set up for reporting stats to Live.
//-----------------------------------------------------------------------------
void CBasePresence::StartStatsReporting( HANDLE handle, bool bArbitrated )
{
	m_bArbitrated = bArbitrated;
	m_hSession = handle;
	m_bReportingStats = true;
	m_PlayerStats.RemoveAll();
}

//-----------------------------------------------------------------------------
// Set a specific stat property.
//-----------------------------------------------------------------------------
void CBasePresence::SetStat( uint iPropertyId, int iPropertyValue, int dataType )
{
	if ( m_bReportingStats )
	{
		XUSER_PROPERTY prop;
		prop.dwPropertyId = iPropertyId;
		prop.value.nData = iPropertyValue;
		prop.value.type = dataType;
		m_PlayerStats.AddToTail( prop );
	}
}

//-----------------------------------------------------------------------------
// Upload the stats to Live.
//-----------------------------------------------------------------------------
void CBasePresence::UploadStats()
{
	Assert( 0 );
}

//---------------------------------------------------------
// Debug support
//---------------------------------------------------------
void CBasePresence::DebugUserSetContext( const CCommand &args )
{
	if ( args.ArgC() == 3 )
	{
		UserSetContext( XBX_GetPrimaryUserId(), atoi( args.Arg( 1 ) ), atoi( args.Arg( 2 ) ) );
	}
	else
	{
		Warning( "user_context <context id> <context value>\n" );
	}
}
void CBasePresence::DebugUserSetProperty( const CCommand &args )
{
	if ( args.ArgC() == 3 )
	{
		int value = atoi( args.Arg( 2 ) );
		UserSetProperty( XBX_GetPrimaryUserId(), strtoul( args.Arg( 1 ), NULL, 0 ), sizeof(int), &value );
	}
	else
	{
		Warning( "user_property <property id> <property value>\n" );
	}
}
