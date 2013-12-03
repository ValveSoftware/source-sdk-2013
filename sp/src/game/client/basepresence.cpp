//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base presence implementation for PC
//
//=====================================================================================//

#include "cbase.h"
#include "basepresence.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Default global singleton.  Mods should override this.
static CBasePresence s_basePresence;
IPresence *presence = NULL;

//-----------------------------------------------------------------------------
// Steam version of Rich Presence is a WIP, so PC implementation is stubbed for now.
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
void CBasePresence::Shutdown( void )
{
	// TODO: Implement for PC
}
void CBasePresence::Update( float frametime )
{
	// TODO: Implement for PC
}
void CBasePresence::UserSetContext( unsigned int nUserIndex, unsigned int nContextId, unsigned int nContextValue, bool bAsync )
{
	// TODO: Implement for PC
}
void CBasePresence::UserSetProperty( unsigned int nUserIndex, unsigned int nPropertyId, unsigned int nBytes, const void *pvValue, bool bAsync )
{
	// TODO: Implement for PC
}
void CBasePresence::SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties )
{
	// TODO: Implement for PC
}
unsigned int CBasePresence::GetPresenceID( const char *pIDName )
{
	return 0;
}
const char *CBasePresence::GetPropertyIdString( const uint id )
{
	return NULL;
}
void CBasePresence::GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes )
{
}
void CBasePresence::StartStatsReporting( HANDLE handle, bool bArbitrated )
{
}
void CBasePresence::SetStat( uint iPropertyId, int iPropertyValue, int dataType )
{
}
void CBasePresence::UploadStats()
{
}

//---------------------------------------------------------
// Debug support
//---------------------------------------------------------
void CBasePresence::DebugUserSetContext( const CCommand &args )
{
	if ( args.ArgC() == 3 )
	{
		UserSetContext( 0, atoi( args.Arg( 1 ) ), atoi( args.Arg( 2 ) ) );
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
		UserSetProperty( 0, strtoul( args.Arg( 1 ), NULL, 0 ), sizeof(int), args.Arg( 2 ) );
	}
	else
	{
		Warning( "user_property <property id> <property value>\n" );
	}
}
