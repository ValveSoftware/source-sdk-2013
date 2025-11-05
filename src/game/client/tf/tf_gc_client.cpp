//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "tf_gc_client.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_gcmessages.h"
#include "kvpacker.h"
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
#include "tf_notification.h"
#include "c_tf_notification.h"
#include "tf_gc_shared.h"
#include <tf_proto_script_obj_def.h>
#include "econ_item_description.h"

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

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTFGCClientSystem::CTFGCClientSystem( void )
{
	// replace base GCClientSystem
	SetGCClientSystem( this );
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
	// Let SDR know that we will likely want access to the relay network, so we're more
	// likely to have initial ping data to the clusters ready by the time we ask for it
	if ( SteamNetworkingUtils() )
	{
		SteamNetworkingUtils()->InitRelayNetworkAccess(); 
	}

	return true;
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