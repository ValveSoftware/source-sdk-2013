//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An application framework 
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//

#ifndef APPFRAMEWORK_H
#define APPFRAMEWORK_H

#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystemGroup.h"


//-----------------------------------------------------------------------------
// Gets the application instance..
//-----------------------------------------------------------------------------
void *GetAppInstance();


//-----------------------------------------------------------------------------
// Sets the application instance, should only be used if you're not calling AppMain.
//-----------------------------------------------------------------------------
void SetAppInstance( void* hInstance );


//-----------------------------------------------------------------------------
// Main entry point for the application
//-----------------------------------------------------------------------------
int AppMain( void* hInstance, void* hPrevInstance, const char* lpCmdLine, int nCmdShow, CAppSystemGroup *pAppSystemGroup );
int AppMain( int argc, char **argv, CAppSystemGroup *pAppSystemGroup );


//-----------------------------------------------------------------------------
// Used to startup/shutdown the application
//-----------------------------------------------------------------------------
int AppStartup( void* hInstance, void* hPrevInstance, const char* lpCmdLine, int nCmdShow, CAppSystemGroup *pAppSystemGroup );
int AppStartup( int argc, char **argv, CAppSystemGroup *pAppSystemGroup );
void AppShutdown( CAppSystemGroup *pAppSystemGroup );


//-----------------------------------------------------------------------------
// Macros to create singleton application objects for windowed + console apps
//-----------------------------------------------------------------------------
#if !defined( _X360 )

#ifdef WIN32
#define DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( _globalVarName ) \
	int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )	\
	{																							\
		return AppMain( hInstance, hPrevInstance, lpCmdLine, nCmdShow, &_globalVarName );		\
	}
#elif defined( OSX )
#define DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( _globalVarName ) \
	int main( int argc, char **argv )										\
	{																							\
		extern int ValveCocoaMain( int argc, char **argv, CAppSystemGroup *pAppSystemGroup ); \
		return ValveCocoaMain( argc, argv, &_globalVarName ); \
	}
#elif defined( LINUX )
#define DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( _globalVarName ) \
	int main( int argc, char **argv )										\
	{																							\
		extern int ValveLinuxWindowedMain( int argc, char **argv, CAppSystemGroup *pAppSystemGroup ); \
		return ValveLinuxWindowedMain( argc, argv, &_globalVarName ); \
	}
#else
#error
#endif
	
#else
#define DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( _globalVarName )	\
	void __cdecl main()																\
	{																				\
		AppMain( (HINSTANCE)1, (HINSTANCE)0, NULL, 0, &_globalVarName );		\
	}
#endif

#if !defined( _X360 )
#define DEFINE_CONSOLE_APPLICATION_OBJECT_GLOBALVAR( _globalVarName ) \
	int main( int argc, char **argv )			\
	{											\
		return AppMain( argc, argv, &_globalVarName );	\
	}
#else
#define DEFINE_CONSOLE_APPLICATION_OBJECT_GLOBALVAR( _globalVarName ) \
	void __cdecl main()							\
	{											\
		AppMain( 0, (char**)NULL, &_globalVarName );	\
	}
#endif

#define DEFINE_WINDOWED_APPLICATION_OBJECT( _className )	\
	static _className __s_ApplicationObject;				\
	DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( __s_ApplicationObject )

#define DEFINE_CONSOLE_APPLICATION_OBJECT( _className )	\
	static _className __s_ApplicationObject;			\
	DEFINE_CONSOLE_APPLICATION_OBJECT_GLOBALVAR( __s_ApplicationObject )


//-----------------------------------------------------------------------------
// This class is a helper class used for steam-based applications.
// It loads up the file system in preparation for using it to load other
// required modules from steam.
//-----------------------------------------------------------------------------
class CSteamApplication : public CAppSystemGroup
{
	typedef CAppSystemGroup BaseClass;

public:
	CSteamApplication( CSteamAppSystemGroup *pAppSystemGroup );

	// Implementation of IAppSystemGroup
	virtual bool Create( );
	virtual bool PreInit( );
	virtual int Main( );
	virtual void PostShutdown();
	virtual void Destroy();

	// Use this version in cases where you can't control the main loop and
	// expect to be ticked
	virtual int Startup();
	virtual void Shutdown();

protected:
	IFileSystem *m_pFileSystem;
	CSteamAppSystemGroup *m_pChildAppSystemGroup;
	bool m_bSteam;
};


//-----------------------------------------------------------------------------
// Macros to help create singleton application objects for windowed + console steam apps
//-----------------------------------------------------------------------------
#define DEFINE_WINDOWED_STEAM_APPLICATION_OBJECT_GLOBALVAR( _className, _varName )	\
	static CSteamApplication __s_SteamApplicationObject( &_varName );	\
	DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( __s_SteamApplicationObject )

#define DEFINE_WINDOWED_STEAM_APPLICATION_OBJECT( _className )	\
	static _className __s_ApplicationObject;				\
	static CSteamApplication __s_SteamApplicationObject( &__s_ApplicationObject );	\
	DEFINE_WINDOWED_APPLICATION_OBJECT_GLOBALVAR( __s_SteamApplicationObject )

#define DEFINE_CONSOLE_STEAM_APPLICATION_OBJECT_GLOBALVAR( _className, _varName )	\
	static CSteamApplication __s_SteamApplicationObject( &_varName );	\
	DEFINE_CONSOLE_APPLICATION_OBJECT_GLOBALVAR( __s_SteamApplicationObject )

#define DEFINE_CONSOLE_STEAM_APPLICATION_OBJECT( _className )	\
	static _className __s_ApplicationObject;			\
	static CSteamApplication __s_SteamApplicationObject( &__s_ApplicationObject );	\
	DEFINE_CONSOLE_APPLICATION_OBJECT_GLOBALVAR( __s_SteamApplicationObject )

#endif // APPFRAMEWORK_H
