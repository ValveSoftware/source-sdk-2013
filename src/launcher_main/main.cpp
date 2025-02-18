//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A redirection tool that allows the DLLs to reside elsewhere.
//
//=====================================================================================//

#if defined( _WIN32 )
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <direct.h>
#endif

#ifdef POSIX
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#define MAX_PATH PATH_MAX
#endif

#include <cstdio>
#include <array>
#include <string>
#include <vector>

#include "tier0/basetypes.h"

#ifdef WIN32
typedef int (*LauncherMain_t)( HINSTANCE hInstance, HINSTANCE hPrevInstance, 
							  LPSTR lpCmdLine, int nCmdShow );
#elif POSIX
typedef int (*LauncherMain_t)( int argc, char **argv );
#else
#error
#endif

#ifdef WIN32
// hinting the nvidia driver to use the dedicated graphics card in an optimus configuration
// for more info, see: http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
extern "C" { _declspec( dllexport ) DWORD NvOptimusEnablement = 0x00000001; }

// same thing for AMD GPUs using v13.35 or newer drivers
extern "C" { __declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1; }

#endif


#include <steam/steam_api.h>

#ifdef _WIN32
static HMODULE Launcher_LoadModule( const char *pszPath )
{
	return LoadLibraryEx( pszPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
}

static void Launcher_CloseModule( HMODULE hHandle )
{
	FreeLibrary( hHandle );
}

static auto Launcher_GetProcAddress( HMODULE hHandle, const char *pszName )
{
	return GetProcAddress( hHandle, pszName );
}
#else
#include <dlfcn.h>
#include <SDL2/SDL.h>

static void* Launcher_LoadModule( const char *pszPath )
{
	return dlopen( pszPath, RTLD_NOW );
}

static void Launcher_CloseModule( void *pHandle )
{
	dlclose( pHandle );
}

static void *Launcher_GetProcAddress( void *pHandle, const char *pszName )
{
	return dlsym( pHandle, pszName );
}

#define MessageBox( x, text, title, y) SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, title, text, NULL )
#endif

static const AppId_t k_unSDK2013MPAppId = 243750;

#ifdef MOD_LAUNCHER
static const AppId_t k_unMyModAppid = MOD_APPID;
#else
static const AppId_t k_unMyModAppid = k_unSDK2013MPAppId;
#endif

static bool s_bInittedSteam = false;

#ifdef _WIN32
static HMODULE s_SteamModule;
#else
static void *s_SteamModule;

#define WRAP( fn, ret, ... ) \
	ret __real_##fn(__VA_ARGS__); \
	__attribute__ ((visibility ("hidden"))) ret __wrap_##fn(__VA_ARGS__)

#define CALL( fn ) __real_##fn

// Some stubs for pre-exec.
extern "C"
{
    WRAP(fopen, FILE *, const char *path, const char *mode)
    {
        return CALL(fopen)( path, mode );
    }

    WRAP(fopen64, FILE *, const char *path, const char *mode)
    {
        return CALL(fopen64)( path, mode );
    }
}
#endif

static void UnloadSteam()
{
	if ( s_SteamModule )
	{
		decltype( SteamAPI_Shutdown )* pfnSAPIShutdown = ( decltype( SteamAPI_Shutdown )* )Launcher_GetProcAddress( s_SteamModule, "SteamAPI_Shutdown" );
		if ( pfnSAPIShutdown )
		{
			pfnSAPIShutdown();
			s_bInittedSteam = false;
		}

		Launcher_CloseModule( s_SteamModule );
	}

}


static bool LoadSteam( const char *pRootDir )
{
#if defined( _WIN64 )
	#define STEAM_API_DLL_PATH	"%s\\" PLATFORM_BIN_DIR "\\steam_api64.dll"
#elif defined( _WIN32 )
	#define STEAM_API_DLL_PATH	"%s\\" PLATFORM_BIN_DIR "\\steam_api.dll"
#elif defined( POSIX )
	#define STEAM_API_DLL_PATH	"%s/" PLATFORM_BIN_DIR "/libsteam_api.so"
#endif

	char szBuffer[4096];
	// Assemble the full path to our "steam_api.dll"
	_snprintf( szBuffer, sizeof( szBuffer ), STEAM_API_DLL_PATH, pRootDir );
	szBuffer[sizeof( szBuffer ) - 1] = '\0';

	s_SteamModule = Launcher_LoadModule( szBuffer );
	if ( !s_SteamModule )
	{
		MessageBox( 0, "Not able to load Steam API. Is " STEAM_API_DLL_PATH " missing?", "Launcher Error", MB_OK );
		return false;
	}

	// Make a steam_appid.txt now, of just eg. Source SDK 2013 MP for this.
	FILE *pFile = fopen( "steam_appid.txt", "w" );
	if ( pFile )
	{
		fprintf( pFile, "%u\n", k_unMyModAppid );
		fclose( pFile );
	}

	decltype(SteamAPI_Init) *pfnSAPIInit = (decltype( SteamAPI_Init ) *) GetProcAddress( s_SteamModule, "SteamAPI_Init" );
	if ( !pfnSAPIInit )
	{
		MessageBox( 0, "SteamAPI_Init was not available!", "Launcher Error", MB_OK );
		UnloadSteam();
		return false;
	}

	if ( !pfnSAPIInit() )
	{
		MessageBox( 0, "SteamAPI_Init failed!", "Launcher Error", MB_OK );
		UnloadSteam();
		return false;
	}

	s_bInittedSteam = true;
	return true;
}

static bool GetGameInstallDir( const char *pRootDir, char *pszBuf, int nBufSize )
{
	if ( !LoadSteam( pRootDir ) )
	{
		return false;
	}

	decltype( SteamAPI_GetHSteamUser )* pfnSteamAPI_GetHSteamUser = ( decltype( SteamAPI_GetHSteamUser )* )GetProcAddress( s_SteamModule, "SteamAPI_GetHSteamUser" );
	if ( !pfnSteamAPI_GetHSteamUser )
	{
		MessageBox( 0, "SteamAPI_GetHSteamUser not present!", "Launcher Error", MB_OK );
		return false;
	}

	decltype( SteamInternal_FindOrCreateUserInterface )* pfnSteamInternal_FindOrCreateUserInterface = ( decltype( SteamInternal_FindOrCreateUserInterface )* )GetProcAddress( s_SteamModule, "SteamInternal_FindOrCreateUserInterface" );
	if ( !pfnSteamInternal_FindOrCreateUserInterface )
	{
		MessageBox( 0, "SteamInternal_FindOrCreateUserInterface not present!", "Launcher Error", MB_OK );
		return false;
	}

	ISteamApps* pSteamApps = ( ISteamApps* )( pfnSteamInternal_FindOrCreateUserInterface( pfnSteamAPI_GetHSteamUser(), STEAMAPPS_INTERFACE_VERSION ) );
	if ( !pSteamApps )
	{
		MessageBox( 0, "ISteamApps not present!", "Launcher Error", MB_OK );
		return false;
	}

	uint32_t unLength = 0;
	if ( pSteamApps->BIsAppInstalled( k_unSDK2013MPAppId ) )
	{
		unLength = pSteamApps->GetAppInstallDir( k_unSDK2013MPAppId, pszBuf, nBufSize );
	}

	UnloadSteam();

	if ( unLength == 0 )
	{
		MessageBox( 0, "Source SDK 2013 Multiplayer (243750) must be installed to launch this mod.", "Launcher Error", MB_OK );
		return false;
	}

	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
#if !defined( _X360 )

static char *GetBaseDir( const char *pszBuffer )
{
	static char	basedir[ MAX_PATH ];
	char szBuffer[ MAX_PATH ];
	size_t j;
	char *pBuffer = NULL;

	strcpy( szBuffer, pszBuffer );

#ifdef _WIN32
	pBuffer = strrchr( szBuffer,'\\' );
#else
	pBuffer = strrchr( szBuffer,'/' );
#endif
	if ( pBuffer )
	{
		*(pBuffer+1) = '\0';
	}

	strcpy( basedir, szBuffer );

	j = strlen( basedir );
	if (j > 0)
	{
		if ( ( basedir[ j-1 ] == '\\' ) || 
			 ( basedir[ j-1 ] == '/' ) )
		{
			basedir[ j-1 ] = 0;
		}
	}

	return basedir;
}

#ifdef WIN32

#include <process.h>

std::wstring GetExePath()
{
	std::vector<WCHAR> exePath;
	exePath.resize( MAX_PATH + 1 );

	DWORD len = ::GetModuleFileNameW( NULL, exePath.data(), MAX_PATH );
	if ( !len || len == MAX_PATH )
		return L"";
	exePath.resize( len + 1 );

	return exePath.data();
}

std::wstring GetExeName()
{
	std::wstring fullPath = GetExePath();
	auto n = fullPath.find_last_of( L'\\' );

	return ( n != std::wstring::npos )
		? fullPath.substr( n + 1 )
		: fullPath;
}

std::wstring GetDefaultGameName()
{
	std::wstring sName = GetExeName();
	auto n = sName.find_last_of( L'.' );

	return ( n != std::wstring::npos )
		? sName.substr( 0, n )
		: sName;
}

static void RelaunchAs64Bit( std::vector<std::wstring> &args )
{
#ifndef _WIN64
	if ( args.size() == 0 )
	{
		// oh no
		return;
	}

	auto iter = args[0].find( L".exe" );
	if ( iter == std::string::npos )
		return;
	args[0].replace( iter, wcslen( L".exe" ), L"_win64.exe" );

	std::wstring sExecutable = args[0];

	std::vector<wchar_t*> pArgs;
	for ( std::wstring& arg : args )
	{
		arg = L"\"" + arg + L"\"";
		pArgs.push_back( (wchar_t *)arg.c_str() );
	}

	pArgs.push_back( NULL );

	_wexecvp( sExecutable.c_str(), &pArgs[0]);
	exit( 1 );
#endif
}

static BOOL FileExists( wchar_t *pwcsPath )
{
	DWORD dwAttrib = GetFileAttributesW( pwcsPath );

	return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

bool Is64BitWindows()
{
#if defined(_WIN64)
	return true;
#elif defined(_WIN32)
	BOOL f64 = FALSE;
	return IsWow64Process( GetCurrentProcess(), &f64 ) && f64;
#else
	return false;
#endif
}

static void HandleRelaunching()
{
#ifndef _WIN64
	// Can't relaunch if on 32-bit only windows.
	if ( !Is64BitWindows() )
		return;

	int nArgs = 0;
	LPWSTR* pBaseArgs = CommandLineToArgvW( GetCommandLineW(), &nArgs );

	std::vector<std::wstring> pArgs;
	for ( int i = 0; i < nArgs; i++ )
	{
		pArgs.push_back( pBaseArgs[i] );
	}

	std::wstring sDefaultGameName = GetDefaultGameName();

	const wchar_t *pGameArg = NULL;
	bool bNextIsGameArg = false;
	for ( std::wstring &arg : pArgs )
	{
		if ( bNextIsGameArg )
		{
			pGameArg = arg.c_str();
			bNextIsGameArg = false;
		}
		else if ( arg == L"-game" )
		{
			bNextIsGameArg = true;
		}
		else if ( arg == L"-force32bit" )
		{
			return;
		}
		else if ( arg == L"-force64bit" )
		{
			RelaunchAs64Bit( pArgs );
			return;
		}
	}

	if ( !pGameArg )
	{
		pGameArg = sDefaultGameName.c_str();
	}

	std::wstring sServerDLL = pGameArg;
	if ( sServerDLL.empty() )
	{
		return;
	}

	if ( sServerDLL[sServerDLL.size() - 1] != L'\\' )
	{
		sServerDLL += L"\\";
	}

	sServerDLL += L"bin\\x64\\server.dll";

	if ( FileExists( (wchar_t *)sServerDLL.c_str() ) )
	{
		RelaunchAs64Bit( pArgs );
	}
#endif
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	HandleRelaunching();

	// Must add 'bin' to the path....
	char* pPath = getenv("PATH");

	char szBuffer[4096];

	// Use the .EXE name to determine the root directory
	char moduleName[MAX_PATH];
	if ( !GetModuleFileName( hInstance, moduleName, MAX_PATH ) )
	{
		MessageBox( 0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK );
		return 0;
	}

	// Get the root directory the .exe is in
	char* pRootDir = GetBaseDir( moduleName );
	const char *pBinaryGameDir = pRootDir;
	char szGameInstallDir[4096];
	if ( !GetGameInstallDir( pRootDir, szGameInstallDir, 4096 ) )
	{
		return 1;
	}

	pBinaryGameDir = szGameInstallDir;

	SetEnvironmentVariableA( "SDK_EXEC_DIR", szGameInstallDir );

#define LAUNCHER_DLL_PATH	"%s\\" PLATFORM_BIN_DIR "\\launcher.dll"
#define LAUNCHER_PATH		"%s\\" PLATFORM_BIN_DIR

	_snprintf( szBuffer, sizeof( szBuffer ), "PATH=" LAUNCHER_PATH ";%s", pBinaryGameDir, pPath );
	szBuffer[sizeof( szBuffer ) - 1] = '\0';
	_putenv( szBuffer );

	// Assemble the full path to our "launcher.dll"
	_snprintf( szBuffer, sizeof( szBuffer ), LAUNCHER_DLL_PATH, pBinaryGameDir );
	szBuffer[sizeof( szBuffer ) - 1] = '\0';

	// STEAM OK ... filesystem not mounted yet
	HINSTANCE launcher = LoadLibraryEx( szBuffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

	if ( !launcher )
	{
		char *pszError;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pszError, 0, NULL);

		char szBuf[1024];
		_snprintf(szBuf, sizeof( szBuf ), "Failed to load the launcher DLL:\n\n%s", pszError);
		szBuf[sizeof( szBuf ) - 1] = '\0';
		MessageBox( 0, szBuf, "Launcher Error", MB_OK );

		LocalFree(pszError);
		return 0;
	}

	LauncherMain_t main = (LauncherMain_t)GetProcAddress( launcher, "LauncherMain" );
	return main( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
}

#elif defined (POSIX)

#if defined( LINUX )

bool GetExePath( char *pszPath, size_t size )
{
	return readlink("/proc/self/exe", pszPath, size ) > 0;
}

#include <fcntl.h>

static bool IsDebuggerPresent( int time )
{
	// Need to get around the __wrap_open() stuff. Just find the open symbol
	// directly and use it...
	typedef int (open_func_t)( const char *pathname, int flags, mode_t mode );
	open_func_t *open_func = (open_func_t *)dlsym( RTLD_NEXT, "open" );

	if ( open_func )
	{
		for ( int i = 0; i < time; i++ )
		{
			int tracerpid = -1;

			int fd = (*open_func)( "/proc/self/status", O_RDONLY, S_IRUSR );
			if (fd >= 0)
			{
				char buf[ 4096 ];
				static const char tracerpid_str[] = "TracerPid:";

				const int len = read( fd, buf, sizeof(buf) - 1 );
				if ( len > 0 )
				{
					buf[ len ] = 0;

					const char *str = strstr( buf, tracerpid_str );
					tracerpid = str ? atoi( str + sizeof( tracerpid_str ) ) : -1;
				}

				close( fd );
			}

			if ( tracerpid > 0 )
				return true;

			sleep( 1 );
		}
	}

	return false;
}

static void WaitForDebuggerConnect( int argc, char *argv[], int time )
{
	for ( int i = 1; i < argc; i++ )
	{
		if ( strstr( argv[i], "-wait_for_debugger" ) )
		{
			printf( "\nArg -wait_for_debugger found.\nWaiting %dsec for debugger...\n", time );
			printf( "  pid = %d\n", getpid() );

			if ( IsDebuggerPresent( time ) )
				printf("Debugger connected...\n\n");

			break;
		}
	}
}

#else

static void WaitForDebuggerConnect( int argc, char *argv[], int time )
{
}

#endif // !LINUX


static const char *GetExecutableModName( char *pszExePath )
{
	static char s_szFinalFilename[ MAX_PATH + 1 ] = "hl2";

	char szExePath[ MAX_PATH + 1 ];
	strncpy( szExePath, pszExePath, sizeof( szExePath ) );
	szExePath[ MAX_PATH ] = 0;

	if ( !*szExePath )
		return s_szFinalFilename;

	const char *pszLastSeparator = strrchr( szExePath, '/' );
	if ( !pszLastSeparator )
		return s_szFinalFilename;

	strncpy( s_szFinalFilename, pszLastSeparator + 1, sizeof( s_szFinalFilename ) );
	s_szFinalFilename[ MAX_PATH ] = 0;
	char *pszLastUnderscore = NULL;
	char* pszLastDot = NULL;
	for ( char *ch = s_szFinalFilename; *ch != 0; ch++ )
	{
		if ( *ch == '_' )
		{
			pszLastUnderscore = ch;
		}

		if ( *ch == '.' )
		{
			pszLastDot = ch;
		}
	}

	if ( pszLastUnderscore )
		*pszLastUnderscore = 0;

	if ( pszLastDot )
		*pszLastDot = 0;

	return s_szFinalFilename;
}

int main( int argc, char *argv[] )
{
	char moduleName[MAX_PATH];
	if ( !GetExePath( moduleName, sizeof( moduleName ) ) )
	{
		return 1;
	}

	char* pRootDir = GetBaseDir( moduleName );

	const char *pBinaryGameDir = pRootDir;

	char szGameInstallDir[4096];
	if ( !GetGameInstallDir( pRootDir, szGameInstallDir, 4096 ) )
	{
		return 1;
	}

	char szExecutable[8192];
	snprintf(szExecutable, sizeof(szExecutable), "%s/hl2.sh", szGameInstallDir );

	std::vector<char *> new_argv;

	new_argv.push_back( szExecutable );

	bool bHasGame = false;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !strcmp( argv[i], "-game" ) )
		{
			bHasGame = true;
		}

		new_argv.push_back(argv[i]);
	}

	char szGamePath[8192];
	if ( !bHasGame )
	{
		new_argv.push_back("-game");

		const char *pModName = GetExecutableModName( moduleName );
		snprintf( szGamePath, sizeof( szGamePath ), "%s/%s", pRootDir, pModName );

		printf( "[Source Mod Launcher] Launching default game: %s\n", pModName );

		new_argv.push_back(szGamePath);
	}

	new_argv.push_back(NULL);

	execvp( szExecutable, new_argv.data() );

	return 0;
}

#else
#error
#endif // WIN32 || POSIX

#endif
