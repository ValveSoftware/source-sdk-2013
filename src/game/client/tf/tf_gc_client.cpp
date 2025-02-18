//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "tf_gc_client.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_gcmessages.h"
#include "kvpacker.h"
#include "tf_party.h"
// XXX(JohnS): Eventually, we want to send a smaller lobby object to clients. For now, they use the CTFGSLobby, which is
//             in shared code for that reason.
#include "tf_lobby_server.h"
#include "base_gcmessages.pb.h"
#include "igameevents.h"
#include "netadr.h"
#include "econ_item_inventory.h"
#include "tf_item_inventory.h"
#include "tf_hud_mann_vs_machine_status.h"
#include "tf_hud_mainmenuoverride.h"
#include "econ/confirm_dialog.h"
#include "rtime.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include "tf_match_description.h"
#include "tf_progression_description.h"
#include "tf_matchcriteria.h"
#include "tf_xp_source.h"
#include "tf_notification.h"
#include "c_tf_notification.h"
#include "tf_gc_shared.h"
#include "tf_partyclient.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_ladder_data.h"
#include "tf_rating_data.h"
#include "tf_quest_map.h"
#include "tf_quest_map_node.h"
#include "econ_quests.h"

#include "econ_item_description.h"

#include "tf_hud_disconnect_prompt.h"

#include "util_shared.h"
#include <steam/isteamnetworkingutils.h>
#include <steam/isteamnetworkingsockets.h>
#include "filesystem.h"
#include "steam/isteamuser.h"
#include "mini_sha256.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
	#define GCMATCHMAKING_DEBUG_LEVEL 4
#else
	#define GCMATCHMAKING_DEBUG_LEVEL 1
#endif

ConVar tf_mm_debug_level( "tf_mm_debug_level", "4" );

#define GCMatchmakingDebugSpew( lvl, ...) do { if ( lvl <= GCMATCHMAKING_DEBUG_LEVEL || lvl <= tf_mm_debug_level.GetInt() ) { Msg( __VA_ARGS__); } } while(false)


static ConVar mod_inventory_request_timeout( "mod_inventory_request_timeout", "300", FCVAR_NONE, "Seconds to wait for TF inventory before assuming failure" );


using namespace GCSDK;

static const char* GetWebBaseUrl()
{
	switch( GetUniverse() )
	{
	case k_EUniverseDev:
		return "https://teamfortress.local.steam.dev/";
	case k_EUniverseBeta:
		return "https://beta.teamfortress.com/";
	case k_EUniversePublic:
	default:
		return "https://www.teamfortress.com/";
	}
}


static CTFGCClientSystem s_TFGCClientSystem;
CTFGCClientSystem *GTFGCClientSystem() { return &s_TFGCClientSystem; }

static CTFPartyClient s_TFPartyClient;
CTFPartyClient *g_pTFPartyClient = nullptr;
CTFPartyClient* GTFPartyClient() { return g_pTFPartyClient; }

// Dialog Prompt Asking users if they want to rejoin a MvM Game
static CTFRejoinConfirmDialog *s_pRejoinLobbyDialog;


void SubscribeToLocalPlayerSOCache( ISharedObjectListener* pListener )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCClientSystem()->GetGCClient()->AddSOCacheListener( steamID, pListener );
	}
	else
	{
		Assert( !"Failed to subscribe to local user's SOCache!" );
	}
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTFGCClientSystem::CTFGCClientSystem( void )
{
	// replace base GCClientSystem
	SetGCClientSystem( this );

	s_pRejoinLobbyDialog = NULL;

}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CTFGCClientSystem::~CTFGCClientSystem( void )
{
	// Prevent other system from using this pointer after it's destroyed
	SetGCClientSystem( NULL );
}


bool CTFGCClientSystem::Init()
{
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "client_beginconnect" );
	ListenForGameEvent( "server_spawn" );

	// Let SDR know that we will likely want access to the relay network, so we're more
	// likely to have initial ping data to the clusters ready by the time we ask for it
	if ( SteamNetworkingUtils() )
		{ SteamNetworkingUtils()->InitRelayNetworkAccess(); }


	return true;
}


void CTFGCClientSystem::PostInit()
{
	BaseClass::PostInit();
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::PreInitGC()
{
	if ( !m_bRegisteredSharedObjects )
	{
//		REG_SHARED_OBJECT_SUBCLASS( CTFGameAccountClient );
		REG_SHARED_OBJECT_SUBCLASS( CTFParty );
		REG_SHARED_OBJECT_SUBCLASS( CTFGSLobby );
//		REG_SHARED_OBJECT_SUBCLASS( CTFBetaParticipation );
		REG_SHARED_OBJECT_SUBCLASS( CTFPartyInvite );
		REG_SHARED_OBJECT_SUBCLASS( CTFLobbyInvite );
		REG_SHARED_OBJECT_SUBCLASS( CQuestMapNode );
		REG_SHARED_OBJECT_SUBCLASS( CTFRatingData );
		REG_SHARED_OBJECT_SUBCLASS( CQuestMapRewardPurchase );

		m_bRegisteredSharedObjects = true;
	}
}


//-----------------------------------------------------------------------------
void CTFGCClientSystem::PostInitGC()
{
	GCMatchmakingDebugSpew( 1, "CTFGCClientSystem::PostInitGC\n" );

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		GCMatchmakingDebugSpew( 1, "CTFGCClientSystem - adding listener\n" );

		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCClientSystem()->FindOrAddSOCache( steamID )->AddListener( this );
	}
	else
	{
		Warning( "CTFGCClientSystem - couldn't add listener because Steam wasn't ready\n" );
	}

}


//-----------------------------------------------------------------------------
void CTFGCClientSystem::LevelShutdownPostEntity()
{
	BaseClass::LevelShutdownPostEntity();
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::LevelInitPreEntity()
{
	BaseClass::LevelInitPreEntity();
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::Shutdown()
{
	GCMatchmakingDebugSpew( 1, "CTFGCClientSystem::ShutdownGC\n" );

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		GCMatchmakingDebugSpew( 1, "CTFGCClientSystem - adding listener\n" );

		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCSDK::CGCClientSharedObjectCache	*pSOCache = GCClientSystem()->GetSOCache( steamID );
		if ( pSOCache )
		{
			pSOCache->RemoveListener( this );
		}
	}
	else
	{
		Warning( "CTFGCClientSystem - couldn't add listener because Steam wasn't ready\n" );
	}

	BaseClass::Shutdown();
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();
	// Disconnected from gameserver
	if ( !Q_stricmp( pEventName, "client_disconnect" ) )
	{
		m_steamIDCurrentServer.Clear();
		m_eConnectState = eConnectState_Disconnected;

		// We treat the gameserver as authoritative when we are connected to it -- disconnecting from it may change what
		// we believe about still having a live match. (e.g. if we have no pLobby anymore, but were loading into a live
		// match so didn't get the m_bAssignedMatchEnded update, we need to reconsider if we think the match is live)
		if ( UpdateAssignedLobby() )
		{
			// Fire this regardless of if our assigned lobby changed, since other SO fields did change.
			FireGameEventLobbyUpdated();
		}
		return;
	}

	// Started attempting connection to gameserver
	if ( !Q_stricmp( pEventName, "client_beginconnect" ) )
	{
		Assert( IsConnectStateDisconnected() );

		// TODO does the retry command set this source? It should go through ::ConnectToServer
		{
			m_eConnectState = eConnectState_NonmatchmadeServer;
		}
		return;
	}

	// Successfully connected to a gameserver. For MM purposes, we stay in state connecting until server spawn as that
	// ensures there's no intermediate "loading into some server but we're not sure of its steamid yet" state.
	if ( !Q_strcmp( pEventName, "server_spawn" ) )
	{
		GCMatchmakingDebugSpew( 4, "Client reached server_spawn.\n" );
		switch ( m_eConnectState )
		{
			default:
				AssertMsg1( false, "Unknown connect state %d", m_eConnectState );
			// These two can happen when doing weird things with timedemo or listen servers
			case eConnectState_Disconnected:
				m_eConnectState = eConnectState_NonmatchmadeServer;
				GCMatchmakingDebugSpew( 4, "Client connected to non-matchmade.\n" );
				break;

			case eConnectState_ConnectingToMatchmade:
				m_eConnectState = eConnectState_ConnectedToMatchmade;
				GCMatchmakingDebugSpew( 4, "Client connected to matchmade.\n" );
				break;

			case eConnectState_ConnectedToMatchmade:
				break;

			case eConnectState_NonmatchmadeServer:
				break;
		}
		m_steamIDCurrentServer.Clear();
		if ( steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUtils() )
		{
			m_steamIDCurrentServer.SetFromString( event->GetString( "steamid", "" ), GetUniverse() );
			GCMatchmakingDebugSpew( 4, "Recognizing MM server id %s\n", m_steamIDCurrentServer.Render() );
		}

		if ( m_eConnectState == eConnectState_ConnectedToMatchmade && !m_steamIDCurrentServer.IsValid() )
		{
			Warning( "Connected to MM server but no GS steamid is set.\n" );
		}

		return;
	}

}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::InvalidatePingData()
{
}


// Backoff api
void CTFGCClientSystem::WebapiInventoryState_t::Backoff()
{
	if ( m_nBackoffSec == 0 )
		m_nBackoffSec = 20;
	else
		m_nBackoffSec = (m_nBackoffSec * 12 + 9) / 10; // exponential backoff @ 1.2x factor, round up

	m_rtNextRequest = CRTime::RTime32TimeCur() + m_nBackoffSec;
}

void CTFGCClientSystem::WebapiInventoryState_t::RequestSucceeded()
{
	m_rtNextRequest = 0;
	m_nBackoffSec = 0;
}

bool CTFGCClientSystem::WebapiInventoryState_t::IsBackingOff()
{
	return m_rtNextRequest != 0 && CRTime::RTime32TimeCur() <= m_rtNextRequest;
}

void CTFGCClientSystem::WebapiInventoryThink()
{
	WebapiInventoryState_t &state = m_WebapiInventory;

	// Early out if we are waiting backoff timer
	if ( state.IsBackingOff() )
		return;

	switch ( state.m_eState )
	{
	case kWebapiInventoryState_Init:
		state.m_eState = kWebapiInventoryState_RequestAuthToken;
		// fallthrough
	case kWebapiInventoryState_RequestAuthToken:
		if ( !SteamUser() )
			return;

		if ( state.m_hSteamAuthTicket != k_HAuthTicketInvalid )
		{
			SteamUser()->CancelAuthTicket( state.m_hSteamAuthTicket );
			state.m_hSteamAuthTicket = k_HAuthTicketInvalid;
		}

		// Request a ticket from Steam
		state.m_hSteamAuthTicket = SteamUser()->GetAuthTicketForWebApi( "tf2sdk" );
		if ( state.m_hSteamAuthTicket == k_HAuthTicketInvalid )
		{
			state.Backoff();
			return;
		}

		state.m_eState = kWebapiInventoryState_WaitingForAuthToken;
		break;

	case kWebapiInventoryState_WaitingForAuthToken:
		// Nothing to do until we get the steam callback; that will advance us to AuthTokenReceived
		break;

	case kWebapiInventoryState_AuthTokenReceived:
		state.m_eState = kWebapiInventoryState_RequestInventory;
		// fallthrough
	case kWebapiInventoryState_RequestInventory:
	{
		// need access to http + local steam id
		if ( !SteamHTTP() || !SteamUser() )
			return;

		// Request inventory from teamfortress.com webapi
		CFmtStr strUrl( "%swebapi/ISDK/GetInventory/v0001", GetWebBaseUrl() );

		state.m_InventoryRequestCompleted.Cancel();
		state.m_hInventoryRequest = SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, strUrl.Get() );
		if ( state.m_hInventoryRequest == INVALID_HTTPREQUEST_HANDLE )
		{
			// try again next frame
			return;
		}

		// This mod's appid (NOT tf2's appid)
		SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hInventoryRequest, "appid", CNumStr( engine->GetAppID() ) );

		// Authentication token
		CUtlMemory<char> strHexToken;
		int nBufSize = 2 * state.m_bufAuthToken.Count();
		strHexToken.EnsureCapacity( nBufSize + 1 );
		V_binarytohex( state.m_bufAuthToken.Base(), state.m_bufAuthToken.Count(), strHexToken.Base(), strHexToken.Count() ); // TODO: Fix V_binarytohex; it's O(n^2) due to repeated uses of strncat.
		SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hInventoryRequest, "ticket", strHexToken.Base() );
		
		if ( GetUniverse() != k_EUniversePublic )
		{
			// use beta tf2 appid on non public universes
			SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hInventoryRequest, "game_appid", "810" );
		}

		// If we already have an so cache for this user, include its version so we don't send the whole cache if it's unchanged
		CGCClientSharedObjectCache* pExistingSOCache = GetSOCache( SteamUser()->GetSteamID() );
		if( pExistingSOCache && pExistingSOCache->BIsSubscribed() )
		{
			SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hInventoryRequest, "version", CNumStr( pExistingSOCache->GetVersion() ) );
		}

		// Wait a long time for this, api might be slow / inventories are large
		if ( mod_inventory_request_timeout.GetInt() > 0 )
		{
			SteamHTTP()->SetHTTPRequestNetworkActivityTimeout( state.m_hInventoryRequest, mod_inventory_request_timeout.GetInt() );
		}

		SteamAPICall_t callResult;
		if ( !SteamHTTP()->SendHTTPRequest( state.m_hInventoryRequest, &callResult ) )
		{
			state.Backoff();
			return;
		}

		state.m_InventoryRequestCompleted.Set( callResult, this, &CTFGCClientSystem::OnWebapiInventoryReceived );
		state.m_eState = kWebapiInventoryState_WaitingForInventory;
		break;
	}
	case kWebapiInventoryState_WaitingForInventory:
		// nothing to do while we wait for http request.  that will advance us to the next state
		break;

	case kWebapiInventoryState_InventoryReceived:
		// We can get returned to this state from any of the 'connected to server' states.
		// There are three main ways in which this happens:
		// - we stop being connected to a server
		// - the user changed their loadout and we need to sign a new auth ticket
		//   for the server to get items from the gc
		// - the server asked steam for their inventory and steam told them that the auth
		//   ticket was invalid/expired.  we will request a new one and the user won't
		//   get their inventory in-game until that is resolved.

		// Cancel any existing server auth ticket until we are connected.
		if ( state.m_hServerAuthTicket != k_HAuthTicketInvalid )
		{
			SteamUser()->CancelAuthTicket( state.m_hServerAuthTicket );
			state.m_hServerAuthTicket = k_HAuthTicketInvalid;
		}

		// Wait to get into game.
		if ( !engine->IsConnected() )
			return;

		state.m_eState = kWebapiInventoryState_BuildServerMessage;
		//fallthrough

	case kWebapiInventoryState_BuildServerMessage:
	{
		if ( !engine->IsConnected() )
		{
			// Disconnected from server or user changed loadout.  Cancel auth ticket and reauth when we next connect
			state.m_eState = kWebapiInventoryState_InventoryReceived;
			return;
		}

		if ( !SteamUser() )
			return;

		CGCClientSharedObjectCache* pSOCache = GetSOCache( SteamUser()->GetSteamID() );
		if ( !pSOCache )
			return;

		// Build message to send server listing equipped items
		CMsgAuthorizeServerItemRetrieval msgItems;
		SDK_SelectItemsToSendToServer( &msgItems, pSOCache );

		// Serialize it
		int nByteSize = msgItems.ByteSize();
		CUtlBuffer bufMsg;
		bufMsg.EnsureCapacity( nByteSize );
		bufMsg.SeekPut( CUtlBuffer::SEEK_HEAD, nByteSize );
		msgItems.SerializeToArray( bufMsg.Base(), nByteSize );

		// Base64-encode it for communication across the wire in kv / webapi
		// We need this encoded pre-auth-ticket generation.
		Base64EncodeIntoUTLMemory( ( const uint8* )bufMsg.Base(), nByteSize, state.m_strMsgItems );

		// We now have encoded the latest state of the SO cache into our message -- if that changes, we need to re-build
		// our message to the server.
		state.m_bLocalChangesApplied = false;

		state.m_eState = kWebapiInventoryState_RequestServerAuthToken;
		//fallthrough
	}
	case kWebapiInventoryState_RequestServerAuthToken:
	{
		if ( !engine->IsConnected() || state.m_bLocalChangesApplied )
		{
			// Disconnected from server or user changed loadout.  Cancel auth ticket and reauth when we next connect
			state.m_eState = kWebapiInventoryState_InventoryReceived;
			return;
		}

		if ( !SteamUser() )
			return;

		// "Sign" the message by including an auth ticket that identifies itself as the hash of the requested items
		const char* strMsg = state.m_strMsgItems.Base();

		SHA256Digest_t digest;
		Sha256_t sha256;
		Sha256_Init( &sha256 );
		Sha256_Update( &sha256, ( const uint8* )strMsg, V_strlen( strMsg ) );
		Sha256_Final( &sha256, digest );

		// HACK: Steam limits the length of the 'identity' parameter, so don't use the full hash
		// (This weakens our security a bit but the server can't sign tickets so even if they can break the hash
		// they can only use it to maybe look at some other items in our inventory.)
		constexpr int knHashBytesToUse = 15;
		char strDigest[knHashBytesToUse * 2 + 1]; // 2 hex characters per byte, +1 for '\0' terminator
		COMPILE_TIME_ASSERT( V_ARRAYSIZE( digest ) == 32 );
		V_binarytohex( digest + V_ARRAYSIZE( digest ) - knHashBytesToUse, knHashBytesToUse, strDigest, V_ARRAYSIZE( strDigest ) );

		// Request the auth ticket from steam and wait for it to arrive.
		state.m_hServerAuthTicket = SteamUser()->GetAuthTicketForWebApi( strDigest );
		state.m_eState = kWebapiInventoryState_WaitingForServerAuthToken;
		break;
	}

	case kWebapiInventoryState_WaitingForServerAuthToken:
		if ( !engine->IsConnected() || state.m_bLocalChangesApplied )
		{
			// Disconnected from server or user changed loadout.  Cancel auth ticket and reauth when we next connect
			state.m_eState = kWebapiInventoryState_InventoryReceived;
			return;
		}

		// Nothing else to do until we get the steam callback; that will advance us to ServerAuthTokenReceived
		break;

	case kWebapiInventoryState_ServerAuthTokenReceived:
	{
		if ( !engine->IsConnected() || state.m_bLocalChangesApplied )
		{
			// Disconnected from server or user changed loadout.  Cancel auth ticket and reauth when we next connect
			state.m_eState = kWebapiInventoryState_InventoryReceived;
			return;
		}

		// Wait until we are fully loaded to send the user our data
		if ( !engine->IsInGame() )
			return;

		// hex-encode the auth token for sending across the wire
		CUtlMemory<char> strHexToken;
		int nBufSize = 2 * state.m_bufServerAuthToken.Count();
		strHexToken.EnsureCapacity( nBufSize + 1 );
		V_binarytohex( state.m_bufServerAuthToken.Base(), state.m_bufServerAuthToken.Count(), strHexToken.Base(), strHexToken.Count() ); // TODO: Fix V_binarytohex; it's O(n^2) due to repeated uses of strncat.

		// Build KV and send to server
		KeyValues *kv = new KeyValues( "sdk_inventory" );
		kv->SetString( "msg", state.m_strMsgItems.Base() );
		kv->SetString( "ticket", strHexToken.Base() );

		// Add any server-specific fields so it knows what to do with the given inventory items (per-mod loadout may not match the user's real tf2 loadout)
		SDK_AddServerInventoryInfo( kv, GetSOCache( SteamUser()->GetSteamID() ) );

		// Send to the server
		engine->ServerCmdKeyValues( kv );
		state.m_eState = kWebapiInventoryState_SentToServer;
		break;
	}

	case kWebapiInventoryState_SentToServer:
		if ( !engine->IsConnected() || state.m_bLocalChangesApplied )
		{
			// Disconnected from server or user changed loadout.  Cancel auth ticket and reauth when we next connect
			state.m_eState = kWebapiInventoryState_InventoryReceived;
			return;
		}

		if( !engine->IsInGame() )
		{
			// Probably changing level.  Do we need to resend the inventory when we are back into the game?
			//state.m_eState = kWebapiInventoryState_ServerAuthTokenReceived;
			return;
		}

		// User is connected, server has their inventory, everything is great from our perspective -- we don't need to do anything.
		break;
	}
}

void CTFGCClientSystem::ServerRequestEquipment()
{
	// Something went wrong on the server side (e.g. steam invalidated our inventory auth ticket)
	// Get a new one and try again.
	if( m_WebapiInventory.m_eState == kWebapiInventoryState_SentToServer )
	{
		m_WebapiInventory.m_eState = kWebapiInventoryState_InventoryReceived;
	}
	else
	{
		// If we are in any other state, we're already in the process of sending the server a new set of equipment as soon as we can
	}
}

void CTFGCClientSystem::LocalInventoryChanged()
{
	m_WebapiInventory.m_bLocalChangesApplied = true;
}


void CTFGCClientSystem::OnSteamGetTicketForWebApiResponse( GetTicketForWebApiResponse_t *pInfo )
{
	OnWebapiAuthTicketReceived( pInfo );
	OnWebapiServerAuthTicketReceived( pInfo );
}

void CTFGCClientSystem::OnWebapiAuthTicketReceived( GetTicketForWebApiResponse_t *pInfo )
{
	WebapiInventoryState_t& state = m_WebapiInventory;
	if ( state.m_eState != kWebapiInventoryState_WaitingForAuthToken )
		return;

	if ( pInfo->m_hAuthTicket != state.m_hSteamAuthTicket )
		return;

	// This is our ticket.  Assume failure for now, we'll correct this if we find it worked.
	state.Backoff();
	state.m_eState = kWebapiInventoryState_RequestAuthToken;

	// Check that the request succeeded
	if ( pInfo->m_eResult != k_EResultOK )
		return;

	// Validate the token makes sense
	if ( pInfo->m_cubTicket < 0 || pInfo->m_cubTicket > pInfo->k_nCubTicketMaxLength )
		return;

	// Copy the token
	state.m_bufAuthToken.SetCount( pInfo->m_cubTicket );
	memcpy( state.m_bufAuthToken.Base(), pInfo->m_rgubTicket, pInfo->m_cubTicket );

	// Success
	state.RequestSucceeded();
	state.m_eState = kWebapiInventoryState_AuthTokenReceived;
}

// This is just copy-paste from above -- might be nice to refactor
void CTFGCClientSystem::OnWebapiServerAuthTicketReceived( GetTicketForWebApiResponse_t* pInfo )
{
	WebapiInventoryState_t& state = m_WebapiInventory;
	if ( state.m_eState != kWebapiInventoryState_WaitingForServerAuthToken )
		return;

	if ( pInfo->m_hAuthTicket != state.m_hServerAuthTicket )
		return;

	// This is our ticket.  Assume failure for now, we'll correct this if we find it worked.
	state.Backoff();
	state.m_eState = kWebapiInventoryState_RequestServerAuthToken;

	// Check that the request succeeded
	if ( pInfo->m_eResult != k_EResultOK )
		return;

	// Validate the token makes sense
	if ( pInfo->m_cubTicket < 0 || pInfo->m_cubTicket > pInfo->k_nCubTicketMaxLength )
		return;

	// Copy the token
	state.m_bufServerAuthToken.SetCount( pInfo->m_cubTicket );
	memcpy( state.m_bufServerAuthToken.Base(), pInfo->m_rgubTicket, pInfo->m_cubTicket );

	// Success
	state.RequestSucceeded();
	state.m_eState = kWebapiInventoryState_ServerAuthTokenReceived;
}

void CTFGCClientSystem::OnWebapiInventoryReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure )
{
	if ( !SteamHTTP() )
		return; // probably shutting down, just ignore it

	WebapiInventoryState_t& state = m_WebapiInventory;
	if ( bIOFailure || !pInfo )
	{
		Assert( false );

		// Failed to communicate with steam
		// Free our http request (Can we be sure this is the right one?)
		if ( state.m_hInventoryRequest != INVALID_HTTPREQUEST_HANDLE )
		{
			SteamHTTP()->ReleaseHTTPRequest( state.m_hInventoryRequest );
			state.m_hInventoryRequest = INVALID_HTTPREQUEST_HANDLE;
		}
		return;
	}

	// Did we lose this request somehow (e.g. reset state while it was in flight)
	// Just throw it away
	if ( pInfo->m_hRequest != state.m_hInventoryRequest )
	{
		Assert( false );
		SteamHTTP()->ReleaseHTTPRequest( pInfo->m_hRequest );
		return;
	}

	// Assume failure -- we'll correct this if this isn't the case
	state.Backoff();
	state.m_eState = kWebapiInventoryState_RequestInventory;

	// This is our handle -- we'll free it by the end of this.
	state.m_hInventoryRequest = INVALID_HTTPREQUEST_HANDLE;

	if ( !pInfo->m_bRequestSuccessful || pInfo->m_eStatusCode != k_EHTTPStatusCode200OK )
	{
		SteamHTTP()->ReleaseHTTPRequest( pInfo->m_hRequest );
		return;
	}

	// Extract the result
	uint32 unBytes;
	Verify( SteamHTTP()->GetHTTPResponseBodySize( pInfo->m_hRequest, &unBytes ) );
	CUtlBuffer bufInventory;
	bufInventory.EnsureCapacity( unBytes );
	bufInventory.SeekPut( CUtlBuffer::SEEK_HEAD, unBytes );
	Verify( SteamHTTP()->GetHTTPResponseBodyData( pInfo->m_hRequest, (uint8*)bufInventory.Base(), unBytes ) );

	// We're done with the request now
	SteamHTTP()->ReleaseHTTPRequest( pInfo->m_hRequest );

	// Parse it to json and extract the data
	GCSDK::CWebAPIValues* pValues = GCSDK::CWebAPIValues::ParseJSON( bufInventory );
	if ( !pValues )
	{
		Warning( "Received invalid response to inventory request\n" );
		return;
	}

	int nResult = pValues->GetChildInt32Value( "result", k_EResultNone );
	switch ( nResult )
	{
	case k_EResultOK:
		break;

	case k_EResultFail:
		return; // will retry after backoff timer expires

	case k_EResultNotLoggedOn:
		// re-request authentication after backoff time
		state.m_eState = kWebapiInventoryState_RequestAuthToken;
		return;

	default:
	{
		CUtlString strError;
		pValues->GetChildStringValue( strError, "error", "" );
		Warning( "Received unexpected result code %d attempting to retrieve inventory. (%s)\n", nResult, strError.Get() );
		return;
	}
	}

	// Parse the inventory message
	CSteamID userSteamID( pValues->GetChildUInt64Value( "steamID" ) );
	if ( !userSteamID.IsValid() || userSteamID.GetEAccountType() != k_EAccountTypeIndividual || userSteamID.GetEUniverse() != GetUniverse() )
	{
		Warning( "Inventory response has bad owner steam id (%s)\n", userSteamID.Render() );
		return;
	}

	if ( pValues->FindChild( "msg" ) )
	{
		CUtlBuffer bufMsgSubscription;
		if ( !pValues->BGetChildBinaryValue( bufMsgSubscription, "msg" ) )
		{
			Warning( "Inventory response missing inventory\n" );
			return;
		}

		CGCClientSharedObjectCache *pSOCache = GetGCClient()->AddLocalSOCache( userSteamID, bufMsgSubscription.Base(), bufMsgSubscription.TellPut() );
		if ( !pSOCache )
		{
			Warning( "Inventory response failed to create SO cache (probably protobuf didn't parse)\n" );
			return;
		}

		// Version should match the one they said we have
		Assert( pSOCache->GetVersion() == pValues->GetChildUInt64Value( "version" ) );
	}
	else
	{
		// Cache up to date.  Validate version matches
		CGCClientSharedObjectCache* pSOCache = GetGCClient()->FindSOCache( userSteamID, false );
		Assert( pSOCache );
		if( pSOCache )
		{
			Assert( pSOCache->GetVersion() == pValues->GetChildUInt64Value( "version" ) );
		}
	}

	// We were successful, clear backoff timers
	state.RequestSucceeded();
	state.m_eState = kWebapiInventoryState_InventoryReceived;
}

void CTFGCClientSystem::SDK_SelectItemsToSendToServer( CMsgAuthorizeServerItemRetrieval* pMsg, CGCClientSharedObjectCache* pSOCache )
{
	// Only send our equipped items, not the whole inventory

	if ( !pSOCache )
		return; // no items to send
		
	CGCClientSharedObjectTypeCache* pItemCache = pSOCache->FindTypeCache( CEconItem::k_nTypeID );
	if ( !pItemCache )
		return; // no items to send

	for ( uint32 i = 0; i < pItemCache->GetCount(); ++i )
	{
		CEconItem* pItem = ( CEconItem* )pItemCache->GetObject( i );
		if ( !pItem->IsEquipped() )
			continue;

		pMsg->add_item_id( pItem->GetID() );
	}
}

void CTFGCClientSystem::SDK_AddServerInventoryInfo( KeyValues* pKV, CGCClientSharedObjectCache* pSOCache )
{
	// Here is where we would tell the DS which items we have equipped, along with any other info it needs to correctly update the socache

	CGCClientSharedObjectTypeCache* pItemCache = pSOCache->FindTypeCache(CEconItem::k_nTypeID);
	if (!pItemCache)
		return;

	CTFPlayerInventory *pLocalInv = dynamic_cast<CTFPlayerInventory*>(TFInventoryManager()->GetLocalInventory());
	if (pLocalInv == NULL)
		return;

	// Extract our current loadout information and record it in the key values.
	KeyValues *pLoadoutKV = new KeyValues("local_loadout");
	for (int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++)
	{
		char szClass[256];
		V_snprintf(szClass, sizeof(szClass), "%i", iClass);

		KeyValues *pClassKV = new KeyValues(szClass);
		pLoadoutKV->AddSubKey(pClassKV);

		for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
		{
			CEconItemView* pItemView = TFInventoryManager()->GetItemInLoadoutForClass(iClass, iSlot);
			if (!pItemView)
				continue;

			// todo: investigate why we get zeroes here...?
			if (pItemView->GetID() == INVALID_ITEM_ID || pItemView->GetID() == 0)
				continue;

			char szSlot[256];
			V_snprintf(szSlot, sizeof(szSlot), "%i", iSlot);

			pClassKV->SetUint64(szSlot, pItemView->GetID());
		}
	}
	pKV->AddSubKey(pLoadoutKV);

}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::Update( float frametime )
{
	BaseClass::Update( frametime );


	WebapiInventoryThink();



	FOR_EACH_VEC_BACK( m_vecDelayedLocalPlayerSOListenersToAdd, i )
	{
		SubscribeToLocalPlayerSOCache( m_vecDelayedLocalPlayerSOListenersToAdd[ i ] );
		m_vecDelayedLocalPlayerSOListenersToAdd.Remove( i );
	}
}

void CTFGCClientSystem::SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	if ( steamIDOwner == ClientSteamContext().GetLocalPlayerSteamID() )
	{
		// Assert( m_pSOCache == NULL ); // we *can* get two SOCacheSubscribed calls in a row.
		m_pSOCache = GCClientSystem()->GetSOCache( steamIDOwner );
		Assert( m_pSOCache != NULL );

		if ( gameeventmanager )
		{

			// force a lobby update whenever our SO cache arrives
			FireGameEventLobbyUpdated();
		}
	}
}

void CTFGCClientSystem::FireGameEventLobbyUpdated()
{
	IGameEvent *event2 = gameeventmanager->CreateEvent( "lobby_updated" );
	if ( event2 )
	{
		gameeventmanager->FireEventClientSide( event2 );
	}
}

void CTFGCClientSystem::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) { SOChanged( pObject, SOChanged_Create, eEvent ); }
void CTFGCClientSystem::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) { SOChanged( pObject, SOChanged_Update, eEvent ); }
void CTFGCClientSystem::SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) { SOChanged( pObject, SOChanged_Destroy, eEvent ); }

void CTFGCClientSystem::SOChanged( const GCSDK::CSharedObject *pObject, SOChangeType_t changeType, GCSDK::ESOCacheEvent eEvent )
{
	// (CTFParty objects are handled by CTFPartyClient)
	if ( pObject->GetTypeID() == CTFGSLobby::k_nTypeID )
	{
		#if GCMATCHMAKING_DEBUG_LEVEL > 0
			switch ( changeType )
			{
				case SOChanged_Create: GCMatchmakingDebugSpew( 1, "Lobby created\n"); break;
				case SOChanged_Update: GCMatchmakingDebugSpew( 2, "Lobby updated\n"); break;
				case SOChanged_Destroy: GCMatchmakingDebugSpew( 1, "Lobby destroyed\n"); break;
				default: AssertMsg1( false, "Bogus change type %d", changeType );
			}
		#endif

		UpdateAssignedLobby();
		// Fire this regardless of if our assigned lobby changed, since other SO fields did change.
		FireGameEventLobbyUpdated();
	}
	// Notifications. Sync/add/delete with what's in our notification drawer
	else if ( pObject->GetTypeID() == CTFNotification::k_nTypeID )
	{
		const CTFNotification* pSONotification = ( const CTFNotification* )( pObject );
		Msg( "Notification %llu %s: \"%s\"\n",
		     (unsigned long long) pSONotification->Obj().notification_id(),
		     changeType == SOChanged_Create ? "created" : changeType == SOChanged_Destroy ? "destroyed" : "updated",
		     pSONotification->Obj().notification_string().c_str() );

		// Update existing notification if found
		bool bFound = false;
		for ( int i = NotificationQueue_GetNumNotifications() - 1; i >= 0; --i )
		{
			CClientNotification *pNotif = dynamic_cast<CClientNotification *>(NotificationQueue_GetByIndex( i ));
			if ( pNotif && pNotif->NotificationID() == pSONotification->Obj().notification_id() )
			{
				Msg( "Notification %llu already displayed, updating\n",
				     (unsigned long long) pSONotification->Obj().notification_id() );
				bFound = true;
				if ( changeType == SOChanged_Destroy )
				{
					NotificationQueue_Remove( pNotif );
				}
				else
				{
					pNotif->Update( pSONotification );
				}
			}
		}

		// Add them to our notifications drawer if not
		if ( !bFound && changeType != SOChanged_Destroy )
		{
			Msg( "New notification %llu arrived: \"%s\"\n",
			     (unsigned long long) pSONotification->Obj().notification_id(),
			     pSONotification->Obj().notification_string().c_str() );
			CClientNotification *pClientNotification = new CClientNotification();
			pClientNotification->Update( pSONotification );
			NotificationQueue_Add( pClientNotification );
		}
	}
	else if ( pObject->GetTypeID() == CTFLobbyInvite::k_nTypeID )
	{
		OnMatchInvitesUpdated();
	}

}

bool CTFGCClientSystem::UpdateAssignedLobby()
{
	return false;
}

CTFParty* CTFGCClientSystem::GetParty()
{
	return NULL;
}

CTFGSLobby* CTFGCClientSystem::GetLobby() const
{
	return NULL;
}


//-----------------------------------------------------------------------------

bool ForceCompetitiveConvars()
{

	bool anyFailures = false;

	Assert( ThreadInMainThread() );
	for ( ConCommandBase *ccb = g_pCVar->GetCommands(); ccb; ccb = ccb->GetNext() )
	{
		if ( ccb->IsCommand() )
			continue;

		ConVar *pVar = ( ConVar * ) ccb;

		if ( !pVar->IsCompetitiveRestricted() )
			continue;

		// Hack: This var is created by the dxconfig system, but it doesn't actually exist.
		// Skip it so we have no vars change when running a clean config.
		if ( V_stricmp( pVar->GetName(), "r_decal_cullsize" ) == 0 )
			continue;
		
		if ( !pVar->SetCompetitiveMode( true ) )
			anyFailures = true;
	}

	return !anyFailures;
}

void CTFGCClientSystem::ConnectToServer( const char *connect )
{

	Msg("Connecting to %s\n", connect );
	CUtlString connectCmd;
	connectCmd.Format( "connect %s matchmaking", connect );
	if ( engine )
	{
		const bool k_bUseCompetitiveConvars = false;

		bool bAllowed = !k_bUseCompetitiveConvars || ForceCompetitiveConvars();
		if ( !bAllowed )
		{
			// ForceCompetitiveConvars() shouldn't fail
			Assert( 0 );
		}

		engine->ClientCmd_Unrestricted( connectCmd.String() );
		//vgui::surface()->PlaySound( "ui/ui_findmatch_join_01.wav" );
	}
	else
	{
		Warning( "Failed to reconnect to game server as engine wasn't ready\n" );
	}
}
void CTFGCClientSystem::SetWorldStatus( CMsgTFWorldStatus &status )
{
	if ( !BMessagesAreEqual( &m_WorldStatus, &status ) )
	{
		m_WorldStatus = status;

		uint32_t nEngineVer = engine->GetClientVersion();
		uint32_t nWorldVer = status.active_client_version();
		// World version 0 is dev-universe for disabled checking
		bool bOutOfDate = ( nWorldVer > 0 && nEngineVer != nWorldVer );
		if ( !m_bClientOutOfDate && bOutOfDate )
			{ Msg( "Client out of date -- world version %u, current version %u\n", nWorldVer, nEngineVer ); }
		m_bClientOutOfDate = bOutOfDate;

		IGameEvent *pEvent = gameeventmanager->CreateEvent( "world_status_changed" );
		if ( pEvent )
		{
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
}

bool CTFGCClientSystem::BLocalPlayerInventoryHasMvmTicket( void )
{
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv == NULL )
		return false;

	static CSchemaItemDefHandle pItemDef_MvmTicket( CTFItemSchema::k_rchMvMTicketItemDefName );
	if ( !pItemDef_MvmTicket )
		return false;

	for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
	{
		CEconItemView *pItem = pLocalInv->GetItem( i );
		Assert( pItem );
		if ( pItem->GetItemDefinition() == pItemDef_MvmTicket )
			return true;
	}

	return false;
}

int CTFGCClientSystem::GetLocalPlayerInventoryMvmTicketCount( void )
{
	int nCount = 0;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv )
	{
		static CSchemaItemDefHandle pItemDef_MvmTicket( CTFItemSchema::k_rchMvMTicketItemDefName );
		if ( pItemDef_MvmTicket )
		{
			for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
			{
				CEconItemView *pItem = pLocalInv->GetItem( i );
				Assert( pItem );
				if ( pItem->GetItemDefinition() == pItemDef_MvmTicket )
				{
					nCount++;
				}
			}
		}
	}

	return nCount;
}

bool CTFGCClientSystem::BLocalPlayerInventoryHasSquadSurplusVoucher( void )
{
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv == NULL )
		return false;

	static CSchemaItemDefHandle k_rchMvMSquadSurplusVoucherItemDefName( CTFItemSchema::k_rchMvMSquadSurplusVoucherItemDefName );
	if ( !k_rchMvMSquadSurplusVoucherItemDefName )
		return false;

	for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
	{
		CEconItemView *pItem = pLocalInv->GetItem( i );
		Assert( pItem );
		if ( pItem->GetItemDefinition() == k_rchMvMSquadSurplusVoucherItemDefName )
			return true;
	}

	return false;
}

int CTFGCClientSystem::GetLocalPlayerInventorySquadSurplusVoucherCount( void )
{
	int nCount = 0;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv )
	{
		static CSchemaItemDefHandle k_rchMvMSquadSurplusVoucherItemDefName( CTFItemSchema::k_rchMvMSquadSurplusVoucherItemDefName );
		if ( k_rchMvMSquadSurplusVoucherItemDefName )
		{
			for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
			{
				CEconItemView *pItem = pLocalInv->GetItem( i );
				Assert( pItem );
				if ( pItem->GetItemDefinition() == k_rchMvMSquadSurplusVoucherItemDefName )
				{
					nCount++;
				}
			}
		}
	}

	return nCount;
}

bool CTFGCClientSystem::BIsBannedFromMatchmaking( EMMPenaltyPool ePool, CRTime* prtExpireTime /* = NULL */, int* pnDuration /* = NULL */ )
{
	return false;
}


#ifdef USE_MVM_TOUR
bool CTFGCClientSystem::BGetLocalPlayerBadgeInfoForTour( int iTourIndex, uint32 *pnBadgeLevel, uint32 *pnCompletedChallenges )
{
	Assert( iTourIndex >= 0 );
	Assert( iTourIndex < GetItemSchema()->GetMvmTours().Count() );
	Assert( pnBadgeLevel );
	Assert( pnCompletedChallenges );

	*pnBadgeLevel = 0;
	*pnCompletedChallenges = 0;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv == NULL )
		return false;

	// We can't search for a badge without knowing which attribute to look for.
	static CSchemaAttributeDefHandle pAttribDef_MvmChallengeCompleted( CTFItemSchema::k_rchMvMChallengeCompletedMaskAttribName );
	Assert( pAttribDef_MvmChallengeCompleted );
	if ( !pAttribDef_MvmChallengeCompleted )
		return false;

	if ( iTourIndex < 0 || iTourIndex >= GetItemSchema()->GetMvmTours().Count() )
	{
		AssertMsg1( false, "Invalid tour index %d", iTourIndex );
		return false;
	}
	const CEconItemDefinition *pBadgeDef = GetItemSchema()->GetMvmTours()[iTourIndex].m_pBadgeItemDef;
	if ( pBadgeDef == NULL )
	{
		Assert( pBadgeDef );
		return false;
	}

	for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
	{
		CEconItemView *pBadge = pLocalInv->GetItem( i );
		Assert( pBadge );
		if ( pBadge->GetItemDefinition() != pBadgeDef )
			continue;

		if ( !pBadge->FindAttribute( pAttribDef_MvmChallengeCompleted, pnCompletedChallenges ) )
		{
			AssertMsg( false, "Badge missing challenges completed attribute?" );
			*pnCompletedChallenges = 0;
		}

		extern uint32 GetItemDescriptionDisplayLevel( const IEconItemInterface *pEconItem );
		*pnBadgeLevel = GetItemDescriptionDisplayLevel( pBadge );
		return true;
	}

	return false;
}

#endif // USE_MVM_TOUR

void CTFGCClientSystem::AbandonCurrentMatch()
{
	Msg( "Sending request to abandon current match\n" );

	if ( BConnectedToMatchServer( true ) )
	{
		// If we were connected to said match, then disconnect
		Msg( "Disconnecting from abandoned match server\n" );
		engine->ClientCmd_Unrestricted( "disconnect" );
	}
}

bool CTFGCClientSystem::IsConnectStateDisconnected()
{
	return ( m_eConnectState != eConnectState_ConnectingToMatchmade &&
	         m_eConnectState != eConnectState_ConnectedToMatchmade );
}

//-----------------------------------------------------------------------------
// Purpose: Ask the GC for the latest global casual criteria stats
//-----------------------------------------------------------------------------
void CTFGCClientSystem::RequestMatchMakerStats() const
{
}

//-----------------------------------------------------------------------------
// Purpose: Set our cached global casual criteria stats and figure out the most
//			popular map so we can do some health computations later.
//-----------------------------------------------------------------------------
void CTFGCClientSystem::SetMatchMakerStats( const CMsgGCMatchMakerStatsResponse newStats )
{
}

//-----------------------------------------------------------------------------
// Purpose: Given a health ratio, get the health data
//-----------------------------------------------------------------------------
CTFGCClientSystem::MatchMakerHealthData_t CTFGCClientSystem::GetHealthBracketForRatio( float flRatio ) const
{
	CTFGCClientSystem::MatchMakerHealthData_t data;
	data.m_flRatio = flRatio;

	static const Color colorBad( 128, 128, 128, 60 );
	static const Color colorOK( 188, 112, 0, 128 );
	static const Color colorGood( 94, 150, 49, 255 );

	// Walk through our brackets and fine where we fall and setup data accordingly
	if ( flRatio < 0.3f )
	{
		data.m_colorBar = LerpColor( colorBad, colorOK, RemapValClamped( flRatio, 0.2f, 0.3f, 0.f, 1.f ) );
		data.m_strLocToken = "TF_Casual_QueueEstimation_Bad";
	}
	else if ( flRatio < 0.7f )
	{
		data.m_colorBar = LerpColor( colorOK, colorGood, RemapValClamped( flRatio, 0.3f, 0.7f, 0.f, 1.f ) );
		data.m_strLocToken = "TF_Casual_QueueEstimation_OK";
	}
	else
	{
		data.m_colorBar = colorGood;
		data.m_strLocToken = "TF_Casual_QueueEstimation_Good";
	}

	return data;
}


//-----------------------------------------------------------------------------
// Purpose: Really here just so we can shortcircuit some staging_only debug
//-----------------------------------------------------------------------------
inline uint32 GetCountForMap( const CMsgGCMatchMakerStatsResponse msg, int nIndex )
{

	return msg.map_count( nIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Get the overall health of the current local casual criteria.
//			Currently just takes the best individual map health.
//-----------------------------------------------------------------------------
// TODO(Universal Parties): Unused?
CTFGCClientSystem::MatchMakerHealthData_t CTFGCClientSystem::GetOverallHealthDataForLocalCriteria() const
{
	uint32 nMostSearchedCount = m_nMostSearchedMapCount;
	uint32 nLargestOfSelected = 0;
	CCasualCriteriaHelper helper = GTFPartyClient()->GetLocalGroupCriteria().GetCasualCriteriaHelper();


	// No data -- we assume bad
	if ( nMostSearchedCount == 0 )
		return GetHealthBracketForRatio( 0.f );

	// Go through all the locallty selected maps and find the one with the best health.
	// Use that to get our estimated overall criteria health.
	for( int i=0; i < m_MatchMakerStats.map_count_size(); ++i )
	{
		if ( helper.IsMapSelected( i ) )
		{
			nLargestOfSelected = Max( nLargestOfSelected, GetCountForMap( m_MatchMakerStats, i ) );
		}
	}

	return GetHealthBracketForRatio( (float)nLargestOfSelected / (float)nMostSearchedCount );
}

//-----------------------------------------------------------------------------
// Purpose: Gets the health of a given map
//-----------------------------------------------------------------------------
CTFGCClientSystem::MatchMakerHealthData_t CTFGCClientSystem::GetHealthDataForMap( uint32 nMapIndex ) const
{
	uint32 nMostSearchedCount = m_nMostSearchedMapCount;
	uint32 nLargestOfSelected = 0;

	// No data -- we assume bad
	if ( nMostSearchedCount == 0 )
		return GetHealthBracketForRatio( 0.f );

	if ( (int)nMapIndex < m_MatchMakerStats.map_count_size() )
	{
		nLargestOfSelected = GetCountForMap( m_MatchMakerStats, nMapIndex );
	}

	return GetHealthBracketForRatio( (float)nLargestOfSelected / (float)nMostSearchedCount );
}

//-----------------------------------------------------------------------------
// Purpose: Sends an xp acknowledge via k_EMsgGC_AcknowledgeXP to the GC if
//			we have any outstanding xp sources or a mismatch in our last
//			acknowledged xp and our current one.
//-----------------------------------------------------------------------------
void CTFGCClientSystem::AcknowledgePendingRatingAndSources( ETFMatchGroup eMatchGroup )
{
}

void CTFGCClientSystem::AcknowledgeNotification( uint32 nAccountID, uint64 ulNotificationID )
{
}

//-----------------------------------------------------------------------------
// Purpose: Hang onto the survey request
//-----------------------------------------------------------------------------
void CTFGCClientSystem::SetSurveyRequest( const CMsgGCSurveyRequest& msgSurveyRequest )
{
	m_msgSurveyRequest = msgSurveyRequest;
}

//-----------------------------------------------------------------------------
// Purpose: Send survey response and clear the stored survey request
//-----------------------------------------------------------------------------
void CTFGCClientSystem::SendSurveyResponse( int32 nResponse )
{
}

void CTFGCClientSystem::ClearSurveyRequest()
{
	m_msgSurveyRequest.Clear();
}

void CTFGCClientSystem::AddLocalPlayerSOListener( ISharedObjectListener* pListener, bool bImmediately )
{
	if ( bImmediately )
	{
		SubscribeToLocalPlayerSOCache( pListener );
	}
	else
	{
		m_vecDelayedLocalPlayerSOListenersToAdd.AddToTail( pListener );
	}
}

void CTFGCClientSystem::RemoveLocalPlayerSOListener( ISharedObjectListener* pListener )
{
	// Remove if it was a delayed add
	auto idx = m_vecDelayedLocalPlayerSOListenersToAdd.Find( pListener );
	if ( idx != m_vecDelayedLocalPlayerSOListenersToAdd.InvalidIndex() )
	{
		m_vecDelayedLocalPlayerSOListenersToAdd.Remove( idx );
	}

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GetGCClient()->RemoveSOCacheListener( steamID, pListener );
	}
}

bool CTFGCClientSystem::BConnectedToMatchServer( bool bLiveMatch )
{
	return false;
}

bool CTFGCClientSystem::BHaveRunningMatch() const
{
	return false;
}

bool CTFGCClientSystem::BHaveLiveMatch() const
{
	return false;
}

EAbandonGameStatus CTFGCClientSystem::GetAssignedMatchAbandonStatus()
{
	return k_EAbandonGameStatus_Safe;
}

ETFMatchGroup CTFGCClientSystem::GetLiveMatchGroup() const
{

	return k_eTFMatchGroup_Invalid;
}

void CTFGCClientSystem::JoinMMMatch()
{
}

//-----------------------------------------------------------------------------
CTFGCClientSystem::MatchInvite_t CTFGCClientSystem::GetMatchInvite( int idx ) const
{
	auto *pTypeCache = m_pSOCache ? m_pSOCache->FindBaseTypeCache( CTFLobbyInvite::k_nTypeID ) : nullptr;
	int nInvites = pTypeCache ? pTypeCache->GetCount() : 0;

	if ( idx < 0 || idx >= nInvites )
	{
		AssertMsg( false, "Invalid index passed to GetMatchInvite" );
		return MatchInvite_t{ 0u, k_eTFMatchGroup_Invalid, false };
	}

	CTFLobbyInvite *pInvite = static_cast< CTFLobbyInvite * >( pTypeCache->GetObject( idx ) );
	PlayerGroupID_t nLobbyID = pInvite->GetGroupID();
	ETFMatchGroup eMatchGroup = pInvite->GetMatchGroup();
	bool bAcceptingThisMsg = ( m_nAcceptingMatchLobbyID == nLobbyID );
	return MatchInvite_t{ nLobbyID, eMatchGroup, bAcceptingThisMsg };
}

//-----------------------------------------------------------------------------
int CTFGCClientSystem::GetMatchInviteIdxByLobbyID( PlayerGroupID_t nLobbyID ) const
{
	int nNumInvites = GetNumMatchInvites();
	for( int idxInvite = 0; idxInvite < nNumInvites; ++idxInvite )
	{
		auto invite = GetMatchInvite( idxInvite );
		if ( invite.nLobbyID == nLobbyID )
			{ return idxInvite; }
	}

	return -1;
}

//-----------------------------------------------------------------------------
int CTFGCClientSystem::GetNumMatchInvites() const
{
	if ( !m_pSOCache )
		{ return 0; }

	CSharedObjectTypeCache *pTypeCache = m_pSOCache->FindBaseTypeCache( CTFLobbyInvite::k_nTypeID );
	return pTypeCache ? pTypeCache->GetCount() : 0;
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::RequestAcceptMatchInvite( PlayerGroupID_t nLobbyID )
{
}

//-----------------------------------------------------------------------------
bool CTFGCClientSystem::BIsMatchGroupDisabled( ETFMatchGroup eMatchGroup ) const
{
	for ( int i = 0; i < WorldStatus().disabled_match_groups_size(); i++ )
	{
		if ( WorldStatus().disabled_match_groups( i ) == eMatchGroup )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::OnAcceptLobbyReply( PlayerGroupID_t )
{
	Assert( m_nAcceptingMatchLobbyID != 0u );
	m_nAcceptingMatchLobbyID = 0u;
	// Update invites with whatever happened (the invite will be gone, or we will have failed and they'll want to see
	// that bAccepting went back to false)
	//
	// We also suspend updating invites while an accept is in flight.
	OnMatchInvitesUpdated();
}

//-----------------------------------------------------------------------------
void CTFGCClientSystem::OnMatchInvitesUpdated()
{
	// Suppress while accept message in flight, OnAcceptLobbyReply will fire
	if ( m_nAcceptingMatchLobbyID )
		{ return; }

	IGameEvent *event = gameeventmanager->CreateEvent( "match_invites_updated" );
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

void CTFGCClientSystem::LeaveGameAndPrepareToJoinParty( GCSDK::PlayerGroupID_t nPartyID )
{
	// Leave current server
	engine->ClientCmd_Unrestricted( "disconnect" );
}

bool CTFGCClientSystem::BIsPhoneVerified( void )
{
	return false;
}

bool CTFGCClientSystem::BIsPhoneIdentifying( void )
{
	return false;
}

bool CTFGCClientSystem::BHasCompetitiveAccess( void )
{
	return false;
}


