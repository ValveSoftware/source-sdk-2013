//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_PARTYCLIENT_H
#define TF_PARTYCLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.pb.h"
#include "tf_matchcriteria.h"
#include "econ/local_steam_shared_object_listener.h"
#include "playergroup.h"

class CTFGCClientSystem;
class CMvMMissionSet;
class CTFGroupMatchCriteria;
class CTFPerPlayerMatchCriteria;
class CTFParty;
class CTFPartyInvite;

class CTFPartyClient : private CAutoGameSystemPerFrame, private CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_GAMEROOT( CTFPartyClient, CAutoGameSystem );
	friend class CTFGCClientSystem;
	// Reliable messages that deliver OnReply
	friend class ReliableMsgPartySetOptions;
	friend class ReliableMsgPartyQueueForMatch;
	friend class ReliableMsgPartyQueueForStandby;
	friend class ReliableMsgPartyRemoveFromQueue;
	friend class ReliableMsgPartyRemoveFromStandbyQueue;
	friend class ReliableMsgPartyClearPendingPlayer;
	friend class ReliableMsgPartyClearOtherPartyRequest;
	// Messages
	friend class CGCClientJobPartyChatMsg;
public:
	CTFPartyClient();
	~CTFPartyClient();

	// If we have a real (server-side) party object, vs a 'dummy' party-of-one
	bool BHaveActiveParty() const { return !!m_pActiveParty; }

	// How many members are in our party.  Will always be at least one.
	int GetNumPartyMembers() const;

	// Get the current status for a given party member
	struct MemberStatus_t { bool bOnline; bool bOutOfDate; };
	MemberStatus_t GetPartyMemberStatus( int idx ) const;

	// How many of our party members are currently online?
	int CountNumOnlinePartyMembers() const;

	// Returns a party member's SteamID
	CSteamID GetPartyMember( int i ) const;

	// How many pending members (invites, requests to join) we have
	int GetNumPendingMembers() const;

	// How many slots we have available. Helper
	int GetNumFreePartySlots() const
		{ return Max( GetMaxPartyMembers() - GetNumPartyMembers() - GetNumPendingMembers(), 0 ); }

	// How large can our party get?
	int GetMaxPartyMembers() const;

	// Get our active party object.  Solo party-of-one state may not have an active party object.
	const CTFParty* GetActiveParty() const { return m_pActiveParty; }

	// Helper to return the active party's group ID or 0 if we are a solo party-of-one
	GCSDK::PlayerGroupID_t GetActivePartyID() const { return m_unActivePartyID; }

	// If we are currently leading our party.  This includes if we have no active party and are simulating a
	// party-of-one.  False if we are in a party but not leading it.
	//
	// NOTE: Prefer BControllingPartyActions() if trying to reason about locally controlled group criteria
	// TODO(Universal Parties): Audit callers, some may want BControllingPartyActions()
	bool BIsPartyLeader() const { return !BHaveActiveParty() || m_bIsActivePartyLeader; }

	// Right now this is identical to leader, but there may be states where parties are locked in the future, or
	// non-leaders can control things, etc
	bool BControllingPartyActions() const { return BIsPartyLeader(); }

	// Leave our current party, returning to a solo party-of-one.  Idempotent.
	void LeaveActiveParty();

	// Set our local UI state.  This is mirrored to other party members if we are the leader, but should always be
	// called upon state changes -- we will internally decide if/when to send it up.
	void SetLocalUIState( ETFSyncedMMMenuStep eMenuStep, ETFMatchGroup eMatchGroup );

	// Get our own local UI state, regardless of what the leader might be doing
	const TFSyncedMMUIState &GetLocalUIState() const { return m_localUIState; }

	// The UI state of our party leader, or our own local state if we are the leader.
	const TFSyncedMMUIState &GetLeaderUIState() const;

	//
	// Preferences (auto-persisted via convar)
	//
	enum EPartyJoinRequestMode
	{
		// Saved in a convar, don't renumber
		k_ePartyJoinRequestMode_OpenToFriends = 0,
		k_ePartyJoinRequestMode_FriendsCanRequestToJoin = 1,
		k_ePartyJoinRequestMode_ClosedToFriends = 2,
	};
	// The current default value when convar has no explicit choice
	static const EPartyJoinRequestMode k_ePartyJoinRequestMode_Current_Default = k_ePartyJoinRequestMode_OpenToFriends;
	// Explicit values range
	static const EPartyJoinRequestMode k_ePartyJoinRequestMode_Min = k_ePartyJoinRequestMode_OpenToFriends;
	static const EPartyJoinRequestMode k_ePartyJoinRequestMode_Max = k_ePartyJoinRequestMode_ClosedToFriends;

	EPartyJoinRequestMode GetPartyJoinRequestMode() const;
	void SetPartyJoinRequestMode( EPartyJoinRequestMode );
	bool GetIgnorePartyInvites() const;
	void SetIgnorePartyInvites( bool bIgnore );

	// Re-read preferences from convars (should be handled automatically unless you're doing something bad)
	void ReadConVarPreferences();

	//
	// Invites
	//

	// Capability checks

	// Is this player a valid target for party invites right meow
	bool BCanInvitePlayer( CSteamID steamID );

	// Invite a player to your party.  Returns false if there is no more room or if the player is not a valid target
	// (offline, no valid relationship).  If bExpectingExistingRequestToJoin is set, only if there is a pending
	// invite (anti-race).
	bool BInvitePlayerToParty( CSteamID steamID, bool bExpectingExistingRequestToJoin );

	// For the party leader -- rejects an incoming join request or revokes an outgoing invite
	void CancelOutgoingInviteOrIncomingJoinRequest( CSteamID steamIDTarget );

	// For the invitee -- cancels an outgoing request to join or reject and incoming invite to join a party
	void CancelOutgoingJoinRequestOrIncomingInvite( CSteamID steamIDTarget );

	// Promot player to be leader
	bool BPromoteToLeader( CSteamID steamID );

	// Can we request to join this player (independent of current party state as it implies we drop party)
	bool BCanRequestToJoinPlayer( CSteamID steamID );

	// Kick a party member
	bool BKickPartyMember( CSteamID steamID );

	// Request to join the given player's party.  If bExpectingExistingInvite is set, only if there is a pending
	// invite (anti-race)
	bool BRequestJoinPlayer( CSteamID steamID, bool bExpectingExistingInvite );

	//
	// Chat
	//   See party_chat client event for listening to incoming chat

	// Send a chat message
	//
	// Note that we are echo'd on own chat, so UI should not display this chat to the client -- this ensures we see our
	// own messages well ordered with others, and are not falsely led into believing lost messages were not.
	void SendPartyChat( const char *pszMessage );

	//
	// Match logic
	//

	// If we are queued for MM in this match group
	bool BInQueueForMatchGroup( ETFMatchGroup eMatchGroup ) const;

	// Enumerate all the match groups we are queued for
	int GetNumQueuedMatchGroups() const;
	ETFMatchGroup GetQueuedMatchGroupByIdx( int idx ) const;

	// Helper to check if we are queued for any match group
	bool BInAnyMatchQueue() const { return GetNumQueuedMatchGroups() > 0; };

	// If we are queued as a standby player to join our party
	bool BInStandbyQueue() const;

	// Can we currently request to queue to join our party's lobby
	bool BCanQueueForStandby() const;

	// Does the current match group or any of our invites prevent us from queuing?
	// If bCheckParty is true, check if any party members are blocked from queuing, otherwise just ourselves
	bool BCurrentMatchOrInviteDisallowsQueuing( bool bCheckParty = true ) const;

	enum EDisabledReasonType
	{
		// In ascending severity
		k_eDisabledType_None = 0,	// Good to go
		k_eDisabledType_Criteria,	// Criteria doesn't allow queuing
		k_eDisabledType_System,		// Mode is disabled / Already queued
		k_eDisabledType_Locked,		// Need comp access
		k_eDisabledType_Banned,		// Banned in this mode
		k_eDisabledType_Network,	// No GC connection

	};

	// Can we currently queue up for a match.
	struct QueueEligibilityData_t
	{
		QueueEligibilityData_t()
		{
			memset( wszCantReason, 0, sizeof( wszCantReason ) );
		}
		wchar_t wszCantReason[ 1024 ];
		EDisabledReasonType m_eReason;
	};
	bool BCanQueueForMatch( ETFMatchGroup eGroup, CUtlVector< QueueEligibilityData_t >& vecReasons ) const;

	// Ask that we be put in queue for the given match group.  Requires BControllingPartyActions() be true
	void RequestQueueForMatch( ETFMatchGroup eMatchGroup );

	// Ask that we queue to join our party's lobby
	void RequestQueueForStandby();

	// Ask that we be removed from match queue for this match group.  Requires BControllingPartyActions() be true
	void CancelMatchQueueRequest( ETFMatchGroup eMatchGroup );

	// Ask that we be removed from standby queue.
	void CancelStandbyQueueRequest();

	//
	// Criteria
	//

	// The current group criteria in effect, may be from e.g. a party leader or forced GC settings, vs. our explicit
	// preferences locally
	const CTFGroupMatchCriteria &GetEffectiveGroupCriteria() const;

	// The last criteria acknowledged by the GC.  You rarely need to know this, as we predict things locally, but could
	// be useful for things like spinners when changing match settings (if it's in effective but not confirmed)
	ConstRefTFGroupMatchCriteria GetLastConfirmedGroupCriteria() const;

	// Our local criteria. May not be what is used if we enter queue right now, see GetEffectiveGroupCriteria.  For
	// instance, if we are in a party, this is our criteria, not the leader's.
	const CTFGroupMatchCriteria &GetLocalGroupCriteria() const { return m_localGroupCriteria; }
	const CTFPerPlayerMatchCriteria &GetLocalPlayerCriteria() const { return m_localPerPlayerCriteria; }

	// Mutable getter for local player criteria.
	CTFPerPlayerMatchCriteria &MutLocalPlayerCriteria();

	// Mutable getter for local group criteria.
	CTFGroupMatchCriteria &MutLocalGroupCriteria();

	// Helpers to load and save our current criteria from/to disk
	void SaveCasualCriteria();
	void LoadSavedCasualCriteria();

	//
	// Invites / Requests for *our* party
	//

	// Players invited to our party
	int GetNumOutgoingInvites() const;
	CSteamID GetOutgoingInvite( int i ) const;
	bool BHaveOutgoingInviteForSteamID( CSteamID steamID ) const;

	// Requests to join our party
	int GetNumIncomingJoinRequests() const;
	CSteamID GetIncomingJoinRequest( int i ) const;
	bool BHaveIncomingJoinRequestForSteamID( CSteamID steamID ) const;

	//
	// Invites / Requests to join *other* parties (not the currently active party)
	//

	// Number of invites we have to join *other* parties (not invites in our active party)
	int GetNumIncomingInvites() const;
	CTFPartyInvite *GetIncomingInvite( int i ) const;
	bool BHaveIncomingInviteForSteamID( CSteamID steamID ) const;

	// Number requests to join we have to join *other* parties (not people requesting to join our party)
	int GetNumOutgoingJoinRequests() const;
	CTFPartyInvite *GetOutgoingJoinRequest( int i ) const;
	bool BHaveOutgoingJoinRequestForSteamID( CSteamID steamID ) const;

	//
	// Misc/Debug
	//
	void ForcePartyUpdate() { UpdateActiveParty(); }

private:
	// IGameSystemPerFrame
	virtual bool Init() OVERRIDE;
	virtual void Update( float frametime ) OVERRIDE { Think(); }

	// ISharedObjectListener
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	// Not currently used
	// virtual void PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	// virtual void PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	//
	/// Reliable messages
	//
	void OnSetOptionsReply( const GCSDK::CProtoBufMsg< CMsgPartySetOptions > &msg,
	                        const GCSDK::CProtoBufMsg< CMsgPartySetOptionsResponse > &reply );
	void OnQueueForMatchReply( const GCSDK::CProtoBufMsg< CMsgPartyQueueForMatch > &msg,
	                           const GCSDK::CProtoBufMsg< CMsgPartyQueueForMatchResponse > &reply );
	void OnQueueForStandbyReply( const GCSDK::CProtoBufMsg< CMsgPartyQueueForStandby > &msg,
	                             const GCSDK::CProtoBufMsg< CMsgPartyQueueForStandbyResponse > &reply );
	void OnRemoveFromQueueReply( const GCSDK::CProtoBufMsg< CMsgPartyRemoveFromQueue > &msg,
	                             const GCSDK::CProtoBufMsg< CMsgPartyRemoveFromQueueResponse > &reply );
	void OnRemoveFromStandbyQueueReply( const GCSDK::CProtoBufMsg< CMsgPartyRemoveFromStandbyQueue > &msg,
	                                    const GCSDK::CProtoBufMsg< CMsgPartyRemoveFromStandbyQueueResponse > &reply );
	void OnClearPendingPlayerReply( const GCSDK::CProtoBufMsg< CMsgPartyClearPendingPlayer > &msg,
	                                const GCSDK::CProtoBufMsg< CMsgPartyClearPendingPlayerResponse > &reply );
	void OnClearOtherPartyRequestReply( const GCSDK::CProtoBufMsg< CMsgPartyClearOtherPartyRequest > &msg,
	                                    const GCSDK::CProtoBufMsg< CMsgPartyClearOtherPartyRequestResponse > &reply );

	//
	/// Inbound messages
	//
	void ReceivedPartyChatMsg( CMsgPartyChatMsg &msg );

	//
	/// Chat
	//
	void PostChatGameEvent( const CSteamID &steamIDPoster, ETFPartyChatType eType, const char *pszText );

	//
	/// Internal
	//

	// Stored info about members
	struct Member_t { CSteamID steamID; bool bOnline; };

	// Periodic think
	void Think();

	// Manage keeping our settings/etc in sync with the GC
	void CheckSendUpdates();
	enum EDirtyFlags {
		eDirty_None           = 0,
		eDirty_UIState        = 1 << 0,
		eDirty_GroupCriteria  = 1 << 1,
		eDirty_PlayerCriteria = 1 << 2
	};
	bool BMakeUpdateMsg( CTFPartyOptions &msgOptions );
	uint32_t CalcDirtyFlags();

	// Create and send a GC invite message this player.  Most users will rather want BInvitePlayerToParty.
	bool BSendPartyInvitePlayerMessageInternal( CSteamID steamID, bool bExpectingExistingRequestToJoin );

	// Check if our relationship with the given steamID would allow us to invite/request-to-join their party
	// (e.g. friends or in-same-server-right-now, mirroring what the GC allows)
	bool BAllowedToPartyWith( CSteamID steamID ) const;

	// Update m_pActiveParty, m_bIsActivePartyLeader, etc, to the currently active party object, calls
	// OnPartyFoo* necessary.
	// Returns true if our active party changed (not leader or other parameters)
	bool UpdateActiveParty();

	void UpdateIncomingInvitesOrOutgoingJoinRequests();

	// Flush all knowledge of the last options we sent up.  The next CheckSendUpdates will believe we need to send an
	// initial update (or not, depending on the situation)
	void CheckResetSentOptions();

	//
	// Internal events fired by UpdateActiveParty
	//   In the case of multiple events being necessary, they fire in the order declared below

	// Our backing party changed
	void OnNewParty();
	// Handle online/offline state changing
	void OnPartyMemberOnlineStateChanged( const Member_t &member );
	// Handle a new member
	void OnPartyMemberGained( CSteamID steamID );
	// Handle a lost member
	void OnPartyMemberLost( CSteamID steamID );
	// When we transition between leading a party and not (no-party counts as not).
	void OnIsLeaderChanged( bool bIsLeader );
	// Called when effective group criteria changes
	void OnEffectiveCriteriaChanged();
	// Called when queued state changes for any match group
	void OnInQueueChanged( ETFMatchGroup eMatchGroup );
	// Called when standby-queued state (BInStandbyQueue) changes
	void OnInStandbyQueueChanged();
	// Handle invites-to/outgoing-join-requests-to  other parties
	void OnIncomingInvitesChanged();
	void OnOutgoingJoinRequestsChanged();
	// Handle outgoing invites to our party or incoming join requests to our party
	void OnOutgoingInvitesChanged();
	void OnIncomingJoinRequestsChanged();
	// Most generic
	void OnPartyUpdated();

	// One of the persisted party preferences updated
	void OnPartyPrefChanged();

	// Are we using our active party for our effective criteria (vs local)
	bool BEffectiveGroupCriteriaFromParty() const;

	// Set the in-queue flag for a match group -- does not actually queue, used for state/event tracking.
	void InternalSetInQueueForMatchGroup( ETFMatchGroup eMatchGroup, bool bSet );

	// Pending queue messages
	bool BHaveAnyPendingQueueMsg() const;
	bool BHavePendingQueueMsg( ETFMatchGroup eMatchGroup ) const;
	void SetPendingQueueMsg( ETFMatchGroup eMatchGroup, bool set );
	bool BHavePendingQueueCancelMsg( ETFMatchGroup eMatchGroup ) const;
	void SetPendingQueueCancelMsg( ETFMatchGroup eMatchGroup, bool set );

	bool m_bInit = false;

	// Local SO Cache, if subscribed
	GCSDK::CGCClientSharedObjectCache *m_pSOCache = nullptr;
	// The currently active party object, if we have one
	CTFParty *m_pActiveParty = nullptr;
	GCSDK::PlayerGroupID_t m_unActivePartyID = 0u;
	// If we are the leader of our active party, false if we have no party
	bool m_bIsActivePartyLeader = false;
	// If we are currently in queue.  Cached so UpdateActiveParty can fire events on edges, since we may leave queue as
	// part of shared object updates wherein the previous state is lost.
	CUtlVector< ETFMatchGroup > m_vecInQueue;
	bool m_bInStandbyQueue = false;
	// Cached active party members fer diffin'
	CUtlVectorFixed< Member_t, MAX_PARTY_SIZE > m_activePartyMembers;
	// So we can index by type easily
	// These pointers die when our SOCache changes, we need to remember who they were for diffing
	struct PartyInvite_t { CTFPartyInvite *pInvite; CSteamID steamIDInviter; };
	CUtlVector< PartyInvite_t > m_vecIncomingInvites;
	CUtlVector< PartyInvite_t > m_vecOutgoingJoinRequests;
	CUtlVector< CSteamID > m_vecIncomingJoinRequests;
	CUtlVector< CSteamID > m_vecOutgoingInvites;

	// When we first accured pending criteria changes that have not been sent.  -1 indicates all changes have been sent.
	float m_flPendingChangesTime = -1.f;
	// The last time we updated our criteria with the GC
	float m_flLastCriteriaUpdate = -1.f;

	// If we have ever sent up criteria, or if pending criteria is a fresh set that should wipe whatever the server has
	// (new session, leader changed, etc)
	bool m_bSentInitialCriteria = false;

	// Cached criteria object from party updates
	CTFGroupMatchCriteria m_activePartyCriteria;

	// Local (this player/client) criteria, not necessary the ones in effect, see GetEffectiveCriteria calls
	CTFGroupMatchCriteria     m_localGroupCriteria;
	CTFPerPlayerMatchCriteria m_localPerPlayerCriteria;
	TFSyncedMMUIState         m_localUIState;

	// Criteria last sent to GC, for reasoning about changes
	CTFGroupMatchCriteria     m_lastSentGroupCriteria;
	CTFPerPlayerMatchCriteria m_lastSentPerPlayerCriteria;
	// TODO(Universal Parties): Somewhere the party is getting echo'd back down at us and we should assert that we match
	//                          these values
	TFSyncedMMUIState         m_lastSentUIState;

	// Tracks if anyone has mutated the criteria, to perform sync logic
	bool m_bGroupCriteriaChanged                             = false;
	bool m_bPerPlayerCriteriaChanged                         = false;
	bool m_bPendingStandbyQueueMsg                           = false;
	bool m_bPendingStandbyQueueCancelMsg                     = false;
	bool m_bPendingReliableCriteriaMsg                       = false;
	bool m_arbPendingQueueCancelMsg[ETFMatchGroup_ARRAYSIZE] = { false };
	bool m_arbPendingQueueMsg[ETFMatchGroup_ARRAYSIZE]       = { false };
	// These guys track both that we have a pending message and for what partyID, so we can reason about if pending
	// messages are now moot.
	GCSDK::PlayerGroupID_t m_unPendingReliableCriteriaMsgParty = 0u;

	// If set, fire an effective-criteria-changed event next frame
	bool m_bQueuedEffectiveCriteriaChangeEvent = false;

	// Preferences
	EPartyJoinRequestMode m_ePrefPartyJoinRequestMode = k_ePartyJoinRequestMode_Current_Default;
	bool                  m_bPrefIgnoreInvites        = false;
};

#endif // TF_PARTYCLIENT_H
