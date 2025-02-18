//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_party.h"
#include "rtime.h"
#include "tf_quickplay_shared.h"
#include "tf_match_description.h"


using namespace GCSDK;

//---------------------------------------------------------------------------------



//---------------------------------------------------------------------------------
CTFParty::CTFParty()
{
}

CTFParty::~CTFParty()
{
}


const CSteamID CTFParty::GetLeader() const
{
	CSteamID steamID( Obj().leader_id() );
	// Player groups gain a leader after the first person completes their join process, but this adds odd edge-cases
	// everywhere -- parties are supposed to always have a leader, at all points.  As an ugly but fairly solid hack --
	// treat parties of one with no leader set as belonging to that user until otherwise specified.  But assert that
	// this condition never exists for multi-member parties.
	Assert( steamID.IsValid() || GetNumMembers() == 1 );
	if ( !steamID.IsValid() && GetNumMembers() == 1 )
		{ steamID = GetMember( 0 ); }

	Assert( steamID.IsValid() );
	return steamID;
}

const CSteamID CTFParty::GetMember( int i ) const
{
	Assert( i >= 0 && i < Obj().member_ids_size() );
	if ( i < 0 || i >= Obj().member_ids_size() )
		return k_steamIDNil;

	return Obj().member_ids( i );
}

int CTFParty::GetMemberIndexBySteamID( const CSteamID &steamID ) const
{
	for ( int i = 0; i < Obj().member_ids_size(); i++ )
	{
		if ( Obj().member_ids( i ) == steamID.ConvertToUint64() )
			return i;
	}
	return -1;
}

#ifdef CLIENT_DLL
int CTFParty::GetClientCentricMemberIndexBySteamID( const CSteamID &steamID ) const
{
	CSteamID steamIDLocalPlayer = k_steamIDNil;

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		steamIDLocalPlayer = steamapicontext->SteamUser()->GetSteamID();
	}

	// The local player always appears to be in slot 0
	if ( steamID == steamIDLocalPlayer )
	{
		return 0;
	}

	int nOtherPlayerSlot = GetMemberIndexBySteamID( steamID );

	// Whoever actually is in slot 0 appears to be in the local player's slot
	if ( nOtherPlayerSlot == 0 )
	{
		return GetMemberIndexBySteamID( steamIDLocalPlayer );
	}

	// If they're not the local player, and not in slot 0, just return
	// their actual slot.
	return nOtherPlayerSlot;
}

CSteamID CTFParty::GetClientCentricMemberSteamIDByIndex( int nIndex ) const
{
	CSteamID steamIDLocalPlayer = k_steamIDNil;

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		steamIDLocalPlayer = steamapicontext->SteamUser()->GetSteamID();
	}

	// Local player appears in slot 0
	if ( nIndex == 0 )
	{
		return steamIDLocalPlayer;
	}

	int nLocalPlayerSlot = GetMemberIndexBySteamID( steamIDLocalPlayer );

	// Whoever actually is in slot 0 appears to be in the local player's slot
	if ( nIndex == nLocalPlayerSlot )
	{
		return GetMember( 0 );
	}

	// If they're not the local player, and not in slot 0, just return
	// whoever is in the slot
	return GetMember( nIndex );
}
#endif

const CSOTFPartyMember_Activity& CTFParty::GetMemberActivity( int i ) const
{
	Assert( i >= 0 && i < Obj().members_size() );
	if ( i < 0 || i >= Obj().members_size() )
		return CSOTFPartyMember_Activity::default_instance();

	return Obj().members( i ).activity();
}

bool CTFParty::BMemberRequestingLobbyStandby( int i ) const
{
	Assert( i >= 0 && i < Obj().members_size() );
	if ( i < 0 || i >= Obj().members_size() )
		return false;

	return Obj().members( i ).lobby_standby();
}


const ConstRefTFPerPlayerMatchCriteria CTFParty::GetMemberMatchCriteria( int i ) const
{
	Assert( i >= 0 && i < Obj().members_size() );
	if ( i < 0 || i >= Obj().members_size() )
		{ return ConstRefTFPerPlayerMatchCriteria( CTFPerPlayerMatchCriteriaProto::default_instance() ); }

	CTFPerPlayerMatchCriteriaProto::default_instance();
	return ConstRefTFPerPlayerMatchCriteria( Obj().members( i ).player_criteria() );
}

RefTFPerPlayerMatchCriteria CTFParty::MutMemberMatchCriteria( int i )
{
	AssertFatal( i >= 0 && i < Obj().members_size() );

	return RefTFPerPlayerMatchCriteria( *Obj().mutable_members( i )->mutable_player_criteria() );
}

const CSteamID CTFParty::GetPendingPlayer( int i ) const
{
	Assert( i >= 0 && i < Obj().pending_members_size() );
	if ( i < 0 || i >= Obj().pending_members_size() )
		return k_steamIDNil;

	return Obj().pending_members( i ).steamid();
}

CTFParty::EPendingType CTFParty::GetPendingPlayerType( int i ) const
{
	Assert( i >= 0 && i < Obj().pending_members_size() );
	if ( i < 0 || i >= Obj().pending_members_size() )
		return ePending_Invite;

	switch ( Obj().pending_members( i ).type() )
	{
		case TFPendingPartyMember_EType_Invited:
			return ePending_Invite;
		case TFPendingPartyMember_EType_RequestedToJoin:
			return ePending_JoinRequest;
	}

	Assert( !"Unhandled enum value" );
	return ePending_Invite;
}

CSteamID CTFParty::GetPendingPlayerInviter( int i ) const
{
	Assert( i >= 0 && i < Obj().pending_members_size() );
	if ( i < 0 || i >= Obj().pending_members_size() )
		return CSteamID();

	return CSteamID( Obj().pending_members( i ).inviter() );
}

int CTFParty::GetPendingPlayerIndexBySteamID( const CSteamID &steamID ) const
{
	for ( int i = 0; i < Obj().pending_members_size(); i++ )
	{
		if ( Obj().pending_members( i ).steamid() == steamID.ConvertToUint64() )
			return i;
	}
	return -1;
}

int CTFParty::GetQueueEntryIdxByMatchGroup( ETFMatchGroup eMatchGroup ) const
{
	int nEntries = GetNumQueueEntries();
	for ( int idx = 0; idx < nEntries; idx++ )
	{
		if ( GetQueueEntryMatchGroup( idx ) == eMatchGroup )
			{ return idx; }
	}
	return -1;
}


void CTFParty::SpewDebug()
{
	CRTime now( CRTime::RTime32TimeCur() );
	char time_buf[k_RTimeRenderBufferSize];
	char now_buf[k_RTimeRenderBufferSize];

	Msg( "TFParty: ID:%016llx  %d member(s)  LeaderID: %s\n", GetGroupID(), GetNumMembers(), GetLeader().Render() );
	Msg( "  In %d queues:\n", GetNumQueueEntries() );
	for ( int idx = 0; idx < GetNumQueueEntries(); idx++ )
	{
		RTime32 rtQueued = GetQueueEntryStartTime( idx );
		CRTime time( rtQueued );
		Msg( "    MatchGroup: %d  Started matchmaking: %s (%d seconds ago, now is %s)\n",
		     GetQueueEntryMatchGroup( idx ), time.Render( time_buf ), CRTime::RTime32TimeCur() - rtQueued,
		     now.Render( now_buf ) );
	}
	Msg( "------\n" );
	Msg( "%s\n", Obj().DebugString().c_str() );
	Dump();
}


bool CTFParty::BAnyMemberWithoutTicket() const
{
	Assert( Obj().members_size() == GetNumMembers() );
	for ( int i = 0 ; i < Obj().members_size() ; ++i )
	{
		if ( !Obj().members(i).owns_ticket() )
			return true;
	}

	return false;
}

bool CTFParty::BAnyMemberWithoutCompetitiveAccess() const
{
	Assert( Obj().members_size() == GetNumMembers() );
	for ( int i = 0; i < Obj().members_size(); ++i )
	{
		if ( !Obj().members( i ).competitive_access() )
			return true;
	}

	return false;
}

bool CTFParty::BAnyMemberWithLowPriority( EMMPenaltyPool eType ) const
{
	// Wretched hive of scum and villainy
	switch ( eType )
	{
	case eMMPenaltyPool_Casual: 
		return Obj().casual_low_priority_time() > CRTime::RTime32TimeCur();
	case eMMPenaltyPool_Ranked: 
		return Obj().ranked_low_priority_time() > CRTime::RTime32TimeCur();
	}

	Assert( false );
	return false;
}

bool CTFParty::BAnyMembersBanned( EMMPenaltyPool eType ) const
{
	// tricksters and miscreants
	switch ( eType )
	{
	case eMMPenaltyPool_Casual: 
		return Obj().casual_banned_time() > CRTime::RTime32TimeCur();
	case eMMPenaltyPool_Ranked: 
		return Obj().ranked_banned_time() > CRTime::RTime32TimeCur();
	}

	Assert( false );
	return false;
}

bool CTFParty::BMembersIsBanned( const CSteamID& steamID, EMMPenaltyPool eType ) const
{
	int idx = GetMemberIndexBySteamID( steamID );
	Assert( idx != -1 );
	if ( idx == -1 )
		return false;

	switch ( eType )
	{
	case eMMPenaltyPool_Casual: 
		return Obj().members( idx ).casual_banned();
	case eMMPenaltyPool_Ranked: 
		return Obj().members( idx ).ranked_banned();
	}

	Assert( false );
	return false;
}

bool CTFParty::BMemberWithoutCompetitiveAccess( const CSteamID &steamID ) const 
{
	int idx = GetMemberIndexBySteamID( steamID );
	Assert( idx != -1 );
	if ( idx == -1 )
		return false;

	return Obj().members( idx ).competitive_access() == false;
}

bool CTFParty::BMemberWithoutTourOfDutyTicket( const CSteamID& steamID ) const
{
	int idx = GetMemberIndexBySteamID( steamID );
	Assert( idx != -1 );
	if ( idx == -1 )
		return false;

	return Obj().members( idx ).owns_ticket() == false;
}

const CSteamID CTFPartyInvite::GetMember( int i ) const
{
	Assert( i >= 0 && i < Obj().members_size() );
	if ( i < 0 || i >= Obj().members_size() )
		return k_steamIDNil;

	return Obj().members( i ).steamid();
}

CTFParty::EPendingType CTFPartyInvite::GetType() const
{
	switch ( Obj().type() )
	{
	case CSOTFPartyInvite::PENDING_INVITE:
		return CTFParty::ePending_Invite;
	case CSOTFPartyInvite::PENDING_JOIN_REQUEST:
		return CTFParty::ePending_JoinRequest;
	}

	DbgAssert( !"Unhandled enum value" );
	return CTFParty::ePending_Invite;
}

