//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_partyclient.h"
#include "confirm_dialog.h"
#include "tf_gc_shared.h"
#include "tf_gc_client.h"
#include "tf_gcmessages.pb.h"
#include "tf_party.h"
#include "tf_matchmaking_shared.h"
#include "tf_match_description.h"
#include "tf_ladder_data.h"
#include "protoutils.h"
#include "clientmode_tf.h"
#include "tier3/tier3.h"
#include "vgui_controls/URLLabel.h"

static const char* s_pszCasualCriteriaSaveFileName = "casual_criteria.vdf";

// Delay on criteria change to wait for more before sending a message
ConVar tf_mm_party_send_criteria_delay( "tf_mm_party_send_criteria_delay", "1.0", FCVAR_DEVELOPMENTONLY,
                                        "", true, 0.f, false, 0.f );
// Minimum time to wait between criteria messages, even if it want to send them urgently, to prevent e.g. checkbox
// spamming while queued
ConVar tf_mm_party_send_criteria_delay_minimum( "tf_mm_party_send_criteria_delay_minimum", "0.5", FCVAR_DEVELOPMENTONLY,
                                                "", true, 0.f, false, 0.f );

#ifdef _DEBUG
	#define DEFAULT_PARTY_DEBUG "1"
#else
	#define DEFAULT_PARTY_DEBUG "0"
#endif

ConVar tf_mm_partyclient_debug( "tf_mm_partyclient_debug", DEFAULT_PARTY_DEBUG );

static bool BPartyDbg() { return tf_mm_partyclient_debug.GetBool(); }

#define PartyDbg( ... )                                 \
	do {                                                \
		if ( BPartyDbg() )                              \
			{ Msg( "[PartyClientDbg] " __VA_ARGS__ ); } \
	} while(false);

#define PartyMsg( ... ) \
	do { Msg( "[PartyClient] " __VA_ARGS__ ); } while(false);

#define PartyWarn( ... ) \
	do { Warning( "[PartyClient] " __VA_ARGS__ ); } while(false);

static const CTFParty *ToParty( const GCSDK::CSharedObject *pObject )
{
	return assert_cast< const CTFParty *>( pObject );
}

static bool AssertValidMatchGroup( ETFMatchGroup eMatchGroup )
{
	if ( !ETFMatchGroup_IsValid( eMatchGroup ) || eMatchGroup == k_eTFMatchGroup_Invalid )
	{
		AssertMsg( false, "Bogus match group" );
		return false;
	}
	return true;
}

static bool SyncedUIStateEqual( TFSyncedMMUIState &a, TFSyncedMMUIState &b )
{
	AssertMsgOnce( ValveProtoUtils::MessageHasExactFields( a, { 1, 2 } ), "Audit this if you change the message thx" );
	return ( a.menu_step() == b.menu_step() ) && ( a.match_group() == b.match_group() );
}

extern CTFPartyClient *g_pTFPartyClient;

//-----------------------------------------------------------------------------
// Persistent settings
//-----------------------------------------------------------------------------
static void OnPartyClientPrefConVarChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	auto *pPartyClient = GTFPartyClient();
	if ( pPartyClient )
	{
		// Otherwise the client system will do this when it starts
		pPartyClient->ReadConVarPreferences();
	}
}

ConVar tf_mm_custom_ping_enabled( "tf_mm_custom_ping_enabled", "0", FCVAR_ARCHIVE,
                                  "Whether to use a custom ping tolerance when matchmaking",
                                  false, 0.f, false, 0.f, OnPartyClientPrefConVarChanged );
ConVar tf_mm_custom_ping( "tf_mm_custom_ping", "100", FCVAR_ARCHIVE,
                          "Custom ping tolerance in the matchmaking system.  See also tf_custom_ping_enabled.",
                          true, (float)CUSTOM_PING_TOLERANCE_MIN, true, (float)CUSTOM_PING_TOLERANCE_MAX,
                          OnPartyClientPrefConVarChanged );

ConVar tf_party_join_request_mode( "tf_party_join_request_mode", "-1", FCVAR_ARCHIVE,
                                   "The current mode for incoming party join requests:\n"
                                   "  -1 - Use default (currently 0), \n"
                                   "   0 - Open to friends, \n"
                                   "   1 - Friends can request to join, \n"
                                   "   2 - Invite only\n",
                                   true, -1.f, // Default placeholder
                                   true, (float)CTFPartyClient::k_ePartyJoinRequestMode_Max,
                                   OnPartyClientPrefConVarChanged );
ConVar tf_party_ignore_invites( "tf_party_ignore_invites", "0", FCVAR_ARCHIVE, "If set, ignore incoming party invites",
                                OnPartyClientPrefConVarChanged );

// TODO(Universal Parties): Hook up and enable in UI
ConVar tf_party_keep_on_same_team( "tf_party_keep_on_same_team", "0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Popup dialog for MM errors with an optional clickable url
//-----------------------------------------------------------------------------
class CTFMMIssueDialog : public CTFMessageBoxDialog
{
	DECLARE_CLASS_SIMPLE( CTFMMIssueDialog, CTFMessageBoxDialog );
public:
	CTFMMIssueDialog()
		: CTFMessageBoxDialog( "", "", NULL, NULL, NULL )
	{
	}

	virtual ~CTFMMIssueDialog() {}

	virtual const char *GetResFile() OVERRIDE
	{
		return "Resource/UI/MMIssueDialog.res";
	}

	void Show( const char *pszText, const char *pszURL = nullptr )
	{
		wchar_t wszExpandedURL[256] = { 0 };
		g_pVGuiLocalize->ConstructString_safe( m_wszText, pszText, 0 );
		if ( pszURL )
			{ g_pVGuiLocalize->ConstructString_safe( wszExpandedURL, pszURL, 0 ); }
		SetDialogVariable( "url", wszExpandedURL );

		BaseClass::Show();

		vgui::URLLabel *urlLabel = dynamic_cast<vgui::URLLabel *>( FindChildByName( "URLLabel" ) );

		if ( urlLabel )
		{
			if ( wszExpandedURL[0] )
			{
				char szExpandedURL[256] = { 0 };
				g_pVGuiLocalize->ConvertUnicodeToANSI( wszExpandedURL, szExpandedURL, sizeof( szExpandedURL ) );
				urlLabel->SetURL( szExpandedURL );
				urlLabel->SetVisible( true );
			}
			else
			{
				urlLabel->SetVisible( false );
			}
		}
	}

	const wchar_t *GetText() OVERRIDE
	{
		return m_wszText;
	}

private:
	wchar_t m_wszText[256] = {0};
};

CTFMMIssueDialog *TFMMIssueDialog()
{
	static vgui::DHANDLE<CTFMMIssueDialog> s_hTFMMIssueDialog;
	if ( !s_hTFMMIssueDialog )
	{
		s_hTFMMIssueDialog = vgui::SETUP_PANEL( new CTFMMIssueDialog() );
	}

	return s_hTFMMIssueDialog;
}

//-----------------------------------------------------------------------------
// Reliable messages
//-----------------------------------------------------------------------------

// Redundant header for messages following the CMsgPartyFoo/EMsgGCParty_Foo/GTFPartyClient()->OnFooReply pattern
#define RELIABLE_PARTY_MSG( name, ... )                                                              \
	class ReliableMsgParty##name : public CJobReliableMessageBase < ReliableMsgParty##name,          \
	                                                                CMsgParty##name,                 \
	                                                                k_EMsgGCParty_##name,            \
	                                                                CMsgParty##name##Response,       \
	                                                                k_EMsgGCParty_##name##Response > \
	{                                                                                                \
	public:                                                                                          \
		const char *MsgName() { return "Party" #name ; }                                             \
		void OnReply( Reply_t &msgReply ) { GTFPartyClient()->On##name##Reply( Msg(), msgReply ); }  \
		void InitDebugString( CUtlString &dbgStr )                                                   \
		{                                                                                            \
			auto &msg = Msg().Body();                                                                \
			dbgStr.Format( __VA_ARGS__ );                                                            \
		}                                                                                            \
	};

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( SetOptions, "Party ID %llu, Overwrite %d", (unsigned long long) msg.party_id(), msg.options().overwrite_existing() )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( QueueForMatch, "Party ID %llu, Embedded Options %d", (unsigned long long) msg.party_id(), msg.has_final_options()  )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( QueueForStandby, "Party ID %llu, Lobby ID %llu", (unsigned long long) msg.party_id(), (unsigned long long) msg.party_lobby_id()  )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( RemoveFromQueue, "Party ID %llu", (unsigned long long) msg.party_id()  )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( RemoveFromStandbyQueue, "Party ID %llu", (unsigned long long) msg.party_id()  )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( ClearPendingPlayer, "Party ID %llu, Pending Player %s",
                    (unsigned long long) msg.party_id(), CSteamID( msg.pending_player_id() ).Render() )

//-----------------------------------------------------------------------------
RELIABLE_PARTY_MSG( ClearOtherPartyRequest, "Other Party ID %llu", (unsigned long long) msg.other_party_id() )

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTFPartyClient::CTFPartyClient()
{
	g_pTFPartyClient = this;
}

//-----------------------------------------------------------------------------
// Constructor/Destructor
//-----------------------------------------------------------------------------
CTFPartyClient::~CTFPartyClient()
{}

//-----------------------------------------------------------------------------
bool CTFPartyClient::Init()
{
	// May have raced with convar init
	m_bInit = true;
	this->ReadConVarPreferences();
	return true;
}

//-----------------------------------------------------------------------------
CTFPerPlayerMatchCriteria &CTFPartyClient::MutLocalPlayerCriteria()
{
	m_bPerPlayerCriteriaChanged = true;
	m_bQueuedEffectiveCriteriaChangeEvent = true;
	return m_localPerPlayerCriteria;
}

//-----------------------------------------------------------------------------
CTFGroupMatchCriteria &CTFPartyClient::MutLocalGroupCriteria()
{
	AssertMsg( BControllingPartyActions(),
	           "Getting mutable group criteria when we're not in control of our group criteria" );

	m_bGroupCriteriaChanged = true;
	// If this is also our effective criteria, queue an event for next frame (after it presumably changes)
	if ( !BEffectiveGroupCriteriaFromParty() )
		{ m_bQueuedEffectiveCriteriaChangeEvent = true; }
	return m_localGroupCriteria;
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumPartyMembers() const
{
	// We are always a party of one if we have no active party from the perspective of outside code.
	return BHaveActiveParty() ? GetActiveParty()->GetNumMembers() : 1;
}

//-----------------------------------------------------------------------------
CTFPartyClient::MemberStatus_t CTFPartyClient::GetPartyMemberStatus( int idx ) const
{
	auto pParty = GetActiveParty();
	if ( !pParty )
	{
		// Just us
		bool bOutOfDate = GTFGCClientSystem()->BClientOutOfDate();
		return MemberStatus_t{ /* bOnline */ true, bOutOfDate };
	}

	auto &worldStatus = GTFGCClientSystem()->WorldStatus();
	auto &activity = pParty->GetMemberActivity( idx );
	// Only if we've seen the client's version and gotten world status can we say they are out of date (may not during
	// login).  World status of 0 means no-enforcement.
	bool bOutOfDate = ( worldStatus.active_client_version() != 0 && activity.has_client_version() &&
	                    activity.client_version() != worldStatus.active_client_version() );
	return MemberStatus_t{ activity.online(), bOutOfDate };
}

//-----------------------------------------------------------------------------
int CTFPartyClient::CountNumOnlinePartyMembers() const
{
	// We are always a party of one if we have no active party from the perspective of outside code.
	auto pParty = GetActiveParty();
	if ( !pParty )
		{ return 1; }

	// Always treat ourselves as online so the count makes a little more sense when the GC is lagged/offline
	int ret = 0;
	CSteamID steamIDCurrent = m_pSOCache ? m_pSOCache->GetOwner() : CSteamID();
	for ( int i=0; i < pParty->GetNumMembers(); ++i )
	{
		if ( ( m_pSOCache && pParty->GetMember( i ) == steamIDCurrent ) || pParty->GetMemberActivity( i ).online() )
			{ ++ret; }
	}

	return Max( ret, 1 );
}

//-----------------------------------------------------------------------------
CSteamID CTFPartyClient::GetPartyMember( int i ) const
{
	Assert( BHaveActiveParty() );
	auto pParty = GetActiveParty();
	if ( !pParty )
		return CSteamID(); 

	int nCount = pParty->GetNumMembers();
	Assert ( i >= 0 && i < nCount );
	if ( i < 0 || i >= nCount )
		return CSteamID();

	return pParty->GetMember( i );
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumPendingMembers() const
{
	return BHaveActiveParty() ? GetActiveParty()->GetNumPendingPlayers() : 0;
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetMaxPartyMembers() const
{
	// There's also a GC convar mostly used for testing, but if we allowed different parties based on context/etc we
	// could do that logic here.
	return MAX_PARTY_SIZE;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BEffectiveGroupCriteriaFromParty() const
{
	// Non-party shows local state, leaders should see their local (predicted) state rather than latency (we ensure
	// these changes flush in our think)
	if ( BControllingPartyActions() )
		{ return false; }

	AssertMsg( BHaveActiveParty(), "Should have a party if we're not in control of party actions" );
	return BHaveActiveParty();
}

//-----------------------------------------------------------------------------
const CTFGroupMatchCriteria &CTFPartyClient::GetEffectiveGroupCriteria() const
{
	return BEffectiveGroupCriteriaFromParty() ? m_activePartyCriteria : m_localGroupCriteria;
}

//-----------------------------------------------------------------------------
ConstRefTFGroupMatchCriteria CTFPartyClient::GetLastConfirmedGroupCriteria() const
{
	auto *pParty = GetActiveParty();
	if ( !pParty )
	{
		return ConstRefTFGroupMatchCriteria( m_localGroupCriteria );
	}
	return ConstRefTFGroupMatchCriteria( pParty->Obj().group_criteria() );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SetLocalUIState( ETFSyncedMMMenuStep eMenuStep, ETFMatchGroup eMatchGroup )
{
	m_localUIState.set_menu_step( eMenuStep );
	m_localUIState.set_match_group( eMatchGroup );
}

//-----------------------------------------------------------------------------
const TFSyncedMMUIState& CTFPartyClient::GetLeaderUIState() const
{
	// Non-party shows local state, leaders should see their local (predicted) state rather than latency (we ensure
	// these changes flush in our think)
	auto *pParty = GetActiveParty();
	if ( !pParty || BIsPartyLeader() )
		{ return m_localUIState; }

	return pParty->Obj().leader_ui_state();
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BInvitePlayerToParty( CSteamID steamID, bool bExpectingExistingRequestToJoin )
{
	if ( !BCanInvitePlayer( steamID ) )
	{
		PartyWarn( "Attempting to invite user %s to party who is not in an invitable state\n",
		           steamID.Render() );
		return false;
	}

	PartyMsg( "Sending request to invite %s to party %llu\n", steamID.Render(), m_unActivePartyID );
	bool bSent = BSendPartyInvitePlayerMessageInternal( steamID, bExpectingExistingRequestToJoin );
	if ( !bSent )
		{ return false; }

	// TODO(Universal Parties): Do this in a response or upon seeing the invite -- and pass party ID (and add party ID command)
	if ( steamapicontext->SteamFriends() && !bExpectingExistingRequestToJoin )
	{
		steamapicontext->SteamFriends()->InviteUserToGame( steamID, CFmtStr( "+tf_party_request_join_user %llu 1",
		                                                                     steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() ) );
	}
	return true;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BSendPartyInvitePlayerMessageInternal( CSteamID steamID, bool bExpectingExistingRequestToJoin )
{
	CProtoBufMsg<CMsgPartyInvitePlayer> msg( k_EMsgGCParty_InvitePlayer );
	msg.Body().set_player_id( steamID.ConvertToUint64() );

	if ( m_unActivePartyID )
		{ msg.Body().set_party_id( m_unActivePartyID ); }

	if ( bExpectingExistingRequestToJoin )
		{ msg.Body().set_expecting_request_to_join( true ); }

	bool bSent = GCClientSystem()->BSendMessage( msg );

	if ( !bSent )
	{
		PartyWarn("Failed to send party invite message for %s\n", steamID.Render() );
	}
	return bSent;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BRequestJoinPlayer( CSteamID steamID, bool bExpectingExistingInvite )
{
	if ( !BCanRequestToJoinPlayer( steamID ) )
	{
		PartyWarn( "Cannot request to join user %s to party who is not in an invitable state\n",
		           steamID.Render() );
		return false;
	}

	PartyMsg( "Sending request to join %s party (current party %llu)\n", steamID.Render(), m_unActivePartyID );
	CProtoBufMsg<CMsgPartyRequestJoinPlayer> msg( k_EMsgGCParty_RequestJoinPlayer );
	if ( m_unActivePartyID )
		{ msg.Body().set_current_party_id( m_unActivePartyID ); }
	msg.Body().set_join_player_id( steamID.ConvertToUint64() );
	if ( bExpectingExistingInvite )
		{ msg.Body().set_expecting_invite( true ); }
	GCClientSystem()->BSendMessage( msg );
	return true;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BCanInvitePlayer( CSteamID steamID )
{
	// Note that auto-invites/rejects check AllowedToPartyWith directly as we never actually have a visible incoming
	// join request
	if ( !BControllingPartyActions() || !BAllowedToPartyWith( steamID ) )
		{ return false; }

	// Must have a free slot unless they are a pending join request already
	return ( GetNumFreePartySlots() > 0 ) || BHaveIncomingJoinRequestForSteamID( steamID );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CancelOutgoingInviteOrIncomingJoinRequest( CSteamID steamIDTarget )
{
	// TODO(Universal Parties): Add local state prediction, (wrap TFParty getters)
	if ( !BControllingPartyActions() )
		{ return; }

	PartyMsg( "Sending request to cancel outgoing invite/incoming join request for %s (current party %llu)\n",
	          steamIDTarget.Render(), m_unActivePartyID );
	CProtoBufMsg<CMsgPartyClearPendingPlayer> msg( k_EMsgGCParty_ClearPendingPlayer );
	if ( m_unActivePartyID )
		{ msg.Body().set_party_id( m_unActivePartyID ); }
	msg.Body().set_pending_player_id( steamIDTarget.ConvertToUint64() );
	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CancelOutgoingJoinRequestOrIncomingInvite( CSteamID steamIDTarget )
{
	// TODO(Universal Parties): Add local state prediction in getters
	PartyMsg( "Sending request to cancel outgoing invite/incoming join request for %s (current party %llu)\n",
	          steamIDTarget.Render(), m_unActivePartyID );
	CProtoBufMsg<CMsgPartyClearOtherPartyRequest> msg( k_EMsgGCParty_ClearOtherPartyRequest );

	// Note that our local lists are the lists the user should see and include prediction -- we want to check the raw
	// object for an invite that can be rejected.
	GCSDK::PlayerGroupID_t nOtherParty = 0u;
	CSharedObjectTypeCache *pTypeCache = ( m_pSOCache ? m_pSOCache->FindBaseTypeCache( CTFPartyInvite::k_nTypeID ) \
	                                                  : nullptr );
	if ( pTypeCache )
	{
		int nCount = pTypeCache->GetCount();
		for ( int i = 0; i < nCount; i++ )
		{
			CTFPartyInvite *pInvite = static_cast< CTFPartyInvite * >( pTypeCache->GetObject( i ) );
			if ( pInvite->GetInviter() == steamIDTarget )
				{ nOtherParty = pInvite->GetGroupID(); }
		}
	}

	if ( nOtherParty == 0u )
	{
		PartyWarn( "CTFPartyClient::CancelOutgoingJoinRequestOrIncomingInvite for %s -- "
		           "could not find an invite related to that user\n",
		           steamIDTarget.Render() );
		return;
	}

	PartyDbg( "Sending CancelOutgoingJoinRequestOrIncomingInvite(%llu)\n", nOtherParty );
	msg.Body().set_other_party_id( nOtherParty );
	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BPromoteToLeader( CSteamID steamID )
{
	if ( !BControllingPartyActions() || !BHaveActiveParty() )
		{ return false; }

	auto *pParty = GetActiveParty();
	if ( pParty->GetMemberIndexBySteamID( steamID ) == -1 )
		{ return false; }

	// Not really worth predicting
	CProtoBufMsg<CMsgPartyPromoteToLeader> msg( k_EMsgGCParty_PromoteToLeader );
	msg.Body().set_party_id( m_unActivePartyID );
	msg.Body().set_new_leader_id( steamID.ConvertToUint64() );

	PartyDbg( "Sending PartyPromoteToLeader(%s)\n", steamID.Render() );
	GCClientSystem()->BSendMessage( msg );

	return true;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BKickPartyMember( CSteamID steamID )
{
	if ( !BControllingPartyActions() || !BHaveActiveParty() )
		{ return false; }

	auto *pParty = GetActiveParty();
	if ( pParty->GetMemberIndexBySteamID( steamID ) == -1 )
		{ return false; }

	// TODO(Universal Parties): Predict
	CProtoBufMsg<CMsgPartyKickMember> msg( k_EMsgGCParty_KickMember );
	msg.Body().set_party_id( m_unActivePartyID );
	msg.Body().set_target_id( steamID.ConvertToUint64() );

	PartyDbg( "Sending PartyKickMember(%s)\n", steamID.Render() );
	GCClientSystem()->BSendMessage( msg );

	return true;
}


//-----------------------------------------------------------------------------
bool CTFPartyClient::BCanRequestToJoinPlayer( CSteamID steamID )
{
	// UpdateActiveParty should only allow invites we're allowed to act upon.  If we have no invite, we can only
	// initiate with players that are valid party targets.
	//
	// Note that auto-invites/rejects check AllowedToPartyWith directly as we never actually have a visible incoming
	// join request
	return BAllowedToPartyWith( steamID ) || BHaveIncomingInviteForSteamID( steamID );
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BAllowedToPartyWith( CSteamID steamID ) const
{
	auto *pParty = GetActiveParty();
	if ( pParty )
	{
		if ( pParty->GetMemberIndexBySteamID( steamID ) != -1 )
			return false;
	}

	auto *pFriends = steamapicontext->SteamFriends();
	Assert( pFriends );

	// Check this logic if new values are added
	COMPILE_TIME_ASSERT( k_EFriendRelationshipMax == 8 && k_EPersonaStateMax == 8 );

	// Only friends for now
	return ( pFriends &&
	         pFriends->GetFriendRelationship( steamID ) == k_EFriendRelationshipFriend );

	// Not checking this -- friend may be invisible but still invitable (user won't know if their invite succeeded
	// though)
	//
	// pFriends->GetFriendPersonaState( steamID ) != k_EPersonaStateOffline
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BCurrentMatchOrInviteDisallowsQueuing( bool bCheckParty /* = true */ ) const
{
	auto *pParty = GetActiveParty();
	if ( !pParty || !m_pSOCache )
		{ return false; }

	CSteamID steamID = m_pSOCache->GetOwner();
	for ( int idx = 0; idx < pParty->GetNumMembers(); idx++ )
	{
		if ( !bCheckParty && pParty->GetMember( idx ) != steamID )
			{ continue; }
		if ( pParty->GetMemberActivity( idx ).multiqueue_blocked() )
			{ return true; }
	}

	return false;
}


//-----------------------------------------------------------------------------
bool CTFPartyClient::BCanQueueForMatch( ETFMatchGroup eGroup, CUtlVector< QueueEligibilityData_t >& vecReasons ) const
{
	auto lambdaCopyWReason = [&]( const wchar_t* pwszReason, EDisabledReasonType eType )
	{
		auto pNewReason = vecReasons.AddToTailGetPtr();
		V_wcsncpy( pNewReason->wszCantReason, pwszReason, sizeof( pNewReason->wszCantReason ) );
		pNewReason->m_eReason = eType;
	};

	auto lambdaCopyReason = [&]( const char* pszLocToken, EDisabledReasonType eType )
	{
		auto *pNewReason = vecReasons.AddToTailGetPtr();
		wchar_t *pwszReason = g_pVGuiLocalize->Find( pszLocToken );
		if ( pwszReason )
			{ V_wcscpy_safe( pNewReason->wszCantReason, pwszReason ); }
		pNewReason->m_eReason = eType;
	};

	auto *pMatchDesc = GetMatchGroupDescription( eGroup );
	if ( !pMatchDesc )
		return false;

	// Check for matchmaking bans and display time remaining if we're banned
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return false;

	// Global state preventing?
	{
		if ( !GTFGCClientSystem()->BHealthyGCConnection() )
		{
			lambdaCopyReason( "#TF_MM_NoGC", k_eDisabledType_Network );
		}

		// Is this match group disabled?
		if ( GTFGCClientSystem()->BIsMatchGroupDisabled( eGroup ) )
		{
			lambdaCopyReason( "#TF_Competitive_MatchTypeDisabled", k_eDisabledType_System );
		}

		// Are we already queued for it?
		if ( GTFPartyClient()->BInQueueForMatchGroup( eGroup ) )
		{
			lambdaCopyReason( "#TF_MM_AlreadyQueued", k_eDisabledType_System );
		}

		// If we have a match that doesn't allow for queuing while playing, then prevent queuing
		if ( BCurrentMatchOrInviteDisallowsQueuing( false ) )
		{
			lambdaCopyReason( "#TF_Competitive_MatchRunning", k_eDisabledType_Locked );
		}

		if ( BCurrentMatchOrInviteDisallowsQueuing( true ) )
		{
			lambdaCopyReason( "#TF_Competitive_PartyMatchRunning", k_eDisabledType_Locked );
		}

		if ( !BControllingPartyActions() )
		{
			lambdaCopyReason( "#TF_Matchmaking_OnlyLeaderCanQueue", k_eDisabledType_Criteria );
		}
	}

	auto pParty = GetActiveParty();

	// Match type criteria preventing?
	switch ( eGroup )
	{
		case k_eTFMatchGroup_Casual_12v12:
		{
			// Need to have at least one map selected
			if ( !GetEffectiveGroupCriteria().GetCasualCriteriaHelper().AnySelected() )
			{
				lambdaCopyReason( "#TF_Matchmaking_CantQueue_NoCasualCriteria", k_eDisabledType_Criteria );
			}
		}
		break;
	
		case k_eTFMatchGroup_Event_Placeholder:
		case k_eTFMatchGroup_Ladder_6v6:
		{

		}
		break;

		case k_eTFMatchGroup_MvM_MannUp:
		{
			if ( pParty ? pParty->BAnyMemberWithoutTicket() : !GTFGCClientSystem()->BLocalPlayerInventoryHasMvmTicket() )
			{
				lambdaCopyReason( "#TF_Matchmaking_CantQueue_NoTicket", k_eDisabledType_Criteria );
			}
		}
		// Intentionally fall through for the criteria check

		case k_eTFMatchGroup_MvM_Practice:
		{
			// Need to have at least one map
			CMvMMissionSet searchChallenges;
			GetEffectiveGroupCriteria().GetMvMMissionSet( searchChallenges, eGroup == k_eTFMatchGroup_MvM_MannUp );
			if ( searchChallenges.IsEmpty() )
			{
				lambdaCopyReason( "#TF_Matchmaking_CantQueue_NoMvMCriteria", k_eDisabledType_Criteria );
			}
		}
		break;

		default:
			// Unhandled match type!
			Assert( false );
	}

	// Party state preventing?
	{
		auto &worldStatus = GTFGCClientSystem()->WorldStatus();

		if ( pParty )
		{
			for ( int i=0; i < pParty->GetNumMembers(); ++i )
			{
				auto &activity = pParty->GetMemberActivity( i );
				// Only if we've seen the client's version and gotten world status can we say they are out of date (may not
				// during login).  World status of 0 means no-enforcement.
				bool bOutOfDate = ( worldStatus.active_client_version() != 0 && activity.has_client_version() &&
									activity.client_version() != worldStatus.active_client_version() );
				if ( bOutOfDate )
					{ lambdaCopyReason( "#TF_Competitive_PartyMemberOutOfDate", k_eDisabledType_Network ); }
				if ( !activity.online() )
					{ lambdaCopyReason( "#TF_Competitive_PartyMemberOffline", k_eDisabledType_Network ); }
			}
		}

		EMMPenaltyPool ePenaltyPool = pMatchDesc->GetPenaltyPool();

		// Check to see if we are banned
		{
			CRTime timeExpire = CRTime::RTime32TimeCur();
			int nDuration = -1;
			bool bBanned = GTFGCClientSystem()->BIsBannedFromMatchmaking( ePenaltyPool, &timeExpire, &nDuration );

			if ( bBanned )
			{
				lambdaCopyReason( "#TF_Matchmaking_CantQueue_Ban", k_eDisabledType_Banned );
			}
		}

		// Check if a party member is banned
		if ( pParty && pParty->BAnyMembersBanned( ePenaltyPool ) )
		{
			CUtlVector< CSteamID > vecBannedSteamIDs;
			for( int i=0; i < pParty->GetNumMembers(); ++i )
			{
				CSteamID steamIDMember = pParty->GetMember( i );
				if ( pParty->BMembersIsBanned( steamIDMember, ePenaltyPool ) )
				{
					vecBannedSteamIDs.AddToTail( steamIDMember );
				}
			}

			Assert( vecBannedSteamIDs.Count() );

			BlameNames_t bannedNames( vecBannedSteamIDs,
									  "#TF_Matchmaking_CantQueue_BanParty",
									  "#TF_Matchmaking_CantQueue_VerbSingle",
									  "#TF_Matchmaking_CantQueue_VerbPlural" );
			lambdaCopyWReason( bannedNames.Get(), k_eDisabledType_Banned );
		}

		// Someone in the party doesn't have comp access
		if ( pMatchDesc->BRequiresCompetitiveAccess() )
		{
			// We don't have comp access
			if( !GTFGCClientSystem()->BHasCompetitiveAccess() )
			{
				lambdaCopyReason( "#TF_Competitive_Requirements", k_eDisabledType_Locked );
			}

			// Someone else doesn't have comp access
			if( pParty && pParty->BAnyMemberWithoutCompetitiveAccess() )
			{
				CUtlVector< CSteamID > vecBlameSteamIDs;
				for( int i=0; i < pParty->GetNumMembers(); ++i )
				{
					CSteamID steamIDMember = pParty->GetMember( i );
					if ( pParty->BMemberWithoutCompetitiveAccess( steamIDMember ) )
					{
						vecBlameSteamIDs.AddToTail( steamIDMember );
					}
				}

				BlameNames_t restrictedNames( vecBlameSteamIDs, 
											  "#TF_Competitive_Requirements_Party",
											  "#TF_PartyMemberState_Singular",
											  "#TF_PartyMemberState_Plural" );
				lambdaCopyWReason( restrictedNames.Get(), k_eDisabledType_Locked );
			}
		}
	}

	vecReasons.SortPredicate( []( const QueueEligibilityData_t& left, const QueueEligibilityData_t& right ) 
	{
		return left.m_eReason > right.m_eReason;
	} );

	return vecReasons.IsEmpty();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::RequestQueueForMatch( ETFMatchGroup eMatchGroup )
{
	// Can't re-queue until we're confirmed out of queue, see BInQueue semantics
	if ( BHavePendingQueueMsg( eMatchGroup ) || BInQueueForMatchGroup( eMatchGroup ) )
		{ return; }

	auto *pReliable = new ReliableMsgPartyQueueForMatch;
	pReliable->Msg().Body().set_match_group( eMatchGroup );
	if ( m_unActivePartyID )
		{ pReliable->Msg().Body().set_party_id( m_unActivePartyID ); }

	// Append any un-sent options so we are never in queue, however briefly, with options we had selected prior to
	// requesting queuing.  We may also have a pending SetOptions message, that's okay, since these will stack with that
	// and come in afterwards.
	CTFPartyOptions pendingOptions;
	if ( BMakeUpdateMsg( pendingOptions ) )
		{ pReliable->Msg().Body().mutable_final_options()->CopyFrom( pendingOptions ); }

	PartyMsg( "Requesting queue for %s\n", GetMatchGroupName( eMatchGroup ) );
	PartyDbg( "Sending PartyQueueForMatch:\n%s", pReliable->Msg().Body().DebugString().c_str() );
	GTFGCClientSystem()->ReliableMsgQueue().Enqueue( pReliable );
	SetPendingQueueMsg( eMatchGroup, true );
	// See comment in UpdateActiveParty about when InQueue changes.
	if ( !BInQueueForMatchGroup( eMatchGroup ) )
	{
		// Force a party update, since we predict in-queue state when we have a pending message.  Otherwise the state
		// change wouldn't fire until the GC acknowledged things.
		UpdateActiveParty();
	}
}

//-----------------------------------------------------------------------------
void CTFPartyClient::RequestQueueForStandby()
{
	// Can't re-queue until we're confirmed out of queue, see BInQueue semantics
	if ( !BHaveActiveParty() || m_bPendingStandbyQueueMsg || !BCanQueueForStandby() || BInStandbyQueue() )
		{ return; }

	auto *pReliable = new ReliableMsgPartyQueueForStandby;
	pReliable->Msg().Body().set_party_id( m_unActivePartyID );
	pReliable->Msg().Body().set_party_lobby_id( GetActiveParty()->GetAssociatedLobbyID() );

	PartyMsg( "Requesting queue to join party's lobby\n" );
	PartyDbg( "Sending PartyQueueForStandby:\n%s", pReliable->Msg().Body().DebugString().c_str() );
	GTFGCClientSystem()->ReliableMsgQueue().Enqueue( pReliable );
	m_bPendingStandbyQueueMsg = true;
	// See comment in UpdateActiveParty about when InQueue changes
	if ( !BInStandbyQueue() )
	{
		m_bInStandbyQueue = true;
		OnInStandbyQueueChanged();
	}
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CancelMatchQueueRequest( ETFMatchGroup eMatchGroup )
{
	// BInQueue remains true until we're confirmed out of queue, see BInQueue semantics comment
	if ( !BInQueueForMatchGroup( eMatchGroup ) || BHavePendingQueueCancelMsg( eMatchGroup ) )
		{ return; }

	// TODO(Universal Parties): Race/prediction?
	auto *pReliable = new ReliableMsgPartyRemoveFromQueue;
	pReliable->Msg().Body().set_match_group( eMatchGroup );
	if ( m_unActivePartyID )
		{ pReliable->Msg().Body().set_party_id( m_unActivePartyID ); }

	PartyMsg( "Requesting exit queue for %s\n", GetMatchGroupName( eMatchGroup ) );
	PartyDbg( "Sending PartyRemoveFromQueue:\n%s", pReliable->Msg().Body().DebugString().c_str() );
	SetPendingQueueCancelMsg( eMatchGroup, true );
	GTFGCClientSystem()->ReliableMsgQueue().Enqueue( pReliable );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CancelStandbyQueueRequest()
{
	// BInQueue remains true until we're confirmed out of queue, see BInQueue semantics comment
	if ( !BInStandbyQueue() || m_bPendingStandbyQueueCancelMsg )
		{ return; }

	// TODO(Universal Parties): Race/prediction?
	auto *pReliable = new ReliableMsgPartyRemoveFromStandbyQueue;
	if ( m_unActivePartyID )
		{ pReliable->Msg().Body().set_party_id( m_unActivePartyID ); }

	PartyMsg( "Requesting exit standby queue\n" );
	PartyDbg( "Sending PartyRemoveFromStandbyQueue:\n%s", pReliable->Msg().Body().DebugString().c_str() );
	m_bPendingStandbyQueueCancelMsg = true;
	GTFGCClientSystem()->ReliableMsgQueue().Enqueue( pReliable );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::InternalSetInQueueForMatchGroup( ETFMatchGroup eMatchGroup, bool bSet )
{
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return; }

	int idxQueue = m_vecInQueue.Find( eMatchGroup );
	bool bQueued = idxQueue != m_vecInQueue.InvalidIndex();
	if ( bQueued && !bSet )
		{ m_vecInQueue.Remove( idxQueue ); }
	else if ( bSet && !bQueued )
		{ m_vecInQueue.AddToTail( eMatchGroup ); }
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHaveAnyPendingQueueMsg() const
{
	for ( int iMatchGroup = k_eTFMatchGroup_First; iMatchGroup < ETFMatchGroup_ARRAYSIZE; iMatchGroup++ )
	{
		if ( !ETFMatchGroup_IsValid( iMatchGroup ) )
			{ continue; }
		if ( m_arbPendingQueueMsg[iMatchGroup] )
			{ return true; }
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHavePendingQueueMsg( ETFMatchGroup eMatchGroup ) const
{
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return false; }

	return m_arbPendingQueueMsg[eMatchGroup];
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SetPendingQueueMsg( ETFMatchGroup eMatchGroup, bool bSet )
{
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return; }

	m_arbPendingQueueMsg[eMatchGroup] = bSet;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHavePendingQueueCancelMsg( ETFMatchGroup eMatchGroup ) const
{
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return false; }

	return m_arbPendingQueueCancelMsg[eMatchGroup];
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SetPendingQueueCancelMsg( ETFMatchGroup eMatchGroup, bool bSet )
{
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return; }

	m_arbPendingQueueCancelMsg[eMatchGroup] = bSet;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SaveCasualCriteria()
{
	GetLocalGroupCriteria().SaveCasualCriteriaToFile( s_pszCasualCriteriaSaveFileName );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::LoadSavedCasualCriteria()
{
	MutLocalGroupCriteria().LoadCasualCriteriaFromFile( s_pszCasualCriteriaSaveFileName );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::ReadConVarPreferences()
{
	if ( !m_bInit )
		{ return; }

	// Custom ping
	{
		bool bEnabled = tf_mm_custom_ping_enabled.GetBool();
		uint32 unValue = bEnabled ? (uint32)Max( 0, tf_mm_custom_ping.GetInt() ) : 0u;

		MutLocalGroupCriteria().SetCustomPingTolerance( unValue );
	}

	// Party join-request mode
	int nRawJoinRequestMode = tf_party_join_request_mode.GetInt();
	if ( nRawJoinRequestMode == -1 )
	{
		SetPartyJoinRequestMode( k_ePartyJoinRequestMode_Current_Default );
	}
	else
	{
		SetPartyJoinRequestMode( (EPartyJoinRequestMode)Clamp( nRawJoinRequestMode,
		                                                       (int)k_ePartyJoinRequestMode_Min,
		                                                       (int)k_ePartyJoinRequestMode_Max ) );
	}

	// Ignore incoming invites
	SetIgnorePartyInvites( tf_party_ignore_invites.GetBool() );
}

//-----------------------------------------------------------------------------
CTFPartyClient::EPartyJoinRequestMode CTFPartyClient::GetPartyJoinRequestMode() const
{
	return m_ePrefPartyJoinRequestMode;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SetPartyJoinRequestMode( EPartyJoinRequestMode eMode )
{
	if ( m_ePrefPartyJoinRequestMode != eMode )
	{
		m_ePrefPartyJoinRequestMode = eMode;
		// This will trigger a OnChanged and call us again, make sure we set the mode first so we don't get in a loop.
		tf_party_join_request_mode.SetValue( (int)eMode );
		OnPartyPrefChanged();
	}
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::GetIgnorePartyInvites() const
{
	return m_bPrefIgnoreInvites;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SetIgnorePartyInvites( bool bIgnore )
{
	if ( m_bPrefIgnoreInvites != bIgnore )
	{
		m_bPrefIgnoreInvites = bIgnore;
		// This will trigger a OnChanged and call us again, make sure we set the mode first so we don't get in a loop.
		tf_party_ignore_invites.SetValue( bIgnore );
		OnPartyPrefChanged();
	}
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BInQueueForMatchGroup( ETFMatchGroup eMatchGroup ) const
{
	// See UpdateActiveParty for where the edges of this are defined w.r.t. pending messages
	if ( !AssertValidMatchGroup( eMatchGroup ) )
		{ return false; }

	return m_vecInQueue.Find( eMatchGroup ) != m_vecInQueue.InvalidIndex();
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumQueuedMatchGroups() const
{
	return m_vecInQueue.Count();
}

//-----------------------------------------------------------------------------
ETFMatchGroup CTFPartyClient::GetQueuedMatchGroupByIdx( int idx ) const
{
	Assert( idx >= 0 && idx < m_vecInQueue.Count() );
	if ( idx < 0 || idx >= m_vecInQueue.Count() )
		return k_eTFMatchGroup_Invalid;

	return m_vecInQueue[ idx ];
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BInStandbyQueue() const
{
	// See UpdateActiveParty for where the edges of this are defined w.r.t. pending messages
	return m_bInStandbyQueue;
}


//-----------------------------------------------------------------------------
bool CTFPartyClient::BCanQueueForStandby() const
{
	// Don't need to check the party since standby queue is per-individual
	if ( BCurrentMatchOrInviteDisallowsQueuing( /* bCheckParty */ false ) || BInStandbyQueue() || !BHaveActiveParty() )
		{ return false; }

	// Does our party have a match?
	PlayerGroupID_t nPartyAssociatedLobby = GetActiveParty()->GetAssociatedLobbyID();
	if ( nPartyAssociatedLobby == 0u )
		{ return false; }

	// Does it allow this?
	auto *pMatchDesc = GetMatchGroupDescription( GetActiveParty()->GetAssociatedLobbyMatchGroup() );
	if ( !pMatchDesc || !pMatchDesc->BAllowsPartyJoins() )
		{ return false; }

	// Are we already in it?  (Note that we *can* queue for standby if we're in *some other* match)
	if ( GTFGCClientSystem()->BHaveLiveMatch() && GTFGCClientSystem()->GetLiveMatchLobbyID() == nPartyAssociatedLobby )
		{ return false; }

	// Are we already invited to it?
	for ( int idxInvite = 0; idxInvite < GTFGCClientSystem()->GetNumMatchInvites(); idxInvite++ )
	{
		if ( GTFGCClientSystem()->GetMatchInvite( idxInvite ).nLobbyID == nPartyAssociatedLobby )
			{ return false; }
	}

	return true;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::Think()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if ( m_bQueuedEffectiveCriteriaChangeEvent )
		{ OnEffectiveCriteriaChanged(); }

	CheckSendUpdates();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CheckSendUpdates()
{
	// Should we flush any changes asap rather than lazily buffering them.  If we are in queue and this actually affects
	// us, we want to flush all changes, but otherwise we will lazily buffer them.
	bool bSyncUrgent = BInAnyMatchQueue();
	// Are we in a state where we want to keep our criteria sync'd with the GC at all?
	bool bSync = ( GetNumPartyMembers() > 1 ) || bSyncUrgent;

	// No action
	if ( !bSync )
		{ return; }

	// Anything to send?
	uint32_t dirtyFlags = CalcDirtyFlags();
	if ( m_bSentInitialCriteria && dirtyFlags == eDirty_None )
		{ return; }

	//
	// Dirty criteria and we want to be in sync, see if we should send an update now
	//

	// Track how long changes have been pending
	if ( m_flPendingChangesTime == -1.f )
		{ m_flPendingChangesTime = Plat_FloatTime(); }

	auto &cvDelay = tf_mm_party_send_criteria_delay;
	auto &cvMinDelay = tf_mm_party_send_criteria_delay_minimum;
	auto &flLastUpdate = m_flLastCriteriaUpdate;
	bool bSendNow = ( m_flPendingChangesTime != -1.f &&
	                  // Not if there's still reliable criteria in queue
	                  !m_bPendingReliableCriteriaMsg &&
	                  // Wait for the nominal delay unless its urgent
	                  ( bSyncUrgent || m_flPendingChangesTime + cvDelay.GetFloat() < Plat_FloatTime() ) &&
	                  // But always wait for the minimum delay (if the user is e.g. spamming a checkbox)
	                  ( flLastUpdate == -1.f || ( flLastUpdate + cvMinDelay.GetFloat() < Plat_FloatTime() ) ) );

	if ( !bSendNow )
		{ return; }

	//
	// Build update
	//


	auto *pReliable = new ReliableMsgPartySetOptions;
	pReliable->Msg().Body().set_party_id( m_unActivePartyID );
	CTFPartyOptions *pOptions = pReliable->Msg().Body().mutable_options();

	bool bMadeOptions = BMakeUpdateMsg( *pOptions );
	if ( !bMadeOptions )
	{
		// This is fine, we may have marked criteria dirty but then changed it back to the previous state, resulting in
		// a no-op.
		PartyDbg( "CheckSendUpdates - Had dirty state, but turned out to be in sync when crafting update\n" );
		m_flPendingChangesTime = -1.f;
		return;
	}

	// Send
	PartyDbg( "Sending Party SetOptions:\n%s", pReliable->Msg().Body().DebugString().c_str() );
	m_flLastCriteriaUpdate = Plat_FloatTime();
	m_flPendingChangesTime = -1.f;
	GTFGCClientSystem()->ReliableMsgQueue().Enqueue( pReliable );
	m_bPendingReliableCriteriaMsg = true;
	m_unPendingReliableCriteriaMsgParty = m_unActivePartyID;
}

//-----------------------------------------------------------------------------
uint32_t CTFPartyClient::CalcDirtyFlags()
{
	// Note we don't consider !m_bSentInitial dirty -- we set the overwrite flag in our initial update, and
	// !m_bSentInitial forces an initial update, so the server *will* match our last sent after that.  (We clear last
	// sent to defaults in CheckResetSentOptions)
	uint32_t dirtyFlags = eDirty_None;
	if ( !SyncedUIStateEqual( m_lastSentUIState, m_localUIState ) && BIsPartyLeader() )
		{ dirtyFlags |= eDirty_UIState; }

	if ( BControllingPartyActions() && m_bGroupCriteriaChanged )
		{ dirtyFlags |= eDirty_GroupCriteria; }
	if ( m_bPerPlayerCriteriaChanged )
		{ dirtyFlags |= eDirty_PlayerCriteria; }

	return dirtyFlags;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BMakeUpdateMsg( CTFPartyOptions &msgOptions )
{
	uint32_t dirtyFlags = CalcDirtyFlags();

#if defined( _DEBUG )
	// Debug only - Guard against forgetting to set dirty flags somewhere
	if ( !( dirtyFlags & ( eDirty_PlayerCriteria | eDirty_GroupCriteria ) ) )
	{
		CTFGroupMatchCriteria groupDelta;
		CTFPerPlayerMatchCriteria perPlayerDelta;
		bool bCheckGroup = BControllingPartyActions(); // We purposefully ignore changes when not in control
		bool bGroupChanges = bCheckGroup && groupDelta.MakeDelta( m_lastSentGroupCriteria, m_localGroupCriteria );
		bool bPlayerChanges = perPlayerDelta.MakeDelta( m_lastSentPerPlayerCriteria, m_localPerPlayerCriteria );
		AssertMsg( !bGroupChanges && !bPlayerChanges, "Criteria was not marked changed but changed" );
	}
#endif // defined( _DEBUG )

	if ( dirtyFlags == eDirty_None && m_bSentInitialCriteria )
		{ return false; }

	//
	// Build update
	//

	bool bOverwrite = !m_bSentInitialCriteria;
	if ( bOverwrite )
		{ msgOptions.set_overwrite_existing( true ); }

	// UI State
	if ( dirtyFlags & eDirty_UIState )
	{
		if ( !SyncedUIStateEqual( m_lastSentUIState, m_localUIState ) )
		{
			msgOptions.mutable_player_uistate()->CopyFrom( m_localUIState );
			m_lastSentUIState = m_localUIState;
		}
		else
		{
			dirtyFlags = ( dirtyFlags & ~eDirty_UIState );
		}
	}
	else if ( bOverwrite )
	{
		m_lastSentUIState.Clear();
	}

	// Group Criteria
	if ( dirtyFlags & eDirty_GroupCriteria )
	{
		if ( bOverwrite )
		{
			msgOptions.mutable_group_criteria()->CopyFrom( m_localGroupCriteria.Proto() );
		}
		else
		{
			// It may be that we were flagged dirty but turned out not to be if our state changed to and fro.  Wipe
			// dirty flag so we know at the end if we ended up with nothing.
			CTFGroupMatchCriteria msgGroupCriteria;
			bool bDelta = msgGroupCriteria.MakeDelta( m_lastSentGroupCriteria, m_localGroupCriteria );
			if ( bDelta )
				{ msgOptions.mutable_group_criteria()->CopyFrom( msgGroupCriteria.Proto() ); }
			else
				{ dirtyFlags = ( dirtyFlags & ~eDirty_GroupCriteria ); }
		}
		m_bGroupCriteriaChanged = false;
		m_lastSentGroupCriteria = m_localGroupCriteria;
	}
	else if ( bOverwrite )
	{
		m_lastSentGroupCriteria = CTFGroupMatchCriteria();
	}

	// Player criteria
	if ( dirtyFlags & eDirty_PlayerCriteria )
	{
		if ( bOverwrite )
		{
			msgOptions.mutable_player_criteria()->CopyFrom( m_localPerPlayerCriteria.Proto() );
		}
		else
		{
			// It may be that we were flagged dirty but turned out not to be if our state changed to and fro.  Wipe
			// dirty flag so we know at the end if we ended up with nothing.
			CTFPerPlayerMatchCriteria msgPlayerCriteria;
			bool bDelta = msgPlayerCriteria.MakeDelta( m_lastSentPerPlayerCriteria, m_localPerPlayerCriteria );
			if ( bDelta )
				{ msgOptions.mutable_player_criteria()->CopyFrom( msgPlayerCriteria.Proto() ); }
			else
				{ dirtyFlags = ( dirtyFlags & ~eDirty_PlayerCriteria ); }

		}
		m_bPerPlayerCriteriaChanged = false;
		m_lastSentPerPlayerCriteria = m_localPerPlayerCriteria;
	}
	else if ( bOverwrite )
	{
		m_lastSentPerPlayerCriteria = CTFPerPlayerMatchCriteria();
	}

	m_bSentInitialCriteria = true;

	// If our state changed to and fro we may have decided some of these flags were empty above, and thus the message
	// ended up empty
	return ( !m_bSentInitialCriteria || dirtyFlags != eDirty_None );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SendPartyChat( const char *pszMessage )
{
	// Check length (the UI should probably be enforcing this)
	int nLen = V_strlen( pszMessage ) + 1;
	if ( nLen > MAX_PARTY_CHAT_MSG )
	{
		Assert( !"The UI should really be enforcing max party message length" );
		return;
	}

	// If we are not in a situation where anyone can hear us, just bounce the chat around locally
	if ( !BHaveActiveParty() )
	{
		PostChatGameEvent( steamapicontext->SteamUser()->GetSteamID(), k_eTFPartyChatType_MemberChat, pszMessage );
		return;
	}

	bool bSent = false;
	CProtoBufMsg<CMsgPartySendChat> msg( k_EMsgGCParty_SendChat );
	msg.Body().set_party_id( m_unActivePartyID );
	msg.Body().set_msg( pszMessage );

	// Message is likely to be lost if not healthy
	if ( GTFGCClientSystem()->BHealthyGCConnection() )
		{ bSent = GCClientSystem()->BSendMessage( msg ); }

	if ( !bSent )
	{
		PartyWarn( "Failed to send party chat:\n%s\n", msg.Body().DebugString().c_str() );
		PostChatGameEvent( steamapicontext->SteamUser()->GetSteamID(), k_eTFPartyChatType_Synthetic_SendFailed,
		                   pszMessage );
		return;
	}

	PartyDbg( "Sent party chat:\n%s\n", msg.Body().DebugString().c_str() );
	return;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::ReceivedPartyChatMsg( CMsgPartyChatMsg &msg )
{
	ETFPartyChatType eType = msg.type();
	CSteamID steamIDActor = CSteamID( msg.actor_id() );
	auto &strMsg = msg.msg();

	if ( msg.has_actor_id() && ( !steamIDActor.IsValid() ||
	                             !steamIDActor.BIndividualAccount() ||
	                             steamIDActor.GetEUniverse() != GetUniverse() ) )
	{
		Assert( !"Bogus party chat actor (corrupt?)" );
		PartyWarn( "Received party chat message with invalid actor, type %d, actor %s: \"%s\"\n",
		           eType, steamIDActor.Render(), strMsg.c_str() );
		return;
	}

	if ( !ETFPartyChatType_IsValid( eType ) )
	{
		Assert( !"Unhandled party chat message type" );
		PartyWarn( "Received party chat message with unknown type %d, actor %s: \"%s\"\n",
		           eType, steamIDActor.Render(), strMsg.c_str() );
		return;
	}

	PostChatGameEvent( steamIDActor, eType, strMsg.c_str() );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::PostChatGameEvent( const CSteamID &steamIDPoster,
                                        ETFPartyChatType eType,
                                        const char *pszText )
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "party_chat" );
	if ( !pEvent )
		{ return; }
	pEvent->SetString( "steamid", CFmtStr("%llu", steamIDPoster.ConvertToUint64() ) );
	pEvent->SetString( "text", pszText );
	pEvent->SetInt( "type", eType );
	gameeventmanager->FireEventClientSide( pEvent );

	PartyDbg( "[Chat] %s [%s (%d)]: %s\n",
	          steamIDPoster.Render(), ETFPartyChatType_Name( eType ).c_str(), eType, pszText );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnSetOptionsReply( const CProtoBufMsg< CMsgPartySetOptions > &msg,
                                        const CProtoBufMsg< CMsgPartySetOptionsResponse > &reply )
{
	NoteUnused( msg );
	NoteUnused( reply );
	Assert( m_bPendingReliableCriteriaMsg );
	m_bPendingReliableCriteriaMsg = false;
	m_unPendingReliableCriteriaMsgParty = 0u;

	// TODO(Universal Parties): In debug, Diff Party object with known options
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnQueueForMatchReply( const CProtoBufMsg< CMsgPartyQueueForMatch > &msg,
                                           const CProtoBufMsg< CMsgPartyQueueForMatchResponse > &reply )
{
	ETFMatchGroup eMatchGroup = msg.Body().match_group();
	Assert( BHavePendingQueueMsg( eMatchGroup ) );
	NoteUnused( reply );
	SetPendingQueueMsg( eMatchGroup, false );
	// Because we predict queue state, we need to double-check that we actually ended up in queue and fire the right
	// events etc if not.  If queuing succeeded, this should be a no-op
	UpdateActiveParty();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnQueueForStandbyReply( const CProtoBufMsg< CMsgPartyQueueForStandby > &msg,
                                             const CProtoBufMsg< CMsgPartyQueueForStandbyResponse > &reply )
{
	NoteUnused( msg );
	NoteUnused( reply );
	Assert( m_bPendingStandbyQueueMsg );
	m_bPendingStandbyQueueMsg = false;
	// Because we predict queue state, we need to double-check that we actually ended up in queue and fire the right
	// events etc if not.  If queuing succeeded, this should be a no-op
	UpdateActiveParty();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnRemoveFromQueueReply( const CProtoBufMsg< CMsgPartyRemoveFromQueue > &msg,
                                             const CProtoBufMsg< CMsgPartyRemoveFromQueueResponse > &reply )
{
	NoteUnused( reply );
	ETFMatchGroup eMatchGroup = msg.Body().match_group();
	Assert( BHavePendingQueueCancelMsg( eMatchGroup ) );
	SetPendingQueueCancelMsg( eMatchGroup, false );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnRemoveFromStandbyQueueReply( const CProtoBufMsg< CMsgPartyRemoveFromStandbyQueue > &msg,
                                                    const CProtoBufMsg< CMsgPartyRemoveFromStandbyQueueResponse > &reply )
{
	NoteUnused( msg );
	NoteUnused( reply );
	Assert( m_bPendingStandbyQueueCancelMsg );
	m_bPendingStandbyQueueCancelMsg = false;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::LeaveActiveParty()
{
	// TODO(Universal Parties): Fixup GC side ExitMatchmaking message and give us one
	if ( !m_unActivePartyID )
		{ return; }

	PartyMsg( "Sending request to leave party %llu\n", m_unActivePartyID );
	CProtoBufMsg<CMsgExitMatchmaking> msg( k_EMsgGCExitMatchmaking );
	msg.Body().set_party_id( m_unActivePartyID );
	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumIncomingInvites() const
{
	return m_vecIncomingInvites.Count();
}

//-----------------------------------------------------------------------------
CTFPartyInvite *CTFPartyClient::GetIncomingInvite( int i ) const
{
	int nCount = m_vecIncomingInvites.Count();
	Assert( i >= 0 && i < nCount );
	if ( i < 0 || i >= nCount )
		{ return nullptr; }

	return m_vecIncomingInvites[ i ].pInvite;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHaveIncomingInviteForSteamID( CSteamID steamID ) const
{
	int nNumIncomingInvites = GetNumIncomingInvites();
	for ( int i = 0; i < nNumIncomingInvites; i++ )
	{
		if ( GetIncomingInvite( i )->GetInviter() == steamID )
			{ return true; }
	}

	return false;
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumOutgoingInvites() const
{
	return m_vecOutgoingInvites.Count();
}

//-----------------------------------------------------------------------------
CSteamID CTFPartyClient::GetOutgoingInvite( int i ) const
{
	int nCount = m_vecOutgoingInvites.Count();
	Assert( i >= 0 && i < nCount );
	if ( i < 0 || i >= nCount )
		{ return CSteamID(); }

	return m_vecOutgoingInvites[ i ];
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHaveOutgoingInviteForSteamID( CSteamID steamID ) const
{
	int nNumOutgoingInvites = GetNumOutgoingInvites();
	for ( int i = 0; i < nNumOutgoingInvites; i++ )
	{
		if ( GetOutgoingInvite( i ) == steamID )
			{ return true; }
	}

	return false;
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumOutgoingJoinRequests() const
{
	return m_vecOutgoingJoinRequests.Count();
}

//-----------------------------------------------------------------------------
CTFPartyInvite *CTFPartyClient::GetOutgoingJoinRequest( int i ) const
{
	int nCount = m_vecOutgoingJoinRequests.Count();
	Assert( i >= 0 && i < nCount );
	if ( i < 0 || i >= nCount )
		{ return nullptr; }

	return m_vecOutgoingJoinRequests[ i ].pInvite;
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHaveOutgoingJoinRequestForSteamID( CSteamID steamID ) const
{
	int nNumOutgoingJoinRequests = GetNumOutgoingJoinRequests();
	for ( int i = 0; i < nNumOutgoingJoinRequests; i++ )
	{
		if ( GetOutgoingJoinRequest( i )->GetInviter() == steamID )
			{ return true; }
	}

	return false;
}

//-----------------------------------------------------------------------------
int CTFPartyClient::GetNumIncomingJoinRequests() const
{
	return m_vecIncomingJoinRequests.Count();
}

//-----------------------------------------------------------------------------
CSteamID CTFPartyClient::GetIncomingJoinRequest( int i ) const
{
	int nCount = m_vecIncomingJoinRequests.Count();
	Assert( i >= 0 && i < nCount );
	if ( i < 0 || i >= nCount )
		{ return CSteamID(); }

	return m_vecIncomingJoinRequests[ i ];
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::BHaveIncomingJoinRequestForSteamID( CSteamID steamID ) const
{
	int nNumIncomingJoinRequests = GetNumIncomingJoinRequests();
	for ( int i = 0; i < nNumIncomingJoinRequests; i++ )
	{
		if ( GetIncomingJoinRequest( i ) == steamID )
			{ return true; }
	}

	return false;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::UpdateIncomingInvitesOrOutgoingJoinRequests()
{
	CUtlVector< PartyInvite_t > vecNewIncomingInvites;
	CUtlVector< PartyInvite_t > vecNewOutgoingJoinRequests;
	bool bIncomingInvitesChanged = false;
	bool bOutgoingJoinRequestsChanged = false;
	bool bIgnoreInvites = GetIgnorePartyInvites();

	// Invites we're going to silently drop
	CUtlVector< CSteamID > vecIgnoredInvites;

	if ( m_pSOCache )
	{
		CSharedObjectTypeCache *pTypeCache = m_pSOCache->FindBaseTypeCache( CTFPartyInvite::k_nTypeID );
		if ( pTypeCache )
		{
			int nCount = pTypeCache->GetCount();
			for ( int i = 0; i < nCount; i++ )
			{
				CTFPartyInvite *pInvite = static_cast< CTFPartyInvite * >( pTypeCache->GetObject( i ) );
				CSteamID steamIDInviter = pInvite->GetInviter();
				auto fnFindOldInvite = [steamIDInviter](const PartyInvite_t &oldInvite) -> bool \
					{
						// The old CTFPartyInvite pointer is dead here!
						return oldInvite.steamIDInviter == steamIDInviter;
					};

				switch ( pInvite->GetType() )
				{
					case CTFParty::ePending_Invite:
					{
						if ( bIgnoreInvites || !BAllowedToPartyWith( steamIDInviter ) )
						{
							// Will nuke below
							vecIgnoredInvites.AddToTail( pInvite->GetInviter() );
						}
						else
						{
							int idxOld = m_vecIncomingInvites.FindPredicate( fnFindOldInvite );
							if ( idxOld == m_vecIncomingInvites.InvalidIndex() )
								{ bIncomingInvitesChanged = true; }
							vecNewIncomingInvites.AddToTail( PartyInvite_t( { pInvite, steamIDInviter } ) );
						}
					}
					break;
					case CTFParty::ePending_JoinRequest:
					{
						int idxOld = m_vecOutgoingJoinRequests.FindPredicate( fnFindOldInvite );
						if ( idxOld == m_vecOutgoingJoinRequests.InvalidIndex() )
							{ bOutgoingJoinRequestsChanged = true; }
						vecNewOutgoingJoinRequests.AddToTail( PartyInvite_t( { pInvite, steamIDInviter } ) );
					}
					break;
					default:
						Assert( !"Unhandled enum value" );
						break;
				}
			}
		}
	}

	// Any invites we should not present to user/UI?
	FOR_EACH_VEC( vecIgnoredInvites, idxIgnored )
	{
		PartyDbg( "Auto-rejecting invite from %s - ignore party invites [%d], allowed to party with [%d]\n",
		          vecIgnoredInvites[idxIgnored].Render(), bIgnoreInvites,
		          BAllowedToPartyWith( vecIgnoredInvites[idxIgnored] ) );
		CancelOutgoingJoinRequestOrIncomingInvite( vecIgnoredInvites[idxIgnored] );
	}

	// If we didn't find any new objects, the lists changed only if there were more previously
	if ( !bIncomingInvitesChanged && vecNewIncomingInvites.Count() != m_vecIncomingInvites.Count() )
		{ bIncomingInvitesChanged = true; }

	if ( !bOutgoingJoinRequestsChanged && vecNewOutgoingJoinRequests.Count() != m_vecOutgoingJoinRequests.Count() )
		{ bOutgoingJoinRequestsChanged = true; }

	if ( bIncomingInvitesChanged )
	{
		m_vecIncomingInvites = vecNewIncomingInvites;
		OnIncomingInvitesChanged();
	}

	if ( bOutgoingJoinRequestsChanged )
	{
		m_vecOutgoingJoinRequests = vecNewOutgoingJoinRequests;
		OnOutgoingJoinRequestsChanged();
	}
}

//-----------------------------------------------------------------------------
bool CTFPartyClient::UpdateActiveParty()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	CTFParty *pActiveParty = nullptr;
	if ( m_pSOCache )
	{
		CSharedObjectTypeCache *pTypeCache = m_pSOCache->FindBaseTypeCache( CTFParty::k_nTypeID );
		if ( pTypeCache && pTypeCache->GetCount() > 0 )
		{
			AssertMsg1( pTypeCache->GetCount() == 1,
			            "Client has %d party objects in his cache!  He should only have 1.", pTypeCache->GetCount() );
			pActiveParty = static_cast<CTFParty*>( pTypeCache->GetObject( pTypeCache->GetCount() - 1 ) );
		}
	}

	CSteamID steamIDLocal = steamapicontext->SteamUser()->GetSteamID();
	int idxLocalPlayer = pActiveParty ? pActiveParty->GetMemberIndexBySteamID( steamIDLocal ) : -1;

	// Comparing queue state
	//
	// Once we have a pending message, tell consumers we are in queue.  We keep reporting in queue even if we have a
	// pending cancel message - since we accept inbound matches during that race period.
	//
	// TODO: Once we have an accept-reject-match step we could predict queue exit as well, and auto-reject incoming
	//       matches in the short window.
	CUtlMap< ETFMatchGroup, bool > mapChangedQueues( DefLessFunc( ETFMatchGroup ) );
	for ( int iMatchGroup = k_eTFMatchGroup_First; iMatchGroup < ETFMatchGroup_ARRAYSIZE; iMatchGroup++ )
	{
		if ( !ETFMatchGroup_IsValid( iMatchGroup ) )
			{ continue; }
		ETFMatchGroup eMatchGroup = (ETFMatchGroup)iMatchGroup;
		bool bWasQueued = BInQueueForMatchGroup( eMatchGroup );
		bool bNowQueued = ( BHavePendingQueueMsg( eMatchGroup ) ||
		                    ( pActiveParty && pActiveParty->BQueuedForMatchGroup( eMatchGroup ) ) );
		if ( bWasQueued != bNowQueued )
			{ mapChangedQueues.Insert( eMatchGroup, bNowQueued ); }
	}

	// Standby queue state
	bool bWasQueuedStandby = m_bInStandbyQueue;
	bool bNowQueuedStandby = idxLocalPlayer != -1 ? pActiveParty->BMemberRequestingLobbyStandby( idxLocalPlayer ) : false;
	bool bQueueStandbyChanged = ( bWasQueuedStandby != bNowQueuedStandby );

	bool bPartyChanged = ( pActiveParty != m_pActiveParty );

	// Compare leaders
	Assert( steamapicontext );
	Assert( steamapicontext->SteamUser() );
	bool bIsLeader = pActiveParty ? pActiveParty->GetLeader() == steamIDLocal : true;
	bool bIsLeaderChanged = ( bIsLeader != m_bIsActivePartyLeader );

	// Compare criteria
	bool bWasUsingActiveCriteria = BEffectiveGroupCriteriaFromParty();
	auto &newCriteria = pActiveParty ? pActiveParty->Obj().group_criteria() \
	                                 : CTFGroupMatchCriteriaProto::default_instance();
	bool bActiveCriteriaChanged = CTFGroupMatchCriteria().MakeDelta( m_activePartyCriteria,
	                                                                 ConstRefTFGroupMatchCriteria( newCriteria ) );

	// Compare party members
	CUtlVectorFixed< Member_t, MAX_PARTY_SIZE > vecNewMembers;
	CUtlVectorFixed< CSteamID, MAX_PARTY_SIZE > vecAddedMembers;
	CUtlVectorFixed< CSteamID, MAX_PARTY_SIZE > vecRemovedMembers;
	CUtlVectorFixed< Member_t, MAX_PARTY_SIZE > vecMembersChangedOnlineState;
	if ( pActiveParty )
	{
		for ( int idx = 0; idx < pActiveParty->GetNumMembers(); idx++ )
		{
			bool bOnline = pActiveParty->GetMemberActivity( idx ).online();
			vecNewMembers.AddToTail( Member_t{ pActiveParty->GetMember( idx ), bOnline } );
		}
		vecNewMembers.SortPredicate( []( const Member_t &a, const Member_t &b ) {
			                             return a.steamID.GetAccountID() < b.steamID.GetAccountID();
		                             } );
	}
	else
	{
		// Alll by mysellllf
		vecNewMembers.AddToTail( Member_t{ steamapicontext->SteamUser()->GetSteamID(), /* bOnline */ true } );
	}

	int idxNew = 0;
	int idxOld = 0;
	while ( idxNew < vecNewMembers.Count() || idxOld < m_activePartyMembers.Count() )
	{
		auto newID = ( idxNew < vecNewMembers.Count() ) ? vecNewMembers[idxNew].steamID.GetAccountID() : 0;
		auto oldID = ( idxOld < m_activePartyMembers.Count() ) ? m_activePartyMembers[idxOld].steamID.GetAccountID() : 0;

		if ( oldID && ( !newID || oldID < newID ) )
		{
			// Delete idxOld
			vecRemovedMembers.AddToTail( m_activePartyMembers[idxOld].steamID );
			idxOld++;
		}
		else if ( newID && ( !oldID || newID < oldID ) )
		{
			// Add idxNew
			vecAddedMembers.AddToTail( vecNewMembers[idxNew].steamID );
			idxNew++;
		}
		else
		{
			// Same, online state different?
			if ( m_activePartyMembers[idxOld].bOnline != vecNewMembers[idxNew].bOnline )
				{ vecMembersChangedOnlineState.AddToTail( vecNewMembers[idxNew] ); }

			Assert( newID == oldID && oldID != 0 && newID != 0 );
			idxNew++;
			idxOld++;
		}
	}

	//
	// Compare pending players
	//

	// See if we are auto-acting on any
	EPartyJoinRequestMode eMode = GetPartyJoinRequestMode();
	// In request to join mode, we take no automatic actions.
	bool bAutoAccept = ( eMode == k_ePartyJoinRequestMode_OpenToFriends );
	bool bAutoReject = ( eMode == k_ePartyJoinRequestMode_ClosedToFriends );
	CUtlVector< CSteamID > vecAutoAcceptJoinRequests;
	CUtlVector< CSteamID > vecAutoRejectJoinRequests;

	bool bIncomingJoinRequestsChanged = false;
	bool bOutgoingInvitesChanged = false;
	CUtlVector< CSteamID > vecNewIncomingJoinRequests;
	CUtlVector< CSteamID > vecNewOutgoingInvites;
	int nNewPendingPlayers = pActiveParty ? pActiveParty->GetNumPendingPlayers() : 0;
	for ( int i = 0; i < nNewPendingPlayers; i++ )
	{
		CSteamID steamID = pActiveParty->GetPendingPlayer( i );
		switch ( pActiveParty->GetPendingPlayerType( i ) )
		{
			case CTFParty::ePending_Invite:
			{
				if ( m_vecOutgoingInvites.Find( steamID ) == m_vecOutgoingInvites.InvalidIndex() )
					{ bOutgoingInvitesChanged = true; }
				vecNewOutgoingInvites.AddToTail( steamID );
			}
			break;
			case CTFParty::ePending_JoinRequest:
			{
				bool bHandled = false;
				// Are we going to auto-handle this?
				if ( BControllingPartyActions() )
				{
					if ( bAutoReject || !BAllowedToPartyWith( steamID ) )
					{
						vecAutoRejectJoinRequests.AddToTail( steamID );
						bHandled = true;
					}
					else if ( bAutoAccept )
					{
						vecAutoAcceptJoinRequests.AddToTail( steamID );
						bHandled = true;
					}
				}

				if ( !bHandled )
				{
					if ( m_vecIncomingJoinRequests.Find( steamID ) == m_vecIncomingJoinRequests.InvalidIndex() )
						{ bIncomingJoinRequestsChanged = true; }
					vecNewIncomingJoinRequests.AddToTail( steamID );
				}
			}
			break;
			default:
				Assert( !"Unhandled enum value" );
				break;
		}
	}

	// If we didn't find anyone new, then we only changed if there were more previously
	if ( !bOutgoingInvitesChanged && m_vecOutgoingInvites.Count() != vecNewOutgoingInvites.Count() )
		{ bOutgoingInvitesChanged = true; }
	if ( !bIncomingJoinRequestsChanged && m_vecIncomingJoinRequests.Count() != vecNewIncomingJoinRequests.Count() )
		{ bIncomingJoinRequestsChanged = true; }

	//
	// Change state
	//

	if ( bIsLeaderChanged )
	{
		m_bIsActivePartyLeader = bIsLeader;
	}

	if ( vecAddedMembers.Count() || vecRemovedMembers.Count() || vecMembersChangedOnlineState.Count() )
		{ m_activePartyMembers = vecNewMembers; }

	if ( bActiveCriteriaChanged )
		{ m_activePartyCriteria = newCriteria; }

	if ( bPartyChanged )
	{
		m_pActiveParty = pActiveParty;
		m_unActivePartyID = pActiveParty ? pActiveParty->GetGroupID() : 0u;
	}

	FOR_EACH_MAP_FAST( mapChangedQueues, idx )
		{ InternalSetInQueueForMatchGroup( mapChangedQueues.Key( idx ), mapChangedQueues[idx] ); }

	if ( bQueueStandbyChanged )
		{ m_bInStandbyQueue = bNowQueuedStandby; }

	if ( bOutgoingInvitesChanged )
		{ m_vecOutgoingInvites = vecNewOutgoingInvites; }

	if ( bIncomingJoinRequestsChanged )
		{ m_vecIncomingJoinRequests = vecNewIncomingJoinRequests; }

	//
	// Perform any auto-actions
	//   Do this after changing state because some of these calls report up our active party ID and such.
	//
	FOR_EACH_VEC( vecAutoAcceptJoinRequests, idx )
	{
		CSteamID steamID = vecAutoAcceptJoinRequests[idx];
		PartyMsg( "Auto-accepting join request from %s - autoaccept [%d], can party with [%d]\n",
		          steamID.Render(), bAutoAccept, BAllowedToPartyWith( steamID ) );
		// Bypass the explicit invite path and just auto-accept.  The normal path will be confused that it doesn't see
		// the expected join request, because we bypassed creating one.  We already checked that we're allowed to party
		// with this player above when considering if it was a valid auto-accept candidate.
		bool bInvited = BSendPartyInvitePlayerMessageInternal( steamID, true );
		if ( !bInvited )
			{ PartyWarn( "Failed to auto-invite player %s\n", steamID.Render() ); }
	}

	FOR_EACH_VEC( vecAutoRejectJoinRequests, idx )
	{
		CSteamID steamID = vecAutoRejectJoinRequests[idx];
		PartyMsg( "Auto-rejecting join request from %s - autoreject [%d], can party with [%d]\n",
		          steamID.Render(), bAutoReject, BAllowedToPartyWith( steamID ) );
		CancelOutgoingInviteOrIncomingJoinRequest( steamID );
	}

	//
	// Fire state changes
	//

	// OnNewParty
	if ( bPartyChanged )
		{ OnNewParty(); }

	// OnPartyMemberOnlineStateChanged
	FOR_EACH_VEC( vecMembersChangedOnlineState, idx )
		{ OnPartyMemberOnlineStateChanged( vecMembersChangedOnlineState[idx] ); }

	// OnPartyMemberGained
	FOR_EACH_VEC( vecAddedMembers, idx )
		{ OnPartyMemberGained( vecAddedMembers[idx] ); }

	// OnPartyMemberLost
	FOR_EACH_VEC( vecRemovedMembers, idx )
		{ OnPartyMemberLost( vecRemovedMembers[idx] ); }

	// OnIsLeaderChanged
	if ( bIsLeaderChanged )
		{ OnIsLeaderChanged( bIsLeader ); }

	// OnActiveCriteriaChanged
	//   if it changed above or if we switched the source of active criteria from above state changes
	bool bNowUsingActive = BEffectiveGroupCriteriaFromParty();
	if ( bActiveCriteriaChanged || ( bNowUsingActive != bWasUsingActiveCriteria ) )
		{ OnEffectiveCriteriaChanged(); }

	// OnQueueChanged
	FOR_EACH_MAP_FAST( mapChangedQueues, idx )
		{ OnInQueueChanged( mapChangedQueues.Key( idx ) ); }

	// OnStandbyQueueChanged
	if ( bQueueStandbyChanged )
		{ OnInStandbyQueueChanged(); }

	// OnOutgoingInvitesChanged
	if ( bOutgoingInvitesChanged )
		{ OnOutgoingInvitesChanged(); }

	// OnIncomingJoinRequestsChanged
	if ( bIncomingJoinRequestsChanged )
		{ OnIncomingJoinRequestsChanged(); }

	// TODO(Universal Parties): Check for unexpected queue state change, fire better coarse events
	OnPartyUpdated();

	// Trigger steam rich presence update -- many sub-changes might trigger these, so having the callback functions each
	// request one would fire it many times on e.g. joining another party.
	// Steam rich presence includes:
	// - queue state
	// - Party ID
	// - Num *online* party members
	if ( bPartyChanged || bQueueStandbyChanged || mapChangedQueues.Count() ||
	     vecAddedMembers.Count() || vecRemovedMembers.Count() || vecMembersChangedOnlineState.Count() )
	{
		GetClientModeTFNormal()->UpdateSteamRichPresence();
	}

	return bPartyChanged;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::CheckResetSentOptions()
{
	PartyDbg( "Resetting sent criteria" );
	// We still want to track how long we've been dirty I think, e.g. party leader changes and we were already dirty -
	// this is a change that dirties us like any other.
	//
	// m_flPendingChangesTime = -1.f;

	auto *pParty = GetActiveParty();
	if ( pParty && !BHaveAnyPendingQueueMsg() &&
	     ( !m_bPendingReliableCriteriaMsg || m_unActivePartyID != m_unPendingReliableCriteriaMsgParty ) )
	{
		// If we have a party and don't have pending messages that adjust options, we can safely guess at what our 'last
		// sent' values should be.
		m_lastSentUIState = ( this->BIsPartyLeader() ? pParty->Obj().leader_ui_state() : TFSyncedMMUIState() );
		m_lastSentGroupCriteria = pParty->GetGroupMatchCriteria();
		int idx = pParty->GetMemberIndexBySteamID( steamapicontext->SteamUser()->GetSteamID() );
		if ( idx != -1 )
			{ m_lastSentPerPlayerCriteria = pParty->GetMemberMatchCriteria( idx ); }
		else
			{ m_lastSentPerPlayerCriteria.Clear(); }
	}
	else
	{
		// If we don't have a party or pending messages are making this a sync-nightmare to calculate, just reset all
		// state and force a fresh send.  This should be the un-common case so the extra message load is worth the
		// complexity tradeoff.
		if ( m_unActivePartyID )
		{
			PartyWarn( "Re-joining party %llu we previously had queued messages for.  "
			           "Triggering full resend of options.\n", m_unActivePartyID );
		}
		// Set all last-sents to the default values.  Next CheckSendUpdates will set the overwrite flag due to
		// !m_bSentInitialCriteria, and then we'll diff against defaults for what to put in it.
		m_lastSentUIState.Clear();
		m_lastSentGroupCriteria.Clear();
		m_lastSentPerPlayerCriteria.Clear();
		// Sending an initial update will overwrite all settings, retroactively making our 'last sent was all defaults' the
		// truth.
		m_bSentInitialCriteria = false;

	}

	// Maybe not actually changed from default, but we want CheckSendUpdates to diff them and see.
	m_bGroupCriteriaChanged = true;
	m_bPerPlayerCriteriaChanged = true;
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnOutgoingInvitesChanged()
{
	PartyDbg( "Outgoing party invites updated [%d]\n", GetNumOutgoingInvites() );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnIncomingJoinRequestsChanged()
{
	PartyDbg( "Incoming party join requests updated [%d]\n", GetNumIncomingJoinRequests() );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnNewParty()
{
	PartyMsg( "Joining party %llu\n", GetActivePartyID() );
	CheckResetSentOptions();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnPartyMemberOnlineStateChanged( const Member_t &member )
{
	PartyMsg( "Member %s now %s\n", member.steamID.Render(), member.bOnline ? "online" : "offline" );
	ETFPartyChatType eChatType = ( member.bOnline
	                               ? k_eTFPartyChatType_Synthetic_MemberOnline
	                               : k_eTFPartyChatType_Synthetic_MemberOffline );

	PostChatGameEvent( member.steamID, eChatType, nullptr );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnPartyMemberGained( CSteamID steamID )
{
	PostChatGameEvent( steamID, k_eTFPartyChatType_Synthetic_MemberJoin, nullptr );

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "party_member_join" );
	if ( !pEvent )
		{ return; }
	pEvent->SetString( "steamid", CFmtStr("%llu", steamID.ConvertToUint64() ) );
	gameeventmanager->FireEventClientSide( pEvent );

	PartyDbg( "Gained member %s\n", steamID.Render() );

}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnPartyMemberLost( CSteamID steamID )
{
	PostChatGameEvent( steamID, k_eTFPartyChatType_Synthetic_MemberLeave, nullptr );

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "party_member_leave" );
	if ( !pEvent )
		{ return; }
	pEvent->SetString( "steamid", CFmtStr("%llu", steamID.ConvertToUint64() ) );
	gameeventmanager->FireEventClientSide( pEvent );

	PartyDbg( "Lost member %s\n", steamID.Render() );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnIsLeaderChanged( bool bIsLeader )
{
	auto nPID = GetActivePartyID();
	if ( nPID )
	{
		PartyMsg( "%s leader of party %llu\n", bIsLeader ? "Became" : "No longer", nPID );
	}

	AssertMsg( nPID || bIsLeader, "We should be considered a leader if we have no party object" );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnEffectiveCriteriaChanged()
{
	m_bQueuedEffectiveCriteriaChangeEvent = false;
	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_criteria_changed" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnInQueueChanged( ETFMatchGroup eMatchGroup )
{
	if ( BInQueueForMatchGroup( eMatchGroup ) )
		{ PartyMsg( "Entering queue for match group %s\n", GetMatchGroupName( eMatchGroup ) ); }
	else
		{ PartyMsg( "Leaving queue for match group %s\n", GetMatchGroupName( eMatchGroup ) ); }

	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_queue_state_changed" ) : nullptr;
	if ( event )
	{
		event->SetInt( "matchgroup", eMatchGroup );
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnInStandbyQueueChanged()
{
	if ( BInStandbyQueue() )
		{ PartyMsg( "Entering standby queue to join party\n" ); }
	else
		{ PartyMsg( "Leaving standby queue\n"); }

	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_queue_state_changed" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnIncomingInvitesChanged()
{
	PartyDbg( "Incoming invites updated [%d]\n", GetNumIncomingInvites() );

	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_invites_changed" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnOutgoingJoinRequestsChanged()
{
	PartyDbg( "Outgoing join requests updated [%d]\n", GetNumOutgoingJoinRequests() );

	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_invites_changed" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnPartyUpdated()
{
	// Fire event
	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_updated" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::OnPartyPrefChanged()
{
	// Fire event
	IGameEvent *event = gameeventmanager ? gameeventmanager->CreateEvent( "party_pref_changed" ) : nullptr;
	if ( event )
		{ gameeventmanager->FireEventClientSide( event ); }
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	if ( steamIDOwner == ClientSteamContext().GetLocalPlayerSteamID() )
	{
		// Assert( m_pSOCache == nullptr ); // we *can* get two SOCacheSubscribed calls in a row.
		m_pSOCache = GCClientSystem()->GetSOCache( steamIDOwner );
		Assert( m_pSOCache != nullptr );
	}
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	if ( steamIDOwner == ClientSteamContext().GetLocalPlayerSteamID() )
	{
		m_pSOCache = nullptr;
		UpdateIncomingInvitesOrOutgoingJoinRequests();
		UpdateActiveParty();
	}
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SOCreated( const CSteamID & steamIDOwner,
                                const GCSDK::CSharedObject *pObject,
                                GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFPartyInvite::k_nTypeID )
	{
		UpdateIncomingInvitesOrOutgoingJoinRequests();
		return;
	}
	if ( pObject->GetTypeID() != CTFParty::k_nTypeID )
		{ return; }

	PartyDbg( "Party object received for %llu, current party %llu\n",
	          ToParty( pObject )->GetGroupID(), GetActivePartyID() );

	// We can get double-creates that overwrite items with SOCache magic, ensure we actually changed
	bool bNewParty = UpdateActiveParty();
	NoteUnused( bNewParty );
	AssertMsg( !bNewParty || m_pActiveParty, "Our party changed from party -> noparty in SOCreate?" );
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SOUpdated( const CSteamID & steamIDOwner,
                                const GCSDK::CSharedObject *pObject,
                                GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFPartyInvite::k_nTypeID )
	{
		UpdateIncomingInvitesOrOutgoingJoinRequests();
		return;
	}
	if ( pObject->GetTypeID() != CTFParty::k_nTypeID )
		{ return; }

	auto *pParty = ToParty( pObject );
	auto *pActiveParty = GetActiveParty();
	// We can get SOUpdated for objects that were never SOCreated in the case of cache subscribe, apparently
	if ( pActiveParty && pParty != pActiveParty )
	{
		PartyWarn( "Party update received for party %llu, but our current party is %llu, forcing update\n",
		           pParty->GetGroupID(), GetActivePartyID() );
		Assert( !"SOUpdated for a party we don't think we are in" );
	}

	UpdateActiveParty();
}

//-----------------------------------------------------------------------------
void CTFPartyClient::SODestroyed( const CSteamID & steamIDOwner,
                                  const GCSDK::CSharedObject *pObject,
                                  GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFPartyInvite::k_nTypeID )
	{
		UpdateIncomingInvitesOrOutgoingJoinRequests();
		return;
	}
	if ( pObject->GetTypeID() != CTFParty::k_nTypeID )
		{ return; }

	auto nPID = ToParty( pObject )->GetGroupID();
	auto nActivePID = GetActivePartyID();
	if ( nPID != nActivePID )
	{
		PartyWarn( "SODestroyed for party %llu, but we believe our active party is %llu\n", nPID, nActivePID );
	}

	bool bChanged = UpdateActiveParty();
	AssertMsg( bChanged, "Received SODestroyed for a party, but we don't think it affected our active party" );
	NoteUnused( bChanged );
}

//
// Inbound message jobs
//

//-----------------------------------------------------------------------------
class CGCClientJobPartyChatMsg : public GCSDK::CGCClientJob
{
public:
	CGCClientJobPartyChatMsg( GCSDK::CGCClient *pGCServer ) : GCSDK::CGCClientJob( pGCServer ) { }

	virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket )
	{
		CProtoBufMsg<CMsgPartyChatMsg> msg( pNetPacket );
		GTFPartyClient()->ReceivedPartyChatMsg( msg.Body() );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCClientJobPartyChatMsg, "CGCClientJobPartyChatMsg", k_EMsgGCParty_ChatMsg, k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
class CGCClientJobPartyMMError : public GCSDK::CGCClientJob
{
public:
	CGCClientJobPartyMMError( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgPartyMMError> msg( pNetPacket );
		CMsgPartyMMError_Type eType = msg.Body().type();

		PartyWarn( "Received MM Error %s\n", ( CMsgPartyMMError_Type_IsValid( eType )
		                                       ? CMsgPartyMMError_Type_Name( eType ).c_str()
		                                       : CFmtStr( "<unknown (%d)>", eType ).Get() ) );

		switch ( eType )
		{
			case CMsgPartyMMError_Type_QUEUE_KICK_NO_PING:
				TFMMIssueDialog()->Show( "#TF_MM_Queue_Error_Ping_Text", "#TF_MM_Queue_Error_Ping_URL" );
				break;
			case CMsgPartyMMError_Type_QUEUE_KICK_AUTH:
				TFMMIssueDialog()->Show( "#TF_MM_Queue_Error_VACIssue_Text", "#TF_MM_Queue_Error_VACIssue_URL" );
				break;
			default:
			{
				AssertMsg( false, "Unhandled CMsgPartyMMError Type" );
				break;
			}
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCClientJobPartyMMError, "CGCClientJobPartyMMError", k_EMsgGCParty_MMError, k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_invite_user, "Invite the given steamid to join your party, or accept a request to do so" )
{
	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage: tf_party_invite_user <steamid>\n" );
		return;
	}

	CSteamID steamID = UTIL_GuessSteamIDFromFuzzyInput( args[1] );
	if ( !steamID.IsValid() || steamID.GetEAccountType() != k_EAccountTypeIndividual )
	{
		ConWarning( "Could not parse an individual Steam ID from '%s'\n", args[1] );
		return;
	}

	if ( !GTFPartyClient()->BCanInvitePlayer( steamID ) )
	{
		ConWarning( "You cannot currently invite player %s to this party\n", steamID.Render() );
		return;
	}

	if ( !GTFPartyClient()->BInvitePlayerToParty( steamID, false ) )
	{
		ConWarning( "Failed to invite %s to your party\n", steamID.Render() );
		return;
	}

	ConMsg( "Invited %s to your party\n", steamID.Render() );
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_request_join_user, "Request to join the given steamid's party, or accept an invite to do so" )
{
	if ( args.ArgC() != 2 && args.ArgC() != 3 )
	{
		ConMsg( "Usage: tf_party_request_join_user <steamid> [<expect invite (1/0)>]\n" );
		return;
	}

	bool bExpectInvite = ( args.ArgC() > 2 && V_atoi( args[2] ) );

	CSteamID steamID = UTIL_GuessSteamIDFromFuzzyInput( args[1] );
	if ( !steamID.IsValid() || steamID.GetEAccountType() != k_EAccountTypeIndividual )
	{
		ConWarning( "Could not parse an individual Steam ID from '%s'\n", args[1] );
		return;
	}

	if ( !GTFPartyClient()->BCanRequestToJoinPlayer( steamID ) )
	{
		ConWarning( "You cannot currently request to join party of player %s\n", steamID.Render() );
		return;
	}

	if ( !GTFPartyClient()->BRequestJoinPlayer( steamID, bExpectInvite ) )
	{
		ConWarning( "Failed to request to join party of player %s\n", steamID.Render() );
		return;
	}

	ConMsg( "Requested to join party of player %s\n", steamID.Render() );
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_chat, "Talk to me" )
{
	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage: tf_party_chat <message>\n" );
		return;
	}

	const char *pszMsg = args.Arg( 1 );
	GTFPartyClient()->SendPartyChat( pszMsg );
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_leave, "Leave current party" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_party_leave - no args\n" );
		return;
	}

	GTFPartyClient()->LeaveActiveParty();
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_incoming_invites_debug, "Show current party invite objects" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_party_incoming_invites_debug - no args\n" );
		return;
	}

	int nCount = GTFPartyClient()->GetNumIncomingInvites();
	PartyMsg( "%d incoming invites%s\n", nCount, nCount ? ":" : "" );
	for ( int i = 0; i < nCount; i++ )
	{
		PartyMsg( "%s\n", GTFPartyClient()->GetIncomingInvite( i )->Obj().DebugString().c_str() );
	}
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_force_update, "Force a party invite now" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_party_force_update - no args\n" );
		return;
	}

	GTFPartyClient()->ForcePartyUpdate();
	PartyMsg( "Forced party update\n" );
}


// These commands could screw with UI flow or be abused maybe
#if defined( STAGING_ONLY ) || defined( _DEBUG )
//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_queue, "Queue the party" )
{
	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage: tf_party_queue - <matchgroup id or enum name>\n" );
		return;
	}

	ETFMatchGroup eMatchGroup = ETFMatchGroup_FuzzyParse( args[1] );
	if ( !AssertValidMatchGroup( eMatchGroup ) )
	{
		ConMsg( "Failed to parse a valid match group from input\n" );
		return;
	}

	GTFPartyClient()->RequestQueueForMatch( eMatchGroup );
	if ( GTFPartyClient()->BInQueueForMatchGroup( eMatchGroup ) )
		{ ConMsg( "Requested MM queue for [%s]\n", GetMatchGroupName( eMatchGroup ) ); }
	else
		{ ConMsg( "Queue request failed for [%s]\n", GetMatchGroupName( eMatchGroup ) ); }
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_queue_standby, "Queue for standby for your party" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_party_queue_standby - no args\n" );
		return;
	}

	GTFPartyClient()->RequestQueueForStandby();
	if ( GTFPartyClient()->BInStandbyQueue() )
		{ ConMsg("Requested queue for standby\n"); }
	else
		{ ConMsg("Queue request failed\n"); }
}

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_queue_cancel, "Un-queue the party" )
{
	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage: tf_party_queue_cancel - <matchgroup id or enum name>\n" );
		return;
	}

	ETFMatchGroup eMatchGroup = ETFMatchGroup_FuzzyParse( args[1] );
	if ( !AssertValidMatchGroup( eMatchGroup ) )
	{
		ConMsg( "Failed to parse a valid match group from input\n" );
		return;
	}

	if ( GTFPartyClient()->BInQueueForMatchGroup( eMatchGroup ) )
	{
		GTFPartyClient()->CancelMatchQueueRequest( eMatchGroup );
		ConMsg("Requested cancel of queue request for [%s]\n", GetMatchGroupName( eMatchGroup ) );
	}
	else
	{
		ConMsg("Not in queue for [%s], nothing to cancel\n", GetMatchGroupName( eMatchGroup ) );
	}
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CON_COMMAND( tf_party_queue_standby_cancel, "Un-queue from standby for your party" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_party_queue_standby_cancel - no args\n" );
		return;
	}

	if (GTFPartyClient()->BInStandbyQueue())
	{
		GTFPartyClient()->CancelStandbyQueueRequest();
		ConMsg("Requested cancel of standby queue request\n");
	}
	else
	{
		ConMsg("Not in standby queue\n");
	}
}
#endif // defined( STAGING_ONLY ) || defined( _DEBUG )
