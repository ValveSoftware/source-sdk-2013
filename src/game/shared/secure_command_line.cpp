//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "fmtstr.h"
#include "secure_command_line.h"

CSecureLaunchSystem::CSecureLaunchSystem( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor )
: CAutoGameSystem( "secure_launch_system" )
, m_pfnUnsafeCmdLineProcessor( pfnUnsafeCmdLineProcessor )
{ }


CSecureLaunchSystem::~CSecureLaunchSystem()
{ 
	m_pfnUnsafeCmdLineProcessor = nullptr;
}


void CSecureLaunchSystem::OnNewUrlLaunchParameters( NewUrlLaunchParameters_t * )
{
	int nBytes = steamapicontext->SteamApps()->GetLaunchCommandLine( nullptr, 0 );
	CUtlString strBuffer;
	strBuffer.SetLength( nBytes );

	int nBytesWritten = steamapicontext->SteamApps()->GetLaunchCommandLine( strBuffer.GetForModify(), nBytes );
	Assert( nBytes == nBytesWritten );
	
	CSteamID steamIDUnknown( ~0u, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
	( *m_pfnUnsafeCmdLineProcessor )( strBuffer.Get(), nBytesWritten, steamIDUnknown );
}


void CSecureLaunchSystem::OnGameRichPresenceJoinRequested( GameRichPresenceJoinRequested_t *pCallback )
{
	if ( !pCallback || !pCallback->m_rgchConnect[0] )
		return;

	char const *szConCommand = pCallback->m_rgchConnect;

	//
	// Work around Steam Overlay bug that it doesn't replace %20 characters
	//
	CFmtStr fmtCommand;
	if ( strstr( szConCommand, "%20" ) )
	{
		fmtCommand.AppendFormat( "%s", szConCommand );
		while ( char *pszReplace = strstr( fmtCommand.Access(), "%20" ) )
		{
			*pszReplace = ' ';
			Q_memmove( pszReplace + 1, pszReplace + 3, Q_strlen( pszReplace + 3 ) + 1 );
		}
		szConCommand = fmtCommand.Access();
	}

	int nBufferSize = V_strlen( szConCommand ) + 1;

	( *m_pfnUnsafeCmdLineProcessor )( szConCommand, nBufferSize, pCallback->m_steamIDFriend );
}

void CSecureLaunchSystem::ProcessCommandLine()
{
	OnNewUrlLaunchParameters( nullptr );
}


CSecureLaunchSystem &SecureLaunchSystem( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor = nullptr )
{
	static bool s_bInited = ( pfnUnsafeCmdLineProcessor != nullptr );
	if ( !s_bInited )
	{
		DebuggerBreakIfDebugging();
		// TODO: Do better than this. But OSX doesn't have Plat_ExitProcessWithError.
		Plat_ExitProcess( 3 );
	}


	// Our singleton. 
	static CSecureLaunchSystem s_SecureLaunchSystem( pfnUnsafeCmdLineProcessor );
	return s_SecureLaunchSystem;
}

void RegisterSecureLaunchProcessFunc( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor )
{
	SecureLaunchSystem( pfnUnsafeCmdLineProcessor );
}

void ProcessInsecureLaunchParameters()
{
	SecureLaunchSystem().ProcessCommandLine();
}

static bool BHelperCheckSafeUserCmdString( char const *ipconnect )
{
	bool bConnectValid = true;
	while ( *ipconnect )
	{
		if ( ( ( *ipconnect >= '0' ) && ( *ipconnect <= '9' ) ) ||
			( ( *ipconnect >= 'a' ) && ( *ipconnect <= 'z' ) ) ||
			( ( *ipconnect >= 'A' ) && ( *ipconnect <= 'Z' ) ) ||
			( *ipconnect == '_' ) || ( *ipconnect == '-' ) || ( *ipconnect == '.' ) ||
			( *ipconnect == ':' ) || ( *ipconnect == '?' ) || ( *ipconnect == '%' ) ||
			( *ipconnect == '/' ) || ( *ipconnect == '=' ) || ( *ipconnect == ' ' ) ||
			( *ipconnect == '[' ) || ( *ipconnect == ']' ) || ( *ipconnect == '@' ) ||
			( *ipconnect == '"' ) || ( *ipconnect == '\'' ) || ( *ipconnect == '#' ) ||
			( *ipconnect == '(' ) || ( *ipconnect == ')' ) || ( *ipconnect == '!' ) ||
			( *ipconnect == '\\' ) || ( *ipconnect == '$' )
			)
			++ipconnect;
		else
		{
			bConnectValid = false;
			break;
		}
	}
	return bConnectValid;
}

#ifndef TF_CLIENT_DLL
void UnsafeCmdLineProcessor( const char *pchUnsafeCmdLine, int cubSize, CSteamID srcSteamID )
{
	if ( cubSize <= 1 )
	{
		DevMsg( "Received empty command line from steam\n" );
		return;
	}

	DevMsg( "Received command line request[%d]: '%s'\n", cubSize, pchUnsafeCmdLine );

	if ( pchUnsafeCmdLine[ 0 ] != '+' )
		return;

	char const *szConCommand = pchUnsafeCmdLine + 1;

	if ( char const *ipconnect = StringAfterPrefix( szConCommand, "connect " ) )
	{
		if ( BHelperCheckSafeUserCmdString( ipconnect ) )
		{
			engine->ClientCmd_Unrestricted( szConCommand );
		}
	}
}
#else
void TFUnsafeCmdLineProcessor( const char *pchUnsafeCmdLine, int cubSize, CSteamID srcSteamID )
{
	if ( cubSize <= 1 )
	{
		DevMsg( "Received empty command line from steam\n" );
		return;
	}

	DevMsg( "Received command line request[%d]: '%s'\n", cubSize, pchUnsafeCmdLine );
	
	//
	// Handle +tf_party_request_join_user first. 
	//
	// Our command parsing sucks -- Check for allowed commands through this path and sanitize their input first
	const char szPrefix[] = "+tf_party_request_join_user";
	size_t nPrefixLen = V_ARRAYSIZE( szPrefix ) - 1;
	if ( V_strncmp( pchUnsafeCmdLine, szPrefix, nPrefixLen ) == 0 && pchUnsafeCmdLine[ nPrefixLen ] )
	{
		unsigned long long arg1 = 0;
		unsigned int arg2 = 0;
		bool bRequestValid = false;
		if ( sscanf( pchUnsafeCmdLine + nPrefixLen, "%llu %u", &arg1, &arg2 ) == 2 )
		{
			CSteamID steamID = CSteamID( arg1 );
			if ( steamID.IsValid() && steamID.GetEAccountType() == k_EAccountTypeIndividual )
			{
				bRequestValid = true;
				CFmtStr cmd( "tf_party_request_join_user %llu %u", (unsigned long long)steamID.ConvertToUint64(), arg2 );
				engine->ClientCmd_Unrestricted( cmd );
			}
		}
		if ( !bRequestValid )
		{
			Warning( "Invalid rich presence join string: \"%s\"\n", pchUnsafeCmdLine );
		}
		return;
	}

	//
	// Handle tf_econ_item_preview
	//
	if ( char const *szItemId = StringAfterPrefix( pchUnsafeCmdLine, "+tf_econ_item_preview " ) )
	{
		Msg( "CClientSteamContext OnGameJoinRequested tf_econ_item_preview" );

		bool bItemIdValid = ( srcSteamID.GetAccountID() == ~0u );
		while ( *szItemId )
		{
			if ( ( ( *szItemId >= '0' ) && ( *szItemId <= '9' ) ) ||
				( ( *szItemId >= 'A' ) && ( *szItemId <= 'S' ) ) )
				++szItemId;	// support new encoding for owner steamid and assetid
			else
			{
				bItemIdValid = false;
				break;
			}
		}
		if ( bItemIdValid )
		{
			engine->ClientCmd( pchUnsafeCmdLine + 1 );
		}
		else
		{
			Warning( "Invalid rich presence preview string: \"%s\"\n", pchUnsafeCmdLine );
		}
		return;
	}

	// Handle +connect.
	{
		if ( char const* ipconnect = StringAfterPrefix( pchUnsafeCmdLine, "+connect " ) )
		{
			if ( BHelperCheckSafeUserCmdString( ipconnect ) )
			{
				char const* szConCommand = pchUnsafeCmdLine + 1;

				engine->ClientCmd_Unrestricted( szConCommand );
				return;
			}
		}
	}

	Warning( "Invalid rich presence string: \"%s\"\n", pchUnsafeCmdLine );
}
#endif

