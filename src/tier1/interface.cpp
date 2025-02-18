//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif

#if !defined( DONT_PROTECT_FILEIO_FUNCTIONS )
#define DONT_PROTECT_FILEIO_FUNCTIONS // for protected_things.h
#endif

#if defined( PROTECTED_THINGS_ENABLE )
#undef PROTECTED_THINGS_ENABLE // from protected_things.h
#endif

#include <stdio.h>
#include "interface.h"
#include "basetypes.h"
#include "tier0/dbg.h"
#include <string.h>
#include <stdlib.h>
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#ifdef _WIN32
#include <direct.h> // getcwd
#elif POSIX
#include <dlfcn.h>
#include <unistd.h>
#define _getcwd getcwd
#endif
#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------ //
// InterfaceReg.
// ------------------------------------------------------------------------------------ //
InterfaceReg *InterfaceReg::s_pInterfaceRegs = NULL;

InterfaceReg::InterfaceReg( InstantiateInterfaceFn fn, const char *pName ) :
	m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = s_pInterfaceRegs;
	s_pInterfaceRegs = this;
}

// ------------------------------------------------------------------------------------ //
// CreateInterface.
// This is the primary exported function by a dll, referenced by name via dynamic binding
// that exposes an opqaue function pointer to the interface.
//
// We have the Internal variant so Sys_GetFactoryThis() returns the correct internal 
// symbol under GCC/Linux/Mac as CreateInterface is DLL_EXPORT so its global so the loaders
// on those OS's pick exactly 1 of the CreateInterface symbols to be the one that is process wide and 
// all Sys_GetFactoryThis() calls find that one, which doesn't work. Using the internal walkthrough here
// makes sure Sys_GetFactoryThis() has the dll specific symbol and GetProcAddress() returns the module specific
// function for CreateInterface again getting the dll specific symbol we need.
// ------------------------------------------------------------------------------------ //
void* CreateInterfaceInternal( const char *pName, int *pReturnCode )
{
	InterfaceReg *pCur;
	
	for (pCur=InterfaceReg::s_pInterfaceRegs; pCur; pCur=pCur->m_pNext)
	{
		if (strcmp(pCur->m_pName, pName) == 0)
		{
			if (pReturnCode)
			{
				*pReturnCode = IFACE_OK;
			}
			return pCur->m_CreateFn();
		}
	}
	
	if (pReturnCode)
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;	
}

void* CreateInterface( const char *pName, int *pReturnCode )
{
    return CreateInterfaceInternal( pName, pReturnCode );
}



#ifdef POSIX
// Linux doesn't have this function so this emulates its functionality
void *GetModuleHandle(const char *name)
{
	void *handle;

	if( name == NULL )
	{
		// hmm, how can this be handled under linux....
		// is it even needed?
		return NULL;
	}

    if( (handle=dlopen(name, RTLD_NOW))==NULL)
    {
            printf("DLOPEN Error:%s\n",dlerror());
            // couldn't open this file
            return NULL;
    }

	// read "man dlopen" for details
	// in short dlopen() inc a ref count
	// so dec the ref count by performing the close
	dlclose(handle);
	return handle;
}
#endif

#if defined( _WIN32 ) && !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
static void *Sys_GetProcAddress( const char *pModuleName, const char *pName )
{
	HMODULE hModule = (HMODULE)GetModuleHandle( pModuleName );
#ifdef WIN32
	return (void *)GetProcAddress( hModule, pName );
#else
	return (void *)dlsym( (void *)hModule, pName );
#endif
}

#if !defined(LINUX)
static void *Sys_GetProcAddress( HMODULE hModule, const char *pName )
{
#ifdef WIN32
	return (void *)GetProcAddress( hModule, pName );
#else
	return (void *)dlsym( (void *)hModule, pName );
#endif
}
#endif

bool Sys_IsDebuggerPresent()
{
	return Plat_IsInDebugSession();
}

struct ThreadedLoadLibaryContext_t
{
	const char *m_pLibraryName;
	HMODULE m_hLibrary;
};

#ifdef _WIN32

// wraps LoadLibraryEx() since 360 doesn't support that
static HMODULE InternalLoadLibrary( const char *pName, Sys_Flags flags )
{
#if defined(_X360)
	return LoadLibrary( pName );
#else
	if ( flags & SYS_NOLOAD )
		return GetModuleHandle( pName );
	else
		return LoadLibraryEx( pName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
#endif
}
uintp ThreadedLoadLibraryFunc( void *pParam )
{
	ThreadedLoadLibaryContext_t *pContext = (ThreadedLoadLibaryContext_t*)pParam;
	pContext->m_hLibrary = InternalLoadLibrary( pContext->m_pLibraryName, SYS_NOFLAGS );
	return 0;
}

#endif // _WIN32

HMODULE Sys_LoadLibrary( const char *pLibraryName, Sys_Flags flags )
{
	char str[ 1024 ];
	// Note: DLL_EXT_STRING can be "_srv.so" or "_360.dll". So be careful
	//	when using the V_*Extension* routines...
	const char *pDllStringExtension = V_GetFileExtension( DLL_EXT_STRING );
	const char *pModuleExtension = pDllStringExtension ? ( pDllStringExtension - 1 ) : DLL_EXT_STRING;

	Q_strncpy( str, pLibraryName, sizeof(str) );

	if ( IsX360() )
	{
		// old, probably busted, behavior for xbox
		if ( !Q_stristr( str, pModuleExtension ) )
		{
			V_SetExtension( str, pModuleExtension, sizeof(str) );
		}
	}
	else
	{
		// always force the final extension to be .dll
		V_SetExtension( str, pModuleExtension, sizeof(str) );
	}

	Q_FixSlashes( str );

#ifdef _WIN32
	ThreadedLoadLibraryFunc_t threadFunc = GetThreadedLoadLibraryFunc();
	if ( !threadFunc )
		return InternalLoadLibrary( str, flags );

	// We shouldn't be passing noload while threaded.
	Assert( !( flags & SYS_NOLOAD ) );

	ThreadedLoadLibaryContext_t context;
	context.m_pLibraryName = str;
	context.m_hLibrary = 0;

	ThreadHandle_t h = CreateSimpleThread( ThreadedLoadLibraryFunc, &context );

#ifdef _X360
	ThreadSetAffinity( h, XBOX_PROCESSOR_3 );
#endif

	unsigned int nTimeout = 0;
	while( ThreadWaitForObject( h, true, nTimeout ) == TW_TIMEOUT )
	{
		nTimeout = threadFunc();
	}

	ReleaseThreadHandle( h );
	return context.m_hLibrary;

#elif POSIX
	int dlopen_mode = RTLD_NOW;

	if ( flags & SYS_NOLOAD )
		dlopen_mode |= RTLD_NOLOAD;

	HMODULE ret = ( HMODULE )dlopen( str, dlopen_mode );
	if ( !ret && !( flags & SYS_NOLOAD ) )
	{
		const char *pError = dlerror();
		if ( pError && ( strstr( pError, "No such file" ) == 0 ) && ( strstr( pError, "image not found" ) == 0 ) )
		{
			Msg( " failed to dlopen %s error=%s\n", str, pError );
		}
	}
	
	return ret;
#endif
}
static bool s_bRunningWithDebugModules = false;

//-----------------------------------------------------------------------------
// Purpose: Loads a DLL/component from disk and returns a handle to it
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
CSysModule *Sys_LoadModule( const char *pModuleName, Sys_Flags flags /* = SYS_NOFLAGS (0) */ )
{
	// If using the Steam filesystem, either the DLL must be a minimum footprint
	// file in the depot (MFP) or a filesystem GetLocalCopy() call must be made
	// prior to the call to this routine.
	char szCwd[1024];
	HMODULE hDLL = NULL;

	if ( !Q_IsAbsolutePath( pModuleName ) )
	{
		// full path wasn't passed in, using the current working dir
		_getcwd( szCwd, sizeof( szCwd ) );
		if ( IsX360() )
		{
			int i = CommandLine()->FindParm( "-basedir" );
			if ( i )
			{
				V_strcpy_safe( szCwd, CommandLine()->GetParm( i + 1 ) );
			}
		}
		if (szCwd[strlen(szCwd) - 1] == '/' || szCwd[strlen(szCwd) - 1] == '\\' )
		{
			szCwd[strlen(szCwd) - 1] = 0;
		}

		char szAbsoluteModuleName[1024];
		if ( strstr( pModuleName, PLATFORM_BIN_DIR ) != NULL )
		{
			// don't make bin/bin path
			Q_snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s" CORRECT_PATH_SEPARATOR_S "%s", szCwd, pModuleName );
		}
		else
		{
			Q_snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s" CORRECT_PATH_SEPARATOR_S PLATFORM_BIN_DIR CORRECT_PATH_SEPARATOR_S "%s", szCwd, pModuleName );
		}
		hDLL = Sys_LoadLibrary( szAbsoluteModuleName, flags );
	}

	if ( !hDLL )
	{
		// full path failed, let LoadLibrary() try to search the PATH now
		hDLL = Sys_LoadLibrary( pModuleName, flags );
#if defined( _DEBUG )
		if ( !hDLL )
		{
// So you can see what the error is in the debugger...
#if defined( _WIN32 ) && !defined( _X360 )
			char *lpMsgBuf;
			
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

			LocalFree( (HLOCAL)lpMsgBuf );
#elif defined( _X360 )
			DWORD error = GetLastError();
			Msg( "Error(%d) - Failed to load %s:\n", error, pModuleName );
#else
			Msg( "Failed to load %s: %s\n", pModuleName, dlerror() );
#endif // _WIN32
		}
#endif // DEBUG
	}

#if !defined(LINUX)
	// If running in the debugger, assume debug binaries are okay, otherwise they must run with -allowdebug
	if ( Sys_GetProcAddress( hDLL, "BuiltDebug" ) )
	{
		if ( !IsX360() && hDLL && 
			 !CommandLine()->FindParm( "-allowdebug" ) && 
			 !Sys_IsDebuggerPresent() )
		{
			Error( "Module %s is a debug build\n", pModuleName );
		}

		DevWarning( "Module %s is a debug build\n", pModuleName );

		if ( !s_bRunningWithDebugModules )
		{
			s_bRunningWithDebugModules = true;
			
#if 0 //def IS_WINDOWS_PC
			char chMemoryName[ MAX_PATH ];
			DebugKernelMemoryObjectName( chMemoryName );
			
			(void) CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, chMemoryName );
			// Created a shared memory kernel object specific to process id
			// Existence of this object indicates that we have debug modules loaded
#endif
		}
	}
#endif

	return reinterpret_cast<CSysModule *>(hDLL);
}

//-----------------------------------------------------------------------------
// Purpose: Determine if any debug modules were loaded
//-----------------------------------------------------------------------------
bool Sys_RunningWithDebugModules()
{
	if ( !s_bRunningWithDebugModules )
	{
#if 0 //def IS_WINDOWS_PC
		char chMemoryName[ MAX_PATH ];
		DebugKernelMemoryObjectName( chMemoryName );

		HANDLE hObject = OpenFileMapping( FILE_MAP_READ, FALSE, chMemoryName );
		if ( hObject && hObject != INVALID_HANDLE_VALUE )
		{
			CloseHandle( hObject );
			s_bRunningWithDebugModules = true;
		}
#endif
	}
	return s_bRunningWithDebugModules;
}


//-----------------------------------------------------------------------------
// Purpose: Unloads a DLL/component from
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
void Sys_UnloadModule( CSysModule *pModule )
{
	if ( !pModule )
		return;

	HMODULE	hDLL = reinterpret_cast<HMODULE>(pModule);

#ifdef _WIN32
	FreeLibrary( hDLL );
#elif defined(POSIX)
	dlclose((void *)hDLL);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : module - windows HMODULE from Sys_LoadModule() 
//			*pName - proc name
// Output : factory for this module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( CSysModule *pModule )
{
	if ( !pModule )
		return NULL;

	HMODULE	hDLL = reinterpret_cast<HMODULE>(pModule);
#ifdef _WIN32
	return reinterpret_cast<CreateInterfaceFn>(GetProcAddress( hDLL, CREATEINTERFACE_PROCNAME ));
#elif defined(POSIX)
	// Linux gives this error:
	//../public/interface.cpp: In function `IBaseInterface *(*Sys_GetFactory
	//(CSysModule *)) (const char *, int *)':
	//../public/interface.cpp:154: ISO C++ forbids casting between
	//pointer-to-function and pointer-to-object
	//
	// so lets get around it :)
	return (CreateInterfaceFn)(GetProcAddress( (void *)hDLL, CREATEINTERFACE_PROCNAME ));
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of this module
// Output : interface_instance_t
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactoryThis( void )
{
	return &CreateInterfaceInternal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of the named module
// Input  : *pModuleName - name of the module
// Output : interface_instance_t - instance of that module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( const char *pModuleName )
{
#ifdef _WIN32
	return static_cast<CreateInterfaceFn>( Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME ) );
#elif defined(POSIX)
	// see Sys_GetFactory( CSysModule *pModule ) for an explanation
	return (CreateInterfaceFn)( Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: get the interface for the specified module and version
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
bool Sys_LoadInterface(
	const char *pModuleName,
	const char *pInterfaceVersionName,
	CSysModule **pOutModule,
	void **pOutInterface )
{
	CSysModule *pMod = Sys_LoadModule( pModuleName );
	if ( !pMod )
		return false;

	CreateInterfaceFn fn = Sys_GetFactory( pMod );
	if ( !fn )
	{
		Sys_UnloadModule( pMod );
		return false;
	}

	*pOutInterface = fn( pInterfaceVersionName, NULL );
	if ( !( *pOutInterface ) )
	{
		Sys_UnloadModule( pMod );
		return false;
	}

	if ( pOutModule )
		*pOutModule = pMod;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Place this as a singleton at module scope (e.g.) and use it to get the factory from the specified module name.  
// 
// When the singleton goes out of scope (.dll unload if at module scope),
//  then it'll call Sys_UnloadModule on the module so that the refcount is decremented 
//  and the .dll actually can unload from memory.
//-----------------------------------------------------------------------------
CDllDemandLoader::CDllDemandLoader( char const *pchModuleName ) : 
	m_pchModuleName( pchModuleName ), 
	m_hModule( 0 ),
	m_bLoadAttempted( false )
{
}

CDllDemandLoader::~CDllDemandLoader()
{
	Unload();
}

CreateInterfaceFn CDllDemandLoader::GetFactory()
{
	if ( !m_hModule && !m_bLoadAttempted )
	{
		m_bLoadAttempted = true;
		m_hModule = Sys_LoadModule( m_pchModuleName );
	}

	if ( !m_hModule )
	{
		return NULL;
	}

	return Sys_GetFactory( m_hModule );
}

void CDllDemandLoader::Unload()
{
	if ( m_hModule )
	{
		Sys_UnloadModule( m_hModule );
		m_hModule = 0;
	}
}


