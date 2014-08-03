//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef FILESYSTEM_TOOLS_H
#define FILESYSTEM_TOOLS_H
#ifdef _WIN32
#pragma once
#endif


#include "filesystem.h"
#include "filesystem_init.h"


// This is the the path of the initial source file
extern char		qdir[1024];

// This is the base engine + mod-specific game dir (e.g. "d:\tf2\mytfmod\")
extern char		gamedir[1024];	


// ---------------------------------------------------------------------------------------- //
// Filesystem initialization.
// ---------------------------------------------------------------------------------------- //

enum FSInitType_t
{
	FS_INIT_FULL,					// Load gameinfo.txt, maybe use filesystem_steam, and setup search paths.
	FS_INIT_COMPATIBILITY_MODE		// Load filesystem_stdio and that's it.
};

//
// Initializes qdir,  and gamedir. Also initializes the VMPI filesystem if MPI is defined.
//
// pFilename can be NULL if you want to rely on vproject and qproject. If it's specified, FileSystem_Init
// will go up directories from pFilename looking for gameinfo.txt (if vproject isn't specified).
//
// If bOnlyUseFilename is true, then it won't use any alternative methods of finding the vproject dir
// (ie: it won't use -game or -vproject or the vproject env var or qproject).
//
bool				FileSystem_Init( const char *pFilename, int maxMemoryUsage=0, FSInitType_t initType=FS_INIT_FULL, bool bOnlyUseFilename=false );
void				FileSystem_Term();

// Used to connect app-framework based console apps to the filesystem tools
void				FileSystem_SetupStandardDirectories( const char *pFilename, const char *pGameInfoPath );

CreateInterfaceFn	FileSystem_GetFactory( void );


extern IBaseFileSystem	*g_pFileSystem;
extern IFileSystem		*g_pFullFileSystem;	// NOTE: this is here when VMPI is being used, but a VMPI app can
											// ONLY use LoadModule/UnloadModule.


#endif // FILESYSTEM_TOOLS_H
