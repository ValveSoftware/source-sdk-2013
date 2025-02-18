//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vrad_launcher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <direct.h>
#include "tier1/strtools.h"
#include "tier0/icommandline.h"


char* GetLastErrorString()
{
	static char err[2048];
	
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	strncpy( err, (char*)lpMsgBuf, sizeof( err ) );
	LocalFree( lpMsgBuf );

	err[ sizeof( err ) - 1 ] = 0;

	return err;
}


void MakeFullPath( const char *pIn, char *pOut, int outLen )
{
	if ( pIn[0] == '/' || pIn[0] == '\\' || pIn[1] == ':' )
	{
		// It's already a full path.
		Q_strncpy( pOut, pIn, outLen );
	}
	else
	{
		_getcwd( pOut, outLen );
		Q_strncat( pOut, "\\", outLen, COPY_ALL_CHARACTERS );
		Q_strncat( pOut, pIn, outLen, COPY_ALL_CHARACTERS );
	}
}

int main(int argc, char* argv[])
{
	char dllName[512];

	CommandLine()->CreateCmdLine( argc, argv );

	// check whether they used the -both switch. If this is specified, vrad will be run
	// twice, once with -hdr and once without
	int both_arg=0;
	for(int arg=1;arg<argc;arg++)
		if (Q_stricmp(argv[arg],"-both")==0)
		{
			both_arg=arg;
		}

	char fullPath[512], redirectFilename[512];
	MakeFullPath( argv[0], fullPath, sizeof( fullPath ) );
	Q_StripFilename( fullPath );
	Q_snprintf( redirectFilename, sizeof( redirectFilename ), "%s\\%s", fullPath, "vrad.redirect" );

	// First, look for vrad.redirect and load the dll specified in there if possible.
	CSysModule *pModule = NULL;
	FILE *fp = fopen( redirectFilename, "rt" );
	if ( fp )
	{
		if ( fgets( dllName, sizeof( dllName ), fp ) )
		{
			char *pEnd = strstr( dllName, "\n" );
			if ( pEnd )
				*pEnd = 0;

			pModule = Sys_LoadModule( dllName );
			if ( pModule )
				printf( "Loaded alternate VRAD DLL (%s) specified in vrad.redirect.\n", dllName );
			else
				printf( "Can't find '%s' specified in vrad.redirect.\n", dllName );
		}
		
		fclose( fp );
	}

	int returnValue = 0;
	
	for(int mode=0;mode<2;mode++)
	{
		if (mode && (! both_arg))
			continue;
		

		// If it didn't load the module above, then use the 
		if ( !pModule )
		{
			strcpy( dllName, "vrad_dll.dll" );
			pModule = Sys_LoadModule( dllName );
		}
		
		if( !pModule )
		{
			printf( "vrad_launcher error: can't load %s\n%s", dllName, GetLastErrorString() );
			return 1;
		}
		
		CreateInterfaceFn fn = Sys_GetFactory( pModule );
		if( !fn )
		{
			printf( "vrad_launcher error: can't get factory from vrad_dll.dll\n" );
			Sys_UnloadModule( pModule );
			return 2;
		}
		
		int retCode = 0;
		IVRadDLL *pDLL = (IVRadDLL*)fn( VRAD_INTERFACE_VERSION, &retCode );
		if( !pDLL )
		{
			printf( "vrad_launcher error: can't get IVRadDLL interface from vrad_dll.dll\n" );
			Sys_UnloadModule( pModule );
			return 3;
		}
		
		if (both_arg)
			strcpy(argv[both_arg],(mode)?"-hdr":"-ldr");
		returnValue = pDLL->main( argc, argv );
		Sys_UnloadModule( pModule );
		pModule=0;
	}
	return returnValue;
}

