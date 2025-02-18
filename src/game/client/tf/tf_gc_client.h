//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef _INCLUDED_TF_GC_CLIENT_H
#define _INCLUDED_TF_GC_CLIENT_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/steam_api.h"
#endif

//#include "dota_gc_common.h"
#include "gcsdk/gcclientsdk.h"
//#include "dota_gamerules.h"
#include "tf_gcmessages.pb.h"
#include "../clientsteamcontext.h"
#include "gc_clientsystem.h"
#include "GameEventListener.h"
#include "tf_quickplay_shared.h"
#include "confirm_dialog.h"
#include "econ_game_account_client.h"
#include "tf_matchmaking_shared.h"
#include "netadr.h"
#include "tf_gc_shared.h"

class CTFParty;
class CTFPartyClient;
class CTFGSLobby;
class CMvMMissionSet;
class IMatchJoiningHandler;
class CTFGroupMatchCriteria;
class CTFPerPlayerMatchCriteria;
class CReliableMessageQueue;
//class CDOTAGameAccountClient;
//class CDOTABetaParticipation;

#if !defined( TF_GC_PING_DEBUG ) && ( defined( STAGING_ONLY ) || defined( _DEBUG ) )
	#define TF_GC_PING_DEBUG
#endif

namespace GCSDK
{
	typedef uint64 PlayerGroupID_t;
}

enum EAbandonGameStatus
{
	k_EAbandonGameStatus_Safe,					//< It's totally safe to leave
	k_EAbandonGameStatus_AbandonWithoutPenalty,	//< Leaving right now would be considered "abandoning", but there will be no penalty right now
	k_EAbandonGameStatus_AbandonWithPenalty,	//< Leaving right now would be considered "abandoning", and you will be penalized
};
static const EAbandonGameStatus k_EAbandonGameStatus_Newest = k_EAbandonGameStatus_AbandonWithPenalty;

class CLoalPlayerSOCacheListener;

class CSendCreateOrUpdatePartyMsgJob;

class CTFGCClientSystem : public CGCClientSystem, public GCSDK::ISharedObjectListener, public CGameEventListener
{
	friend class CTFMatchmakingPopup;
	friend class CLoalPlayerSOCacheListener;
	friend class CSendCreateOrUpdatePartyMsgJob;
	DECLARE_CLASS_GAMEROOT( CTFGCClientSystem, CGCClientSystem );
public:
	CTFGCClientSystem( void );
	~CTFGCClientSystem( void );

	// CAutoGameSystemPerFrame
	virtual bool Init() OVERRIDE;
	virtual void PostInit() OVERRIDE;
	virtual void LevelInitPreEntity() OVERRIDE;
	virtual void LevelShutdownPostEntity() OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual void Update( float frametime ) OVERRIDE;

	// Force discard all current ping data, forcing it to be refreshed, and causing BHavePingData to be false until it
	// completes.
	//
	// Normally, the client think will idly refresh this data, so this is only valuable for debug or cases where we know
	// the network changed and our previous data is worse than no data.
	void InvalidatePingData();

	bool BHavePingData() { return false; }
	// If !BHavePingData() this will have no datacenters in it.
	CMsgGCDataCenterPing_Update GetPingData() { return m_msgCachedPingUpdate; }

	// ISharedObjectListener
	virtual void	SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void	SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void	SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { m_pSOCache = NULL; }

	// IGameEventListener2
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	enum SOChangeType_t
	{
		SOChanged_Create,
		SOChanged_Update,
		SOChanged_Destroy
	};
	void SOChanged( const GCSDK::CSharedObject *pObject, SOChangeType_t changeType,  GCSDK::ESOCacheEvent eEvent );

//	void SetCurrentMatchID( uint32 unMatchID ) { m_unCurrentMatchID = unMatchID; }
//	uint32 GetCurrentMatchID() { return m_unCurrentMatchID; }

	void OnGCUserSessionCreated() {  }
	bool HasGCUserSessionBeenCreated();

//	CDOTAGameAccountClient* GetGameAccountClient();
//	void DumpGameAccountClient();
//	CDOTABetaParticipation* GetBetaParticipation();
//	void DumpBetaParticipation();

	//
	// Matchmaking
	//

	// Pending invites to matches.
	struct MatchInvite_t
	{
		PlayerGroupID_t nLobbyID;
		ETFMatchGroup eMatchGroup;
		// If we have an in-flight accept message for this invite. Might toggle back to false if message is lost.
		bool bSentAcceptMsg;
	};
	MatchInvite_t GetMatchInvite( int idx ) const;
	int GetMatchInviteIdxByLobbyID( PlayerGroupID_t nLobbyID ) const;
	int GetNumMatchInvites() const;
	void RequestAcceptMatchInvite( PlayerGroupID_t nLobbyID );

	// Is this match group temporarily disabled by the system
	bool BIsMatchGroupDisabled( ETFMatchGroup eMatchGroup ) const;

	// Are we presently connected to a match server.  If bLiveMatch is true, only consider servers hosting our current
	// live match, as opposed to servers we joined once-upon-a-time for a match (but might now be on the match-result
	// screen)
	bool BConnectedToMatchServer( bool bLiveMatch );

	// !! Does NOT mean you're *in* this match. See Above.
	bool BHaveRunningMatch() const;
	bool BHaveLiveMatch() const;
	uint64_t GetLiveMatchID() const { return 0u; }
	PlayerGroupID_t GetLiveMatchLobbyID() const { return 0u; }

	// Whether the local player has a chat suspension in their assigned match.
	bool BHaveChatSuspensionInCurrentMatch() const { return m_bAssignedMatchChatSuspension; }

	// Abandon our currently assigned match.
	void AbandonCurrentMatch();

	// If we are assigned to a match, this is its matchgroup.
	ETFMatchGroup GetLiveMatchGroup() const;

	// The abandon status for our current match, whether or not we're connected to it
	EAbandonGameStatus GetAssignedMatchAbandonStatus();

	// Helper that combines GetMatchAbandonStatus and BConnectedToMatch as this is usually what you're asking.
	EAbandonGameStatus GetCurrentServerAbandonStatus()
	{
		return BConnectedToMatchServer( true ) ? GetAssignedMatchAbandonStatus() : k_EAbandonGameStatus_Safe;
	}

	static bool BIsBannedFromMatchmaking( EMMPenaltyPool ePool, CRTime* prtExpireTime = NULL, int* pnDuration = NULL );

	// Connect to our current match
	void JoinMMMatch();

	// TODO(Universal Parties): Audit/relocate remaining party functions.
	CTFParty* GetParty();

	CTFGSLobby* GetLobby() const;

	void ConnectToServer( const char *connect );

//	GCSDK::CGCClientSharedObjectCache	*GetSOCache() { return m_pSOCache; }

	// World status
	const CMsgTFWorldStatus &WorldStatus() const { return m_WorldStatus; }

	/// See if we've got a ticket
	static bool BLocalPlayerInventoryHasMvmTicket( void );
	static int GetLocalPlayerInventoryMvmTicketCount( void );

	/// See if we've got a double-down
	static bool BLocalPlayerInventoryHasSquadSurplusVoucher( void );
	static int GetLocalPlayerInventorySquadSurplusVoucherCount( void );

#ifdef USE_MVM_TOUR
	/// Get info about the local player's badge.  Returns false if we can't
	/// find his inventory or he doesn't own a badge
	static bool BGetLocalPlayerBadgeInfoForTour( int iTourIndex, uint32 *pnBadgeLevel, uint32 *pnCompletedChallenges );
#endif // USE_MVM_TOUR

	// TODO(Universal Parties): Audit which of these is still in use
	struct MatchMakerHealthData_t
	{
		float m_flRatio;
		Color m_colorBar;
		CUtlString m_strLocToken;
	};

	MatchMakerHealthData_t GetHealthBracketForRatio( float flRatio ) const;

	uint32 GetMostSearchedCount() const { return m_nMostSearchedMapCount; }
	MatchMakerHealthData_t GetOverallHealthDataForLocalCriteria() const;
	MatchMakerHealthData_t GetHealthDataForMap( uint32 nMapIndex ) const;
	void RequestMatchMakerStats() const;
	void SetMatchMakerStats( const CMsgGCMatchMakerStatsResponse newStats );
	const CMsgGCMatchMakerStatsResponse &GetMatchMakerStats() { return m_MatchMakerStats; }
	const CUtlDict< float > &GetDataCenterPopulationRatioDict( ETFMatchGroup eMatchGroup ) { return m_dictDataCenterPopulationRatio[ eMatchGroup ]; }

	void AcknowledgePendingRatingAndSources( ETFMatchGroup eMatchGroup );
	void AcknowledgeNotification( uint32 nAccountID, uint64 ulNotificationID );

	void SetSurveyRequest( const CMsgGCSurveyRequest& msgSurveyRequest );
	const CMsgGCSurveyRequest& GetSurveyRequest() const { return m_msgSurveyRequest; }
	void SendSurveyResponse( int32 nResponse );
	void ClearSurveyRequest();

	void LeaveGameAndPrepareToJoinParty( GCSDK::PlayerGroupID_t nPartyID );
	bool BIsPhoneVerified( void );
	bool BIsPhoneIdentifying( void );
	bool BHasCompetitiveAccess( void );

	bool BIsIPRecentMatchServer( netadr_t ip ) { return m_vecMatchServerHistory.Find( ip ) != m_vecMatchServerHistory.InvalidIndex(); }

	void AddLocalPlayerSOListener( ISharedObjectListener* pListener, bool bImmedately = true );
	void RemoveLocalPlayerSOListener( ISharedObjectListener* pListener );


	//
	// Reliable Messages
	//
	const CReliableMessageQueue &ReliableMsgQueue() const { return m_ReliableMsgQueue; }
	CReliableMessageQueue &ReliableMsgQueue() { return m_ReliableMsgQueue; }
	bool BPendingReliableMessages() const { return ReliableMsgQueue().NumPendingMessages() > 0; }
	bool BStalledReliableMessages() const { return ReliableMsgQueue().BStalled(); }

	// Helper that should be used for determining if our connection is lagged/resyncing/reconnecting/etc, over simply
	// BConnectedtoGC()
	bool BHealthyGCConnection() const { return BConnectedtoGC() && !BStalledReliableMessages(); }

	// Have we received positive signal from the GC that our client version is out of date?
	bool BClientOutOfDate() const { return m_bClientOutOfDate; }

	void ServerRequestEquipment();
	void LocalInventoryChanged();

protected:

	// CGCClientSystem
	virtual void PreInitGC() OVERRIDE;
	virtual void PostInitGC() OVERRIDE;


private:
	friend class CGCClientAcceptInviteResponse;
	friend class CGCWorldStatusBroadcast;
//	void CreateSourceTVProxy( uint32 source_tv_public_addr, uint32 source_tv_private_addr, uint32 source_tv_port );

	//
	// GC data
	//
	bool m_bRegisteredSharedObjects = false;
	bool m_bInittedGC               = false;
	GCSDK::CGCClientSharedObjectCache *m_pSOCache = nullptr;
	CUtlVector< ISharedObjectListener* > m_vecDelayedLocalPlayerSOListenersToAdd;

	//
	// Ping
	//
	CMsgGCDataCenterPing_Update m_msgCachedPingUpdate;


	//
	// World Status
	//
	void SetWorldStatus( CMsgTFWorldStatus &status );
	CMsgTFWorldStatus m_WorldStatus;

	//
	// Stats
	//
	uint32 m_nMostSearchedMapCount = 0;
	CMsgGCMatchMakerStatsResponse m_MatchMakerStats;
	CUtlDict< float > m_dictDataCenterPopulationRatio[ ETFMatchGroup_ARRAYSIZE ];

	/// Steam callbacks
#define DECL_STEAM_CALLBACK( callback )             \
	void OnSteam##callback ( callback##_t *pInfo ); \
	CCallback<ThisClass, callback##_t, false>       \
		m_callbackSteam##callback { this, &ThisClass::OnSteam##callback };

	// Creates:
	//  void OnSteamThing( Thing_t *pInfo );
	//  CCallback<ThisClass, Thing_t, false > m_callbackSteamThing{ this, &ThisClass::OnSteamThing };

	// DECL_STEAM_CALLBACK( LobbyChatMsg );
	// DECL_STEAM_CALLBACK( SomeOtherFunThing... );
	DECL_STEAM_CALLBACK( GetTicketForWebApiResponse );

#undef DECL_STEAM_CALLBACK

	//
	// SDK inventory
	//
	void WebapiInventoryThink();
	void OnWebapiInventoryReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure );
	void OnWebapiAuthTicketReceived( GetTicketForWebApiResponse_t* pInfo );

	enum EWebapiInventoryState {
		kWebapiInventoryState_Init,

		// Request inventory for the local client
		kWebapiInventoryState_RequestAuthToken,
		kWebapiInventoryState_WaitingForAuthToken,
		kWebapiInventoryState_AuthTokenReceived,
		kWebapiInventoryState_RequestInventory,
		kWebapiInventoryState_WaitingForInventory,
		kWebapiInventoryState_InventoryReceived,

		// Once we have the local client inventory, we will update it to match
		// our set of equipped items, and then we will build an auth ticket to
		// send to whatever server we are connected to.
		kWebapiInventoryState_BuildServerMessage,
		kWebapiInventoryState_RequestServerAuthToken,
		kWebapiInventoryState_WaitingForServerAuthToken,
		kWebapiInventoryState_ServerAuthTokenReceived,
		kWebapiInventoryState_SentToServer,
	};

	struct WebapiInventoryState_t
	{
		EWebapiInventoryState m_eState = kWebapiInventoryState_Init;

		// Authentication
		HAuthTicket m_hSteamAuthTicket = k_HAuthTicketInvalid;
		CUtlVector<uint8> m_bufAuthToken;

		// Inventory request
		HTTPRequestHandle m_hInventoryRequest = INVALID_HTTPREQUEST_HANDLE;
		CCallResult<CTFGCClientSystem, HTTPRequestCompleted_t> m_InventoryRequestCompleted;

		// Server inventory -- they get a subset of our items that we allow
		CMsgAuthorizeServerItemRetrieval m_msgItems;
		CUtlMemory<char> m_strMsgItems; // serialized and base64 encoded version of m_msgItems, so we can sign it
		HAuthTicket m_hServerAuthTicket = k_HAuthTicketInvalid;
		CUtlVector<uint8> m_bufServerAuthToken;
		CUtlString m_strServerIdentity; // hex-encoded SHA256 of m_bufMsgItems

		// Did we make any changes that we need to communicate to a server?
		bool m_bLocalChangesApplied = false;

		// Backoff
		RTime32 m_rtNextRequest = 0;
		int m_nBackoffSec = 0;
		void Backoff();
		void RequestSucceeded();	// resets backoff timers
		bool IsBackingOff();
	};
	WebapiInventoryState_t m_WebapiInventory;

	//
	// SDK Server inventory -- just get the auth ticket and send it to the server
	//
	enum EWebapiServerInventoryState {
		kWebapiServerInventoryState_Init,
		kWebapiServerInventoryState_RequestAuthToken,
		kWebapiServerInventoryState_WaitingForAuthToken,
		kWebapiServerInventoryState_AuthTokenReceived,
		kWebapiServerInventoryState_SendToServer
	};
	WebapiInventoryState_t m_WebapiServerInventory;
	void OnWebapiServerAuthTicketReceived( GetTicketForWebApiResponse_t* pInfo );

	// SDK expansion points
	void SDK_SelectItemsToSendToServer( CMsgAuthorizeServerItemRetrieval* /*out*/ pMsg, CGCClientSharedObjectCache* pSOCache );
	void SDK_AddServerInventoryInfo( KeyValues* /*out*/ pKV, CGCClientSharedObjectCache* pSOCache );

	//
	// Match logic
	//

	friend class ReliableMsgAcceptLobbyInvite;
	void OnAcceptLobbyReply( PlayerGroupID_t );

	bool IsConnectStateDisconnected();
	// Called to re-evaluate our have-a-lobby state, returns true if it updated anything.
	bool UpdateAssignedLobby();
	void FireGameEventLobbyUpdated();

	// Fired when our invite objects change
	void OnMatchInvitesUpdated();

	enum EConnectState
	{
		eConnectState_Disconnected,
		eConnectState_ConnectingToMatchmade,
		eConnectState_ConnectedToMatchmade,
		eConnectState_NonmatchmadeServer,
	};
	EConnectState          m_eConnectState                    = eConnectState_Disconnected;

	// Reliable messages in flight to accept matches, to prevent UI confusion
	PlayerGroupID_t m_nAcceptingMatchLobbyID = 0;


	// If we were given a chat suspension for the assigned match.
	bool m_bAssignedMatchChatSuspension = false;


	// Due to network race conditions, delay for a bit before we respond
	float m_flCheckForRejoinTime = 0.f;

	float m_flNextCasualStatsUpdateTime = 0.f;

	// If we've seen that we are out of date according to the GC status
	bool m_bClientOutOfDate = false;

	// History of assigned matches so things like the server browser can reason about our connect history.
	CUtlVector< netadr_t > m_vecMatchServerHistory;

	// Are we connected, and to whom
	CSteamID m_steamIDCurrentServer;

	//
	// Survey
	//
	CMsgGCSurveyRequest m_msgSurveyRequest;

	//
	// Reliable Messages
	//

	CReliableMessageQueue m_ReliableMsgQueue;
};

CTFGCClientSystem* GTFGCClientSystem();
CTFPartyClient* GTFPartyClient();

#endif // _INCLUDED_TF_GC_CLIENT_H
