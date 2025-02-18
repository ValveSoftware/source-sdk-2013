//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// WaitAndRestart.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vmpi_defs.h"
#include "vmpi_logfile.h"


#pragma warning( disable : 4127 )


char* GetLastErrorString()
{
	static char err[2048];
	
	LPVOID lpMsgBuf;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	strncpy( err, (char*)lpMsgBuf, sizeof( err ) );
	LocalFree( lpMsgBuf );

	err[ sizeof( err ) - 1 ] = 0;
	return err;
}


int main( int argc, char* argv[] )
{
	if ( argc < 4 )
	{
		VMPI_WriteToLogFile( "WaitAndRestart run with invalid command line args. Exiting. But first, you get to see the arguments:\n" );
		for ( int i=0; i < argc; i++ )
		{
			VMPI_WriteToLogFile( "\targv[%d]: %s\n", i, argv[i] );
		}
		
		printf( "WaitAndRestart <seconds to wait> <working directory> command line...\n" );
		return 1;
	}

	const char *pTimeToWait = argv[1];
	const char *pWorkingDir = argv[2];
	
	// If a * precedes the time-to-wait arg, then it's a process ID and we wait for that process to exit.
	if ( pTimeToWait[0] == '*' )
	{
		++pTimeToWait;
		DWORD dwProcessId;
		sscanf( pTimeToWait, "%lu", &dwProcessId );
		
		VMPI_WriteToLogFile( "Waiting for process %lu to exit. Press a key to cancel...\n", dwProcessId );
		
		HANDLE hProcess = OpenProcess( SYNCHRONIZE, false, dwProcessId );
		if ( hProcess )
		{
			while ( 1 )
			{
				DWORD val = WaitForSingleObject( hProcess, 100 );
				if ( val == WAIT_OBJECT_0 )
				{
					break;
				}
				else if ( val == WAIT_ABANDONED )
				{
					VMPI_WriteToLogFile( "Got WAIT_ABANDONED (error). Waiting 5 seconds, then continuing.\n" );
					Sleep( 5000 );
					break;
				}

				if ( kbhit() )
					return 2;
			}
			VMPI_WriteToLogFile( "Process %lu terminated. Continuing.\n", dwProcessId );
		}
		else
		{
			VMPI_WriteToLogFile( "Process %lu not running. Continuing.\n", dwProcessId );
		}
		
		CloseHandle( hProcess );
	}
	else
	{
		DWORD timeToWait = (DWORD)atoi( argv[1] );
		
		VMPI_WriteToLogFile( "\n\nWaiting for %d seconds to launch ' ", timeToWait );
		VMPI_WriteToLogFile( "%s> ", pWorkingDir );
		for ( int i=3; i < argc; i++ )
		{
			VMPI_WriteToLogFile( "%s ", argv[i] );
		}
		VMPI_WriteToLogFile( "'\n\nPress a key to cancel... " );

		DWORD startTime = GetTickCount();
		while ( GetTickCount() - startTime < (timeToWait*1000) )
		{
			if ( kbhit() )
				return 2;
			
			Sleep( 100 );
		}
	}

	// Ok, launch it!
	char commandLine[1024] = {0};
	for ( int i=3; i < argc; i++ )
	{
		strcat_s( commandLine, sizeof( commandLine ), "\"" );
		strcat_s( commandLine, sizeof( commandLine ), argv[i] );
		strcat_s( commandLine, sizeof( commandLine ), "\" " );
	}
	
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,						// security
		NULL,
		FALSE,
		0,							// flags
		NULL,						// environment
		pWorkingDir,						// current directory
		&si,
		&pi ) )
	{
		VMPI_WriteToLogFile( "Process started.\n" );
		CloseHandle( pi.hThread );	// We don't care what the process does.
		CloseHandle( pi.hProcess );
	}
	else
	{
		VMPI_WriteToLogFile( "CreateProcess error!\n%s", GetLastErrorString() );
	}

	return 0;
}

