//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "gc_clientsystem.h"
#include "econ_item_system.h"
#include "econ_item_inventory.h"
#include "quest_objective_manager.h"
#ifdef GAME_DLL
#include "tf_wartracker.h"
#endif
//#include "gcsdk/msgprotobuf.h"

#ifdef TF_CLIENT_DLL
#include "secure_command_line.h"
#endif

//
// TODO: NO_STEAM support!
//

using namespace GCSDK;

// Retry for sending a ClientHello if we don't hear back from the GC.  Generally this shouldn't be necessary, as the GC
// should be aware of our session via the GCH and should not need us to pester it.
const float k_flClientHelloRetry = 30.f;

// Client GC System.
//static CGCClientSystem s_CGCClientSystem;
static CGCClientSystem* s_pCGCGameSpecificClientSystem = NULL; // set this in the game specific derived version if needed
void SetGCClientSystem( CGCClientSystem* pGCClientSystem )
{
	s_pCGCGameSpecificClientSystem = pGCClientSystem;
}

CGCClientSystem *GCClientSystem()
{
	AssertMsg( s_pCGCGameSpecificClientSystem, "GCClientSystem is not initialized in the game specific client system constructor" );
	return s_pCGCGameSpecificClientSystem;
}


#ifdef GAME_DLL
CON_COMMAND( dump_all_caches, "Dump the contents all subsribed SOCaches" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	GCClientSystem()->GetGCClient()->Dump();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
#ifdef _WIN32
// Old code, stricter compiler
#pragma warning(disable : 4355) // warning C4355: 'this': used in base member initializer list
#endif
CGCClientSystem::CGCClientSystem()
: CAutoGameSystemPerFrame( "CGCClientSystem" )
#ifdef CLIENT_DLL
	, m_GCClient( NULL, false )
#else
	, m_GCClient( NULL, true )
	, m_CallbackLogonSuccess( this, &CGCClientSystem::OnLogonSuccess )
#endif
{
	m_bInittedGC = false;
	m_bConnectedToGC = false;
	m_bLoggedOn = false;
	m_timeLastSendHello = 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CGCClientSystem::~CGCClientSystem()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
GCSDK::CGCClient *CGCClientSystem::GetGCClient()
{
	Assert ( this != NULL );
	return &m_GCClient;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CGCClientSystem::BSendMessage( uint32 unMsgType, const uint8 *pubData, uint32 cubData )
{
	return m_GCClient.BSendMessage( unMsgType, pubData, cubData );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CGCClientSystem::BSendMessage( const GCSDK::CGCMsgBase& msg )									
{ 
	return m_GCClient.BSendMessage( msg ); 
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CGCClientSystem::BSendMessage( const GCSDK::CProtoBufMsgBase& msg )									
{ 
	return m_GCClient.BSendMessage( msg ); 
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
GCSDK::CGCClientSharedObjectCache *CGCClientSystem::GetSOCache( const CSteamID &steamID )
{
	return m_GCClient.FindSOCache( steamID, false );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
GCSDK::CGCClientSharedObjectCache *CGCClientSystem::FindOrAddSOCache( const CSteamID &steamID )
{
	return m_GCClient.FindSOCache( steamID, true );
}



//-----------------------------------------------------------------------------
void CGCClientSystem::PostInit()
{
	// Call into the BaseClass.
	CAutoGameSystemPerFrame::PostInit();

	#ifdef CLIENT_DLL
		// Install callback to be notified when our steam logged on status changes.
		ClientSteamContext().InstallCallback( UtlMakeDelegate( this, &CGCClientSystem::SteamLoggedOnCallback ) );

		// Except when debugging internally, we really should never launch the game
		// while not logged on!
		AssertMsg( ClientSteamContext().BLoggedOn(), "No Steam logged on for GC setup!" );

		ThinkConnection();
	#endif
}

//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CGCClientSystem::GameServerActivate()
{
	ThinkConnection();
}
#endif


//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CGCClientSystem::SteamLoggedOnCallback( const SteamLoggedOnChange_t &loggedOnState )
{
	ThinkConnection();
}

#else

//-----------------------------------------------------------------------------
void CGCClientSystem::OnLogonSuccess( SteamServersConnected_t *pLogonSuccess )
{
	ThinkConnection();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGCClientSystem::LevelInitPreEntity()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGCClientSystem::LevelShutdownPostEntity()
{
#ifdef GAME_DLL
	QuestObjectiveManager()->Shutdown();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGCClientSystem::Shutdown()
{
	// Shutdown the GC.
	m_GCClient.Uninit();

	// Reset the init flag.
	m_bInittedGC = false;
	m_bConnectedToGC = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGCClientSystem::SetupGC()
{
	// Pre-Init.
	PreInitGC();
	InventoryManager()->PreInitGC();

	// Init.
	InitGC();

	// Post-Init.
	PostInitGC();
	InventoryManager()->PostInitGC();
	QuestObjectiveManager()->Initialize();

#ifdef GAME_DLL
	GetWarTrackerManager()->Initialize();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGCClientSystem::InitGC()
{
	// Check to see if we have already initialized the GCClient.
	if ( m_bInittedGC )
		return;

	m_GCClient.BInit( nullptr );
	m_bInittedGC = true;
}


//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CGCClientSystem::Update( float frametime )
{
	ThinkConnection();
	if ( m_bInittedGC )
		m_GCClient.BMainLoop( k_nThousand, (uint64)( frametime * 1000000.0f ) );
}
#else
void CGCClientSystem::PreClientUpdate()	
{ 
	ThinkConnection();
	if ( m_bInittedGC )
		m_GCClient.BMainLoop( k_nThousand, ( uint64 )( gpGlobals->frametime * 1000000.0f ) ); 	
}
#endif

//-----------------------------------------------------------------------------
void CGCClientSystem::ThinkConnection()
{
	// Currently logged on?
	#ifdef CLIENT_DLL	
		bool bLoggedOn = ClientSteamContext().BLoggedOn();
	#else
		bool bLoggedOn = steamgameserverapicontext && steamgameserverapicontext->SteamGameServer() && steamgameserverapicontext->SteamGameServer()->BLoggedOn();
	#endif
	if ( bLoggedOn )
	{

		// We're logged on.  Is this a rising edge?
		if ( !m_bLoggedOn )
		{

			// Re-init logon
			m_bLoggedOn = true;

			m_timeLastSendHello = -999.9;
			SetupGC();
		}


	}
	else
	{

		// We're not logged on.  Clear all connection state flags
		m_bLoggedOn = false;
		SetConnectedToGC( false );
		m_timeLastSendHello = -999.9;
	}
}

void CGCClientSystem::SetConnectedToGC( bool bConnected )
{
	m_bConnectedToGC = bConnected;
	/// XXX(JohnS): If we want server-side gc state events this is the place to add them. Consider if they should be
	///             networked.
#ifdef CLIENT_DLL
	IGameEvent *pEvent = gameeventmanager->CreateEvent( bConnected ? "gc_new_session" : "gc_lost_session" );
	if ( pEvent )
	{
		gameeventmanager->FireEventClientSide( pEvent );
	}
#endif
}


//-----------------------------------------------------------------------------
