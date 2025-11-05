//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "tf_gc_server.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_gcmessages.h"
#include "tf_player.h"
#include "rtime.h"
// XXX(JohnS): Eventually, we want to send a smaller lobby object to clients. For now, they use the CTFGSLobby, which is
//             in shared code for that reason.
#include "tf_gamerules.h"
#include "eiface.h"
#include "cdll_int.h"
#include "econ_item_inventory.h"
#include "gameinterface.h"
#include "client.h"
#include "tier1/convar.h"
#include "util.h"
#include "tier1/utlqueue.h"
#include "tf_player_resource.h"
#include "tf_gc_shared.h"
#include "iserver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

static const char* GetWebBaseUrl()
{
	switch ( GetUniverse() )
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

#ifdef ENABLE_GC_MATCHMAKING

/***********************************************************************************************************************
////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

   XXX(JohnS) NOTE The current state of the matchmaking flow through this class is a bit of a mess.  Have been
                   incrementally cleaning things up, but be careful.

   UpdateConnectedPlayersAndServerInfo()
     This is the heug god function that sync's our state with the GC's state via the Lobby shared object:
     - Our actual connected players

     - m_pMatchInfo (via GetMatch()) - this represents our match in progress, and should generally mirror the GC, but
       *MIGHT NOT*.  For instance, when the GC is unavailable this object is locked, and when the GC returns we may be
       desync'd. This function is in charge of managing that.  Outside code should simply look at the MatchInfo object
       and trust that it is the state of the match.

     - m_vecReservationExpiryTime - this should be merged into MatchInfo eventually, but is an array of active
       reservations and when they expire.  This isn't in MatchInfo because in some modes we operate with reservations
       but without running a proper Match.  When we're running a match, anyone in this vector should be in the MatchInfo

     - CTFGSLobby - This is the shared object from the server that represents the match we are hosting.  However, it is
       *NOT* the article of record on the match.  This is due to matches being designed to be resilient to GC connection
       loss.  Essentially, only this function should be looking at CTFGSLobby and negotiating the state of the actual
       match it believes itself to have in MatchInfo.

   == Gameserver / GC Authority
      - GC forms matches, adds players to matches, passes them to servers
      - Servers run matches to completion, have authority on abandons/etc. regardless of GC state
      - Servers pass result, including any abandons, to GC.  Message is queued if GC is unavailable.
      - GC takes match results and does ELO calculation and any stats/etc.
      - GC can request players be kicked from matches or matches be canceled
      - If more players are needed
        - Gameserver requests GC attention with appropriate flag (6v6: Stalled, waiting on complete match, 12v12:
          Non-full match)
        - GC adds players to lobby, making them part of the match
      - If server state is poor (hypothetically: lag, too many abandons, abnormal something or other)
        - Game server sends KickLobby to terminate match, sends failed match result
      - If GC is unavailable
        - Game server still carries out duties, may decide to make changes like end match instead of request late joins
          if it decides GC wont be able to provide them.

   == Match Start
      - GC creates a lobby and hands it to us. UpdateConnectedPlayers tick initializes a MatchInfo struct as
        appropriate, accepts players.

   == Adding Players
      - The GC adds players to the lobby (so, when GC down, matches cannot gain players)
      - UpdateConnectedPlayersAndServerInfo ensures that makes sense (it should, though, we no longer have legacy match
        types where the GC adds players we shouldn't accept)
      - UpdateConnectedPlayers calls AcceptGCReservation, player is added to match and put in reservation list

   == Dropping Players
      - Case 1: Player is not present, but is in the lobby (GC *might* be down, doesn't matter)
        - Player marked missing in MatchInfo by UpdateConnectedPlayers tick
        - After a grace period, player marked dropped, as an abandoner in MatchInfo
        - PlayerLeftMatch message is sent to tell the GC about their leaving.
      - Case 2: Player is dropped from GC lobby
        - UpdateConnectedPlayers assumes GC kicked them, marks them dropped from match and kicks them.
        - TODO: Ideally there'd be a KickThisGuy GC message, and we'd respond with PlayerLeftMatch, rather than the GC
          unilaterally dropping people like this.
      - Case 3: Votekicked
        - Vote system sends us PlayerRequestVoteKick
        - We decide whether or not to allow it, possibly asking the GC
        - Vote system tells us results in SubmitVoteKickResults
        - We tell GC results, it decides whether to drop player from match
        - We see them vanish from the lobby (like Case 2) and get a response indicating what it did for our records.
      - All cases:
        - A reliable GC message player-abandoned (or was kicked or never joined) message queued to reconcile this with
          the lobby state, but if GC is unavailable it will be informed when it returns.
        - Player is marked dropped in MatchInfo

   == Team Assignments
      - The GC delivers an initial team assignment for each player added to the match.  This team assignment does not
        change when game teams change sides, see TFGameRules::GameTeamToLobbyTeam and its inverse to map these to game
        logic teams (vs TF_GC_TEAM objects)

      - All other team changes have to be initiated by a game server message, in modes that allow it, to prevent
        race-conditions.

        - The NewMatchForLobby message expects the GC to shuffle our teams.  We prevent races by not issuing other team
          change messages while this message is pending.  If we time out waiting for the GC, some modes may start a
          speculative server-created match (expecting the GC to come back and respond to that message positively).  In
          this case, we queue a ChangeMatchPlayerTeams message to stomp any assignments back to our known state,
          allowing us to ignore the temporary de-sync (queued messages always get processed in sequence)

        - The ChangeMatchPlayerTeams message allows the gameserver to change match player teams mid-game in match modes
          that allow it.  The game server is in charge of not queuing this message in parallel with NewMatchForLobby
          above, or handling the potential race.

        - When processing either of these messages, the GC cancels any players that are awaiting acceptance by the
          game-server, and re-tries if necessary.  This prevents team changes from racing with player-joins which may
          have been predicated on differing team layouts.
          - The game server does not accept pending players or send any heartbeats until any queued messages have been
            responded to.  See Queued Messages below.

   == Match End
      - Match result message provides canonical record of match, is queued to send to GC when available.
      - GameServerKickingLobby message dissolves live match if GC is available/tracking it. Queued similarly.
        - ** This can happen before or after the match result.
        - In MvM, we send potentially multiple victory messages per match -- they can cycle missions and keep winning.
        - As of right now, in competitive, we end the match coincident with sending a match result.
      - Match ended doesn't necessarily kick players, so a dead/finished match will stick around on our end until
        everyone Disconnects, (or the game logic kicks them, e.g. MatchInfo->BEnded + a timeout)
      - Ended matches have queued a message to dissolve their lobby, though, so further GC interaction with the match is
        not possible, and players are allowed to leave (since they're now allowed to be put in a new match by the GC)

   == Queued Messages And Match State And Race Conditions
      - Since queued messages are sent in order until confirmed, the GC will always see (eventually) a coherent
        story. For instance:
        - PlayerLeftMatch - GC marks this player as leaving match
        - KickingLobby - GC marks match as finished, result pending
        - MatchResult (minus the two players who left) - GC finishes match accounting, marks match complete, missing
          players are already noted as leavers so their absence from the result is expected.
      - While messages are queued, we do not run the UpdateConnectedPlayers() think. This prevents having to worry about
        a fractal of potential edge cases -- we don't look at updated lobby data or send heartbeats while anything we're
        trying to tell the GC hasn't been confirmed.  This also means we won't send a heartbeat until all such actions
        have been confirmed.
        - GC message handlers for queued messages do have to handle possible races -- if the GC sends us players while
          we're sending a "Reassign Player Team" message, this behavior means we'll stubbornly wait for a response to
          the team message before acknowledging any players, allowing the GC to easily resolve the race (in this case,
          by canceling or retrying any attempted add-player-match actions)

   == Gameserver Crashes
      - If GC is available, it handles it, otherwise, match is lost.  Gameservers don't currently try to persist this
        state.

   == Match empties out
      - If the match is still going, it should reach ended as everyone in it gets timed out as an abandon.
      - If the GC is around, it will revoke the lobby once we inform it everyone has dropped.
      - Once the match is marked ended, and the GC concurs and deletes the lobby, we delete MatchInfo
        - If the GC is not around, we hang out on the completed match state until it is.  We can't exactly take new
          matches in the mean time. (but, see k_InvalidState_Timeout_With_Match)

\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\////////////////////////////////////////////////////////////
***********************************************************************************************************************/

//ConVar dota_force_upload_match_stats( "dota_force_upload_match_stats", "0", FCVAR_CHEAT, "If enabled, server will upload match stats even when there aren't human players on each side" );
//extern ConVar dota_force_bot_cycle;
extern CServerGameDLL g_ServerGameDLL;

ConVar tf_allow_server_hibernation( "tf_allow_server_hibernation", "1", FCVAR_NONE, "Allow the server to hibernate when empty." );

//DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_CONSOLE, "Console" );

static CTFGCServerSystem s_TFGCServerSystem;
CTFGCServerSystem *GTFGCClientSystem() { return &s_TFGCServerSystem; }

//bool g_bServerReceivedGCWelcome = false;
int g_gcServerVersion = 0; // Version from the GC

static bool g_bWarnedAboutMaxplayersInMVM = false;

//-----------------------------------------------------------------------------
CTFGCServerSystem::CTFGCServerSystem()
	: m_iSavedVisibleMaxPlayers( -1 )
	, m_bOverridingVisibleMaxPlayers( false )
{
	// replace base GCClientSystem
	SetGCClientSystem( this );

	m_bSetupSchema = false;
	m_timeLastConnectedToGC = 0.f;

	g_bWarnedAboutMaxplayersInMVM = false;
}


CTFGCServerSystem::~CTFGCServerSystem( void )
{
	// Prevent other system from using this pointer after it's destroyed
	SetGCClientSystem( NULL );
}


bool CTFGCServerSystem::Init()
{
	g_bWarnedAboutMaxplayersInMVM = false;
	return true;
}

bool CTFGCServerSystem::ShouldHibernate()
{
	// We only hibernate if we're just sitting there with a freshly loaded map
	return ( engine->IsDedicatedServer() && tf_allow_server_hibernation.GetBool() && !BPendingReliableMessages() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGCServerSystem::PreInitGC()
{
	BaseClass::PreInitGC();

	if ( !m_bSetupSchema )
	{
		m_bSetupSchema = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGCServerSystem::PostInitGC()
{
	BaseClass::PostInitGC();
}

//-----------------------------------------------------------------------------
void CTFGCServerSystem::Shutdown()
{
	BaseClass::Shutdown();

	m_mapEquipmentRequests.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
void CTFGCServerSystem::PreClientUpdate( )
{
	BaseClass::PreClientUpdate();

	CRTime::UpdateRealTime();

	WebapiEquipmentThink();

	if ( GCClientSystem()->BConnectedtoGC() )
	{
		m_timeLastConnectedToGC = Plat_FloatTime();
	}

	// Check for slamming visiblemaxplayers
	static ConVarRef sv_visiblemaxplayers( "sv_visiblemaxplayers" );
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// Abort the server if they don't have enough maxplayers
		if ( gpGlobals->maxClients < 32 )
		{
			if( !g_bWarnedAboutMaxplayersInMVM )
			{
				// Prevent this warning from endlessly spamming the console...
				g_bWarnedAboutMaxplayersInMVM = true;
				Warning( "You must set maxplayers to 32 to host Mann vs. Machine\n" );
			}

			if ( engine->IsDedicatedServer() )
			{
				engine->ServerCommand( "exit\n" );
			}
			return;
		}

		// This changes what the server browser displays
		// update sv_visiblemaxplayers for MvM, count only non-bot spectators
		CUtlVector<CTFPlayer *> spectatorVector;
		CollectPlayers( &spectatorVector, TEAM_SPECTATOR );
		int spectatorCount = 0;
		FOR_EACH_VEC ( spectatorVector, iIndex )
		{
			if ( !spectatorVector[iIndex]->IsBot() && !spectatorVector[iIndex]->IsReplay() && !spectatorVector[iIndex]->IsHLTV() )
			{
				spectatorCount++;
			}
		}

		int playerCount = tf_mvm_defenders_team_size.GetInt() + spectatorCount;
		if ( sv_visiblemaxplayers.GetInt() <= 0 || sv_visiblemaxplayers.GetInt() != playerCount )
		{
			MMLog( "Setting sv_visiblemaxplayers to %d for MvM\n", playerCount );

			// save off visible players
			if ( !m_bOverridingVisibleMaxPlayers )
			{
				m_bOverridingVisibleMaxPlayers = true;
				m_iSavedVisibleMaxPlayers = sv_visiblemaxplayers.GetInt();
			}

			sv_visiblemaxplayers.SetValue( playerCount );
		}
	}
	else
	{
		// Not in MvM.  Check for restoring sv_visiblemaxplayers
		if ( m_bOverridingVisibleMaxPlayers )
		{
			MMLog( "Restoring sv_visiblemaxplayers to %d\n", m_iSavedVisibleMaxPlayers );
			sv_visiblemaxplayers.SetValue( m_iSavedVisibleMaxPlayers );
			m_bOverridingVisibleMaxPlayers = false;
			m_iSavedVisibleMaxPlayers = -1;
		}
	}
}

// Backoff api
void CTFGCServerSystem::WebapiEquipmentState_t::Backoff()
{
	if ( m_nBackoffSec == 0 )
		m_nBackoffSec = 20;
	else
		m_nBackoffSec = ( m_nBackoffSec * 12 + 9 ) / 10; // exponential backoff @ 1.2x factor, round up

	m_rtNextRequest = CRTime::RTime32TimeCur() + m_nBackoffSec;
}

void CTFGCServerSystem::WebapiEquipmentState_t::RequestSucceeded()
{
	m_rtNextRequest = 0;
	m_nBackoffSec = 0;
}

bool CTFGCServerSystem::WebapiEquipmentState_t::IsBackingOff()
{
	return m_rtNextRequest != 0 && CRTime::RTime32TimeCur() <= m_rtNextRequest;
}

CTFGCServerSystem::WebapiEquipmentState_t& CTFGCServerSystem::FindOrCreateWebapiEquipmentState( CSteamID steamID )
{
	TMapEquipmentRequests::IndexType_t unEquipmentRequest = m_mapEquipmentRequests.Find( steamID );
	if ( unEquipmentRequest == m_mapEquipmentRequests.InvalidIndex() )
	{
		WebapiEquipmentState_t* pNewState = new WebapiEquipmentState_t;
		pNewState->m_ownerID = steamID;
		unEquipmentRequest = m_mapEquipmentRequests.Insert( steamID, pNewState );
	}

	return *m_mapEquipmentRequests.Element( unEquipmentRequest );
}

void CTFGCServerSystem::WebapiEquipmentThink()
{
	FOR_EACH_MAP_FAST( m_mapEquipmentRequests, i )
	{
		WebapiEquipmentThinkRequest( m_mapEquipmentRequests.Key( i ), m_mapEquipmentRequests.Element( i ) );
	}
}
void CTFGCServerSystem::WebapiEquipmentThinkRequest( CSteamID steamID, WebapiEquipmentState_t* pState )
{
	Assert( pState );
	WebapiEquipmentState_t& state = *pState;

	// If we are waiting on timer/rate limit, don't do anything
	if ( state.IsBackingOff() )
		return;

	switch( state.m_eState )
	{
	case kWebapiEquipmentState_Init:
		// Safe to return to this state from anywhere, let's clean up any in-flight data

		// Remove any existing current request
		if ( state.m_pKVCurrentRequest != nullptr )
		{
			state.m_pKVCurrentRequest->deleteThis();
			state.m_pKVCurrentRequest = nullptr;
		}

		state.m_EquipmentRequestCompleted.Cancel();

		if ( state.m_hEquipmentRequest != INVALID_HTTPREQUEST_HANDLE )
		{
			SteamHTTP()->ReleaseHTTPRequest( state.m_hEquipmentRequest );
			state.m_hEquipmentRequest = INVALID_HTTPREQUEST_HANDLE;
		}

		state.m_eState = kWebapiEquipmentState_WaitingForClientRequest;
		// fallthrough;

	case kWebapiEquipmentState_WaitingForClientRequest:
	{
		Assert( state.m_pKVCurrentRequest == nullptr );
		if ( state.m_pKVNextRequest == nullptr )
			return;

		V_swap( state.m_pKVCurrentRequest, state.m_pKVNextRequest );

		state.m_eState = kWebapiEquipmentState_RequestInventory;
		// fallthrough
	}

	case kWebapiEquipmentState_RequestInventory:
	{
		Assert( state.m_pKVCurrentRequest != nullptr );
		KeyValues* pKV = state.m_pKVCurrentRequest;

		if ( !SteamHTTP() )
			return;

		// Request inventory from teamfortress.com webapi
		CFmtStr strUrl( "%swebapi/ISDK/GetEquipment/v0001", GetWebBaseUrl() );

		state.m_EquipmentRequestCompleted.Cancel();
		state.m_hEquipmentRequest = SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, strUrl.Get() );
		if ( state.m_hEquipmentRequest == INVALID_HTTPREQUEST_HANDLE )
		{
			// try again next frame
			return;
		}

		// This mod's appid (NOT tf2's appid)
		SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hEquipmentRequest, "appid", CNumStr( engine->GetAppID() ) );

		// Item list
		SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hEquipmentRequest, "msg", pKV->GetString( "msg", nullptr ) );

		// Authentication token
		SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hEquipmentRequest, "ticket", pKV->GetString( "ticket", nullptr ) );

		if ( GetUniverse() != k_EUniversePublic )
		{
			// use beta tf2 appid on non public universes
			SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hEquipmentRequest, "game_appid", "810" );
		}

		// Is there a way we can validate the existing so cache?  We could only request the new items.
		// Right now we only expect this message rarely so let's just ask for all the items each time.
		//
		// Since our cache is a subset of the full cache, we can't really use version# caching unless
		// the set of items we are requesting is identical.  Maybe it'd be worth it to add "new" and
		// "known" items to the API request and only version-cache the known ones?

		//CGCClientSharedObjectCache* pExistingSOCache = GetSOCache( steamID );
		//if ( pExistingSOCache && pExistingSOCache->BIsSubscribed() )
		//{
		//	SteamHTTP()->SetHTTPRequestGetOrPostParameter( state.m_hInventoryRequest, "version", CNumStr( pExistingSOCache->GetVersion() ) );
		//}

		SteamAPICall_t callResult;
		if ( !SteamHTTP()->SendHTTPRequest( state.m_hEquipmentRequest, &callResult ) )
		{
			state.Backoff();
			return;
		}

		state.m_EquipmentRequestCompleted.Set( callResult, pState, &WebapiEquipmentState_t::OnWebapiEquipmentReceived );
		state.m_eState = kWebapiEquipmentState_WaitingForInventory;
		break;
	}

	case kWebapiEquipmentState_WaitingForInventory:
		// nothing to do until steam callback completes
		break;

	case kWebapiEquipmentState_InventoryReceived:
		// No need to keep the current request around.
		if ( state.m_pKVCurrentRequest )
		{
			state.m_pKVCurrentRequest->deleteThis();
			state.m_pKVCurrentRequest = nullptr;
		}

		// Don't allow spamming this api -- wait 20 seconds before we ask gc for items again
		state.RequestSucceeded();
		state.Backoff();
		state.m_eState = kWebapiEquipmentState_WaitingForClientRequest;
		break;

	case kWebapiEquipmentState_NotifyClientOfFailure:
		if ( !TFGameRules() )
			return;

		TFGameRules()->RequestClientInventory( steamID );
		state.m_eState = kWebapiEquipmentState_Init; // reset everything
		break;
	}
}

void CTFGCServerSystem::ProcessPlayerInventoryRequest( CSteamID steamID, KeyValues* pKVRequest )
{
	WebapiEquipmentState_t& state = FindOrCreateWebapiEquipmentState( steamID );

	// If they have a pending request we haven't acted on, it's now stale.
	if( state.m_pKVNextRequest )
	{
		state.m_pKVNextRequest->deleteThis();
		state.m_pKVNextRequest = nullptr;
	}

	// Clone off their existing request for processing
	state.m_pKVNextRequest = pKVRequest->MakeCopy();
}

void CTFGCServerSystem::WebapiEquipmentState_t::OnWebapiEquipmentReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure )
{
	GTFGCClientSystem()->OnWebapiEquipmentReceived( m_ownerID, pInfo, bIOFailure );
}

void CTFGCServerSystem::OnWebapiEquipmentReceived( CSteamID steamID, HTTPRequestCompleted_t* pInfo, bool bIOFailure )
{
	WebapiEquipmentState_t& state = FindOrCreateWebapiEquipmentState( steamID );
	if ( state.m_eState != kWebapiEquipmentState_WaitingForInventory )
		return;

	// Assume failure, we'll correct this change if we succeeded
	state.Backoff();
	state.m_eState = kWebapiEquipmentState_RequestInventory;

	if ( !SteamHTTP() )
		return;

	if( bIOFailure || !pInfo || state.m_hEquipmentRequest != pInfo->m_hRequest )
	{
		Assert( false );
		if( state.m_hEquipmentRequest != INVALID_HTTPREQUEST_HANDLE )
		{
			SteamHTTP()->ReleaseHTTPRequest( state.m_hEquipmentRequest );
		}
		return;
	}

	// request failed -- backoff and retry
	if ( !pInfo->m_bRequestSuccessful || pInfo->m_eStatusCode != k_EHTTPStatusCode200OK )
	{
		SteamHTTP()->ReleaseHTTPRequest( state.m_hEquipmentRequest );
		return;
	}

	// Extract the result
	uint32 unBytes;
	Verify( SteamHTTP()->GetHTTPResponseBodySize( pInfo->m_hRequest, &unBytes ) );
	CUtlBuffer bufInventory;
	bufInventory.EnsureCapacity( unBytes );
	bufInventory.SeekPut( CUtlBuffer::SEEK_HEAD, unBytes );
	Verify( SteamHTTP()->GetHTTPResponseBodyData( pInfo->m_hRequest, ( uint8* )bufInventory.Base(), unBytes ) );

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

	case k_EResultValueOutOfRange:
		// client gave us garbage?  Let's give them the benefit of the doubt and try again.
		state.m_eState = kWebapiEquipmentState_NotifyClientOfFailure;
		return;

	case k_EResultNotLoggedOn:
		// Ticket didn't authenticate successfully, ask them to send us a new one
		state.m_eState = kWebapiEquipmentState_NotifyClientOfFailure;
		return;

	default:
	{
		CUtlString strError;
		pValues->GetChildStringValue( strError, "error", "" );
		Warning( "Received unexpected result code %d attempting to retrieve equipment for %s. (%s)\n", nResult, steamID.Render(), strError.Get() );
		return;
	}
	}

	// Parse the inventory message
	CSteamID resultSteamID( pValues->GetChildUInt64Value( "steamID" ) );
	if ( resultSteamID != steamID )
	{
		Warning( "Equipment response has bad owner steam id (%s, expected %s)\n", resultSteamID.Render(), steamID.Render() );
		return;
	}

	if ( pValues->FindChild( "msg" ) )
	{
		CUtlBuffer bufMsgSubscription;
		if ( !pValues->BGetChildBinaryValue( bufMsgSubscription, "msg" ) )
		{
			Warning( "Equipment response failed to extract inventory msg\n" );
			return;
		}

		CGCClientSharedObjectCache *pSOCache = GetGCClient()->AddLocalSOCache( steamID, bufMsgSubscription.Base(), bufMsgSubscription.TellPut() );
		if ( !pSOCache )
		{
			Warning( "Inventory response failed to create SO cache (probably protobuf didn't parse)\n" );
			return;
		}

		SDK_ApplyInventoryInfo( pSOCache, state.m_pKVCurrentRequest );

		// Version should match the one they said we have
		Assert( pSOCache->GetVersion() == pValues->GetChildUInt64Value( "version" ) );
	}
	else
	{
		// this is a weird response, the server doesn't currently send socache versions and yet we didn't get the result
		Warning( "Inventory response missing inventory msg\n" );
	}

	// We were successful, clear backoff timers
	state.RequestSucceeded();
	state.m_eState = kWebapiEquipmentState_InventoryReceived;
}

void CTFGCServerSystem::SDK_ApplyInventoryInfo(CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest)
{
	// update any data from the client to match SDK_AddServerInventoryInfo

	SDK_ApplyLocalLoadout(pCache, pKVRequest);
}

void CTFGCServerSystem::SDK_ApplyLocalLoadout(CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest)
{
	CGCClientSharedObjectTypeCache* pItemCache = pCache->FindTypeCache(CEconItem::k_nTypeID);
	if (!pItemCache)
		return;

	CSteamID playerSteamID = pCache->GetOwner();
	CTFPlayerInventory* pTFInventory = dynamic_cast<CTFPlayerInventory*>(InventoryManager()->GetInventoryForAccount(playerSteamID.GetAccountID()));
	if (!pTFInventory)
		return;
	
	// Mark everything as unequipped.
	for (uint32 i = 0; i < pItemCache->GetCount(); ++i)
	{
		CEconItem* pItem = (CEconItem*)pItemCache->GetObject(i);
		if (!pItem)
			continue;
		pTFInventory->UnequipLocal(pItem->GetID());
		pItem->Unequip();
	}

	// Extract loadout information from the keyvalues and apply it to each item.
	KeyValues* pLoadoutKV = pKVRequest->FindKey("local_loadout");
	if (!pLoadoutKV)
	{
		Warning("Failed to find a loadout in SDK inventory message.\n");
		return;
	}

	FOR_EACH_TRUE_SUBKEY(pLoadoutKV, pClassKey)
	{
		const char *pszClass = pClassKey->GetName();
		const int iClass = V_atoi(pszClass);

		FOR_EACH_SUBKEY(pClassKey, pLoadoutEntry)
		{
			const int iSlot = V_atoi(pLoadoutEntry->GetName());
			const itemid_t uItemId = pLoadoutEntry->GetUint64();

			if (uItemId == INVALID_ITEM_ID || uItemId == 0)
				continue;

			CEconItem soIndex;
			soIndex.SetItemID(uItemId);

			CEconItem* pItem = (CEconItem*) pItemCache->FindSharedObject(soIndex);
			if (pItem) {
				pTFInventory->EquipLocal(uItemId, iClass, iSlot);
			}
			else {
				Warning("Failed to find item %llu in shared object, but client says it should be equipped by [%i] in slot [%i].\n", uItemId, iClass, iSlot);
			}
		}
	}
}

#endif // #ifdef ENABLE_GC_MATCHMAKING
