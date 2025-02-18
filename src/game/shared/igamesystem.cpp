//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "datacache/imdlcache.h"
#include "utlvector.h"
#include "vprof.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Pointer to a member method of IGameSystem
typedef void (IGameSystem::*GameSystemFunc_t)();

// Pointer to a member method of IGameSystem
typedef void (IGameSystemPerFrame::*PerFrameGameSystemFunc_t)();

// Used to invoke a method of all added Game systems in order
static void InvokeMethod( GameSystemFunc_t f, char const *timed = 0 );
// Used to invoke a method of all added Game systems in reverse order
static void InvokeMethodReverseOrder( GameSystemFunc_t f );

// Used to invoke a method of all added Game systems in order
static void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed = 0 );

static bool s_bSystemsInitted = false; 

// List of all installed Game systems
static CUtlVector<IGameSystem*> s_GameSystems( 0, 4 );
// List of all installed Game systems
static CUtlVector<IGameSystemPerFrame*> s_GameSystemsPerFrame( 0, 4 );

// The map name
static char* s_pMapName = 0;

static CBasePlayer *s_pRunCommandPlayer = NULL;
static CUserCmd *s_pRunCommandUserCmd = NULL;

//-----------------------------------------------------------------------------
// Auto-registration of game systems
//-----------------------------------------------------------------------------
static	CAutoGameSystem *s_pSystemList = NULL;

CAutoGameSystem::CAutoGameSystem( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pSystemList;
		s_pSystemList = this;
	}
}

static	CAutoGameSystemPerFrame *s_pPerFrameSystemList = NULL;

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
CAutoGameSystemPerFrame::CAutoGameSystemPerFrame( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pPerFrameSystemList;
		s_pPerFrameSystemList = this;
	}
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystem::~IGameSystem()
{
	Remove( this );
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystemPerFrame::~IGameSystemPerFrame()
{
	Remove( this );
}


//-----------------------------------------------------------------------------
// Adds a system to the list of systems to run
//-----------------------------------------------------------------------------
void IGameSystem::Add( IGameSystem* pSys )
{
	s_GameSystems.AddToTail( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.AddToTail( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}


//-----------------------------------------------------------------------------
// Removes a system from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::Remove( IGameSystem* pSys )
{
	s_GameSystems.FindAndRemove( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.FindAndRemove( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}

//-----------------------------------------------------------------------------
// Removes *all* systems from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::RemoveAll(  )
{
	s_GameSystems.RemoveAll();
	s_GameSystemsPerFrame.RemoveAll();
}


//-----------------------------------------------------------------------------
// Client systems can use this to get at the map name
//-----------------------------------------------------------------------------
char const*	IGameSystem::MapName()
{
	return s_pMapName;
}

#ifndef CLIENT_DLL
CBasePlayer *IGameSystem::RunCommandPlayer()
{
	return s_pRunCommandPlayer;
}

CUserCmd *IGameSystem::RunCommandUserCmd()
{
	return s_pRunCommandUserCmd;
}
#endif

//-----------------------------------------------------------------------------
// Invokes methods on all installed game systems
//-----------------------------------------------------------------------------
bool IGameSystem::InitAllSystems()
{
	int i;

	{
		// first add any auto systems to the end
		CAutoGameSystem *pSystem = s_pSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}
			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}

	{
		CAutoGameSystemPerFrame *pSystem = s_pPerFrameSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}

			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}
	// Now remember that we are initted so new CAutoGameSystems will add themselves automatically.
	s_bSystemsInitted = true;

	for ( i = 0; i < s_GameSystems.Count(); ++i )
	{
		MDLCACHE_CRITICAL_SECTION();

		IGameSystem *sys = s_GameSystems[i];

#if defined( _X360 )
		char sz[128];
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Start", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		bool valid = sys->Init();

#if defined( _X360 )
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Finish", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		if ( !valid )
			return false;
	}

	return true;
}

void IGameSystem::PostInitAllSystems( void )
{
	InvokeMethod( &IGameSystem::PostInit, "PostInit" );
}

void IGameSystem::ShutdownAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::Shutdown );
}

void IGameSystem::LevelInitPreEntityAllSystems( char const* pMapName )
{
	// Store off the map name
	if ( s_pMapName )
	{
		delete[] s_pMapName;
	}

	int len = Q_strlen(pMapName) + 1;
	s_pMapName = new char [ len ];
	Q_strncpy( s_pMapName, pMapName, len );

	InvokeMethod( &IGameSystem::LevelInitPreEntity, "LevelInitPreEntity" );
}

void IGameSystem::LevelInitPostEntityAllSystems( void )
{
	InvokeMethod( &IGameSystem::LevelInitPostEntity, "LevelInitPostEntity" );
}

void IGameSystem::LevelShutdownPreClearSteamAPIContextAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreClearSteamAPIContext );
}

void IGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreEntity );
}

void IGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPostEntity );

	if ( s_pMapName )
	{
		delete[] s_pMapName;
		s_pMapName = 0;
	}
}

void IGameSystem::OnSaveAllSystems()
{
	InvokeMethod( &IGameSystem::OnSave );
}

void IGameSystem::OnRestoreAllSystems()
{
	InvokeMethod( &IGameSystem::OnRestore );
}

void IGameSystem::SafeRemoveIfDesiredAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::SafeRemoveIfDesired );
}

#ifdef CLIENT_DLL

void IGameSystem::PreRenderAllSystems()
{
	VPROF("IGameSystem::PreRenderAllSystems");
	InvokePerFrameMethod( &IGameSystemPerFrame::PreRender );
}

void IGameSystem::UpdateAllSystems( float frametime )
{
	SafeRemoveIfDesiredAllSystems();

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c; ++i )
	{
		IGameSystemPerFrame *sys = s_GameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		sys->Update( frametime );
	}
}

void IGameSystem::PostRenderAllSystems()
{
	InvokePerFrameMethod( &IGameSystemPerFrame::PostRender );
}

#else

void IGameSystem::FrameUpdatePreEntityThinkAllSystems()
{
	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePreEntityThink );
}

void IGameSystem::FrameUpdatePostEntityThinkAllSystems()
{
	SafeRemoveIfDesiredAllSystems();

	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePostEntityThink );
}

void IGameSystem::PreClientUpdateAllSystems() 
{
	InvokePerFrameMethod( &IGameSystemPerFrame::PreClientUpdate );
}

#endif


//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethod( GameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_GameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystemPerFrame *sys  = s_GameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in reverse order
//-----------------------------------------------------------------------------
void InvokeMethodReverseOrder( GameSystemFunc_t f )
{
	int i;
	int c = s_GameSystems.Count();
	for ( i = c; --i >= 0; )
	{
		IGameSystem *sys = s_GameSystems[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}


