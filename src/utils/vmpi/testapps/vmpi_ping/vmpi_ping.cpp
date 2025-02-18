//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vmpi_ping.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iphelpers.h"
#include "vmpi.h"
#include "tier0/platform.h"
#include "bitbuf.h"
#include <conio.h>
#include <stdlib.h>


const char* FindArg( int argc, char **argv, const char *pName, const char *pDefault = "" )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pName ) == 0 )
		{
			if ( (i+1) < argc )
				return argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}


int main(int argc, char* argv[])
{
	CUtlVector<CIPAddr> addrs;

	printf( "\n" );
	printf( "vmpi_ping <option>\n" );
	printf( "option can be:\n" );
	printf( "    -stop              ..  stop any VMPI services\n" );
	printf( "    -kill              ..  kill any processes being run by VMPI\n" );
	printf( "    -patch <timeout>   ..  stops VMPI services for <timeout> seconds\n" );
	printf( "    -mpi_pw <password> ..  only talk to services with the specified password\n" );
	printf( "    -dns               ..  enable DNS lookups (slows the listing down)\n" );
	printf( "    -ShowConsole       ..  show the console window\n" );
	printf( "    -HideConsole       ..  hide the console window\n" );
	
	//Scary to show these to users...
	//printf( "    -ShowCache         ..  show the cache directory and its capacity\n" );
	//printf( "    -FlushCache        ..  flush the cache of ALL VMPI services\n" );
	
	printf( "\n" );


	ISocket *pSocket = CreateIPSocket();
	if ( !pSocket->BindToAny( 0 ) )
	{
		printf( "Error binding to a port!\n" );
		return 1;
	}

	const char *pPassword = FindArg( argc, argv, "-mpi_pw" );

	
	// Figure out which action they want to take.
	int timeout = 0;
	char cRequest = VMPI_PING_REQUEST;
	if ( FindArg( argc, argv, "-Stop" ) )
	{
		cRequest = VMPI_STOP_SERVICE;
	}		
	else if ( FindArg( argc, argv, "-Kill" ) )
	{
		cRequest = VMPI_KILL_PROCESS;
	}
/*
	else if ( FindArg( argc, argv, "-ShowConsole" ) )
	{
		cRequest = VMPI_SHOW_CONSOLE_WINDOW;
	}
	else if ( FindArg( argc, argv, "-HideConsole" ) )
	{
		cRequest = VMPI_HIDE_CONSOLE_WINDOW;
	}
*/
	else if ( FindArg( argc, argv, "-ShowCache" ) )
	{
		cRequest = VMPI_GET_CACHE_INFO;
	}
	else if ( FindArg( argc, argv, "-FlushCache" ) )
	{
		cRequest = VMPI_FLUSH_CACHE;
	}
	else
	{
		const char *pTimeout = FindArg( argc, argv, "-patch", "60" );
		if ( pTimeout )
		{
			if ( isdigit( pTimeout[0] ) )
			{
				cRequest = VMPI_SERVICE_PATCH;
				timeout = atoi( pTimeout );
				printf( "Patching with timeout of %d seconds.\n", timeout );
			}
			else
			{
				printf( "-patch requires a timeout parameter.\n" );
				return 1;
			}
		}
	}


	int nMachines = 0;
	printf( "Pinging VMPI Services... press a key to stop.\n\n" );
	while ( !kbhit() )
	{
		for ( int i=VMPI_SERVICE_PORT; i <= VMPI_LAST_SERVICE_PORT; i++ )
		{
			unsigned char data[256];
			bf_write buf( data, sizeof( data ) );
			buf.WriteByte( VMPI_PROTOCOL_VERSION );
			buf.WriteString( pPassword );
			buf.WriteByte( cRequest );

			if ( cRequest == VMPI_SERVICE_PATCH )
				buf.WriteLong( timeout );

			pSocket->Broadcast( data, buf.GetNumBytesWritten(), i );
		}

		while ( 1 )
		{
			CIPAddr ipFrom;
			char in[256];
			int len = pSocket->RecvFrom( in, sizeof( in ), &ipFrom );
			if ( len == -1 )
				break;

			if ( len >= 2 && 
				in[0] == VMPI_PROTOCOL_VERSION && 
				in[1] == VMPI_PING_RESPONSE && 
				addrs.Find( ipFrom ) == -1 )
			{
				char *pStateString = "(unknown)";
				if ( len >= 3 )
				{
					if ( in[2] )
						pStateString = "(running)";
					else
						pStateString = "(idle)   ";
				}

				++nMachines;
				char nameStr[256];

				if ( FindArg( argc, argv, "-dns" ) && ConvertIPAddrToString( &ipFrom, nameStr, sizeof( nameStr ) ) )
				{
					printf( "%02d. %s - %s:%d (%d.%d.%d.%d)", 
						nMachines, pStateString, nameStr, ipFrom.port,
						ipFrom.ip[0], ipFrom.ip[1], ipFrom.ip[2], ipFrom.ip[3] );
				}
				else
				{
					printf( "%02d. %s - %d.%d.%d.%d:%d", 
						nMachines, pStateString, ipFrom.ip[0], ipFrom.ip[1], ipFrom.ip[2], ipFrom.ip[3], ipFrom.port );
				}

				if ( cRequest == VMPI_GET_CACHE_INFO )
				{
					// Next var is a 64-bit int with the size of the cache.
					char *pCur = &in[3];
					__int64 cacheSize = *((__int64*)pCur);
					pCur += sizeof( __int64 );

					char *pCacheDir = pCur;

					__int64 nMegs = cacheSize / (1024*1024);
					printf( "\n\tCache dir: %s, size: %d megs", pCur, nMegs );
				}

				printf( "\n" );

				addrs.AddToTail( ipFrom );
			}
		}

		Sleep( 1000 );
	}

	return 0;
}

