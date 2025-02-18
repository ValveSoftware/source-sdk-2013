//================ Copyright (c) 1996-2009 Valve Corporation. All Rights Reserved. =================
//
//
//
//==================================================================================================

#include <windows.h>
#include <io.h>
#include <conio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>


char g_szVMPILogFilename[MAX_PATH] = {0};


bool VMPI_IsLogFileEnabled()
{
	static bool bChecked = false;
	if ( !bChecked )
	{
		bChecked = true;

		// I'd rather have it check the registry but it's returning cryptic errors when accessed from different accounts
		// even though permissions are set appropriately and docs say you should be able to do it.
		if ( access( "c:\\vmpi_log_enable.txt", 0 ) == 0 )
		{
			strncpy( g_szVMPILogFilename, "c:\\vmpi.log", sizeof( g_szVMPILogFilename ) );
			g_szVMPILogFilename[ sizeof( g_szVMPILogFilename ) - 1 ] = 0;
			return true;
		}
		else
		{
			return false;
		}
	}

	return ( g_szVMPILogFilename[0] != 0 );
}


void VMPI_WriteToLogFile( const char *pMsg, ... )
{
	if ( !VMPI_IsLogFileEnabled() )
		return;

	// Get a time string.
	time_t aclock;
	time( &aclock );
	struct tm newtime;
	struct tm *pTempTime = localtime( &aclock );
	if ( pTempTime )
		newtime = *pTempTime;
	else
		memset( &newtime, 0, sizeof( newtime ) );
	
	// Get rid of the \n.
	char timeString[512];
	strncpy( timeString, asctime( &newtime ), sizeof( timeString ) );
	timeString[ sizeof( timeString ) - 1 ] = 0;

	char *pEnd = strstr( timeString, "\n" );
	if ( pEnd )
		*pEnd = 0;

	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
	{
		strncpy( baseExeFilename, "<unknown exe>", sizeof( baseExeFilename ) );
		baseExeFilename[ sizeof( baseExeFilename ) - 1 ] = 0;
	}

	FILE *fp = fopen( g_szVMPILogFilename, "at" );
	if ( fp )
	{
		fprintf( fp, "[%s, %s]\n", timeString, baseExeFilename );
		size_t len = 4 + strlen( timeString ) + strlen( baseExeFilename );
		while ( len-- )
		{
			fprintf( fp, "-" );
		}

		// Blat out a real string.
		va_list marker;
		va_start( marker, pMsg );
		fprintf( fp, "\n" );
		vfprintf( fp, pMsg, marker );
		fprintf( fp, "\n" );
		va_end( marker );

		fflush( fp );
		fclose( fp );
	}
}



