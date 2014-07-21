//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FILESYSTEM_INIT_H
#define FILESYSTEM_INIT_H
#ifdef _WIN32
#pragma once
#endif


#include "filesystem.h"


// If this option is on the command line, then filesystem_init won't bring up the vconfig
// dialog even if FS_ERRORMODE_VCONFIG is used.
#define CMDLINEOPTION_NOVCONFIG	"-NoVConfig"

#define	GAMEDIR_TOKEN		"VProject"


#if defined( _WIN32 ) || defined( WIN32 )
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#else	//_WIN32
#define PATHSEPARATOR(c) ((c) == '/')
#endif	//_WIN32


enum FSReturnCode_t
{
	FS_OK,
	FS_MISSING_GAMEINFO_FILE,
	FS_INVALID_GAMEINFO_FILE,
	FS_INVALID_PARAMETERS,
	FS_UNABLE_TO_INIT,
	FS_MISSING_STEAM_DLL
};


enum FSErrorMode_t
{
	FS_ERRORMODE_AUTO,		// Call Error() in case of an error.
	FS_ERRORMODE_VCONFIG,	// Call Error() for errors and run vconfig when appropriate.
	FS_ERRORMODE_NONE,		// Just return FSReturnCode values and setup the string for FileSystem_GetLastErrorString.
};


class CFSSteamSetupInfo
{
public:
	CFSSteamSetupInfo();

// Inputs.
public:
	// If this is set, then the init code will look in this directory up to the root for gameinfo.txt.
	// It must be set for FileSystem_LoadSearchPaths to work.
	//
	// (default: null)
	const char		*m_pDirectoryName;

	// If this is true, then it won't look at -vproject, -game, or the vproject environment variable
	// to find gameinfo.txt. If this is true, then m_pDirectoryName must be set.
	//
	// (default: false)
	bool			m_bOnlyUseDirectoryName;

	// If this is true, then:
	// 1. It will set the environment variables that steam.dll looks at for startup info.
	// 2. It will look for ToolsAppId in the gameinfo.txt file and load the
	// steam caches associated with that cache if it's there. This is so apps like Hammer and hlmv
	// can load the main steam caches (like for Counter-Strike or Half-Life 2), and also load the
	// caches that include tools-specific materials (materials\editor, materials\debug, etc).
	//
	// (default: true - should be FALSE for the engine)
	bool			m_bToolsMode;

	// If this is true, and m_bToolsMode is false, then it will append the path to steam.dll to the
	// PATH environment variable. This makes it so you can run the engine under Steam without
	// having to copy steam.dll up into your hl2.exe folder.
	// (default: false)
	bool			m_bSetSteamDLLPath;

	// Are we loading the Steam filesystem? This should be the same value that 
	// FileSystem_GetFileSystemDLLName gave you.
	bool			m_bSteam;

	// If this is true, then it won't look for a gameinfo.txt.
	//
	// (default: false)
	bool			m_bNoGameInfo;

// Outputs (if it returns FS_OK).
public:
	char			m_GameInfoPath[512];	// The directory that gameinfo.txt lives in.
};	


class CFSLoadModuleInfo	: public CFSSteamSetupInfo
{
public:
	CFSLoadModuleInfo();

// Inputs.
public:
	// Full path to the file system DLL (gotten from FileSystem_GetFileSystemDLLName).
	const char		*m_pFileSystemDLLName;

	// Passed to IFileSystem::Connect.
	CreateInterfaceFn	m_ConnectFactory;

// Outputs (if it returns FS_OK).
public:
	// The filesystem you got from FileSystem_LoadFileSystemModule.
	IFileSystem		*m_pFileSystem;
	CSysModule		*m_pModule;
};	


class CFSMountContentInfo
{
public:
	CFSMountContentInfo();

// Inputs.
public:

	// See CFSLoadModuleInfo::m_bToolsMode (this valid should always be the same as you passed to CFSLoadModuleInfo::m_bToolsMode).
	bool				m_bToolsMode;

	// This specifies the directory where gameinfo.txt is. This must be set.
	// It can come from CFSLoadModuleInfo::m_GameInfoPath.
	const char			*m_pDirectoryName;

	// Gotten from CFSLoadModuleInfo::m_pFileSystem.
	IFileSystem			*m_pFileSystem;
};


class CFSSearchPathsInit
{
public:
	CFSSearchPathsInit();

// Inputs.
public:
	// This specifies the directory where gameinfo.txt is. This must be set.
	const char		*m_pDirectoryName;

	// If this is set, then any search paths with a _english will be replaced with _m_pLanguage and added before the
	// _english path
	// (default: null)
	const char		*m_pLanguage;

	// This is the filesystem FileSystem_LoadSearchPaths is talking to.
	IFileSystem		*m_pFileSystem;

	bool m_bMountHDContent;
	bool m_bLowViolence;

// Outputs.
public:
	// This is the location of the first search path called "game", which also becomes your "mod" search path.
	char			m_ModPath[512];
};


const char *GetVProjectCmdLineValue();


// Call this to use a bin directory relative to VPROJECT
void FileSystem_UseVProjectBinDir( bool bEnable );

// This is used by all things that use the application framework:
// Note that the application framework automatically takes care of step 1 if you use CSteamApplication.
// Step 1: Ask filesystem_init for the name of the filesystem DLL to load
FSReturnCode_t FileSystem_GetFileSystemDLLName( char *pFileSystemDLL, int nMaxLen, bool &bSteam );

// Step 2: Use filesystem framework to load/connect/init that filesystem DLL
// -or- just set up the steam environment and get back the gameinfo.txt path
// The second method is used by the application framework, which wants to connect/init the filesystem itself
FSReturnCode_t FileSystem_LoadFileSystemModule( CFSLoadModuleInfo &info );
FSReturnCode_t FileSystem_SetupSteamEnvironment( CFSSteamSetupInfo &info );

// Step 3: Ask filesystem_init to set up the executable search path, and mount the steam content based on the mod gameinfo.txt file
FSReturnCode_t FileSystem_MountContent( CFSMountContentInfo &fsInfo );

// Step 4: Load the search paths out of pGameDirectory\gameinfo.txt.
FSReturnCode_t FileSystem_LoadSearchPaths( CFSSearchPathsInit &initInfo );

// This is automatically done during step 3, but if you want to redo all the search
// paths (like Hammer does), you can call this to reset executable_path.
FSReturnCode_t FileSystem_SetBasePaths( IFileSystem *pFileSystem );

// Utility function to add the PLATFORM search path.
void FileSystem_AddSearchPath_Platform( IFileSystem *pFileSystem, const char *szGameInfoPath );

// See FSErrorMode_t. If you don't specify one here, then the default is FS_ERRORMODE_VCONFIG.
void FileSystem_SetErrorMode( FSErrorMode_t errorMode = FS_ERRORMODE_VCONFIG );

bool FileSystem_GetExecutableDir( char *exedir, int exeDirLen );

// Clear SteamAppUser, SteamUserPassphrase, and SteamAppId from this process's environment.
// TODO: always do this after LoadFileSysteModule.. there's no reason it should be
// in the environment.
void FileSystem_ClearSteamEnvVars();

// Find the steam.cfg above you for optional stuff
FSReturnCode_t GetSteamCfgPath( char *steamCfgPath, int steamCfgPathLen );

// Setup the Steam.dll path without needing all the extra gameinfo stuff first
// used by the CSteamApplication::Create() code to LoadModule() on the filesystem
// before the underlying apps know specific details about the environment to load
FSReturnCode_t FileSystem_SetupSteamInstallPath();

// Returns the last error.
const char *FileSystem_GetLastErrorString();

void Q_getwd( char *out, int outSize );

#endif // FILESYSTEM_INIT_H
