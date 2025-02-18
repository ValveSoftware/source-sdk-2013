//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_lobby_shared.h"

using namespace GCSDK;

///
/// Implements the base accessors shared between CTFLobby/CTFGSLobby
///


//-----------------------------------------------------------------------------
int CTFLobbyShared::GetMemberIndexBySteamID( const CSteamID &steamID ) const
{
	for ( int i = 0; i < GSObj().members_size(); i++ )
	{
		if ( GSObj().members( i ).id() == steamID.ConvertToUint64() )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
ConstTFLobbyPlayer CTFLobbyShared::GetMemberDetails( int i ) const
{
	const CTFLobbyPlayerProto *pObj = nullptr;
	if ( BAssertValidMemberIndex( i ) )
		{ pObj = &GSObj().members( i ); }
	return ConstTFLobbyPlayer( *pObj );
}

//-----------------------------------------------------------------------------
const CSteamID CTFLobbyShared::GetMember( int i ) const
{
	Assert( i >= 0 && i < GSObj().members_size() );
	if ( i < 0 || i >= GSObj().members_size() )
		return k_steamIDNil;

	return GSObj().members( i ).id();
}

//-----------------------------------------------------------------------------
bool CTFLobbyShared::BAssertValidMemberIndex( int iMemberIndex ) const
{
	bool bValidMemberIndex = iMemberIndex >= 0 && iMemberIndex < GSObj().members_size();
	Assert( bValidMemberIndex );
	return bValidMemberIndex;
}

//-----------------------------------------------------------------------------
// Match players iterator
//-----------------------------------------------------------------------------
CTFLobbyShared::MatchPlayers_t CTFLobbyShared::GatherMatchPlayers() const
{
	int nMatchMembers = 0;
	int nMatchPlayers = InternalCountPlayers( true, true, &nMatchMembers );
	return MatchPlayers_t( *this, nMatchMembers, nMatchPlayers );
}

//-----------------------------------------------------------------------------
CTFLobbyShared::MatchPlayers_t::index CTFLobbyShared::MatchPlayers_t::GetIndexBySteamID( CSteamID steamID ) const
{
	int idxMatchPlayer = 0;
	for ( int i = 0; i < m_lobby.GSObj().members_size(); i++ )
	{
		if ( !m_lobby.GetMemberDetails( i ).BMatchPlayer() )
			{ continue; }
		if ( m_lobby.GetMember( i ) == steamID )
			{ return MatchPlayers_t::index{ idxMatchPlayer }; }
		// Effective idxMatchPlayer by all-match-players
		++idxMatchPlayer;
	}

	for ( int idxPending = 0; idxPending < m_lobby.GetNumPendingPlayers(); idxPending++ )
	{
		if ( !m_lobby.GetPendingPlayerDetails( idxPending ).BMatchPlayer() )
			{ continue; }
		if ( m_lobby.GetPendingPlayer( idxPending ) == steamID )
			{ return MatchPlayers_t::index{ idxMatchPlayer }; }
		// Effective idxMatchPlayer by all-match-players
		++idxMatchPlayer;
	}

	// Will be == end() if not found
	return MatchPlayers_t::index{ idxMatchPlayer };
}

//-----------------------------------------------------------------------------
ConstTFLobbyPlayer CTFLobbyShared::MatchPlayers_t::GetDetails( MatchPlayers_t::index idxMatchPlayer ) const
{
	AssertFatal( idxMatchPlayer.value >= 0 );

	if ( BIsMember( idxMatchPlayer ) )
	{
		// nth match-player member
		int idxFound = 0;
		for ( int i = 0; i < m_lobby.GSObj().members_size(); i++ )
		{
			auto details = m_lobby.GetMemberDetails( i );
			if ( !details.BMatchPlayer() )
				{ continue; }
			if ( idxFound == idxMatchPlayer.value )
				{ return details; }
			// Effective match-player-index by all-match-players
			++idxFound;
		}
	}
	else
	{
		// nth pending match player after members
		int idxFound = m_nMemberCount;
		for ( int idxPending = 0; idxPending < m_lobby.GetNumPendingPlayers(); idxPending++ )
		{
			auto details = m_lobby.GetPendingPlayerDetails( idxPending );
			if ( !details.BMatchPlayer() )
				{ continue; }
			if ( idxFound == idxMatchPlayer.value )
				{ return details; }
			// Effective match-player-index by all-match-players
			++idxFound;
		}
	}

	AssertFatal( false );
	return m_lobby.GetMemberDetails( 0 );
}

//-----------------------------------------------------------------------------
int CTFLobbyShared::CountCurrentMatchPlayers() const
{
	return InternalCountPlayers( /* bOnlyMatchPlayers */ true, /* bIncludePending */ false );
}

//-----------------------------------------------------------------------------
int CTFLobbyShared::CountCurrentAndPendingMatchPlayers() const
{
	return InternalCountPlayers( /* bOnlyMatchPlayers */ true, /* bIncludePending */ true );
}

//-----------------------------------------------------------------------------
int CTFLobbyShared::InternalCountPlayers( bool bOnlyMatchPlayers, bool bIncludePending,
                                          int *pOutNumMember /* = nullptr */ ) const
{
	// All match player iteration uses this un-cached. If the compiler isn't being good about inlining it could get
	// ugly.  (But adding VPROF edges is almost certainly going to make inlining worse)
	//
	// If it becomes a problem, we'll need to make this a cached value and catch all the edges.

	int ret = 0;

	if ( bOnlyMatchPlayers )
	{
		for ( int i = 0; i < GSObj().members_size(); i++ )
		{
			ret += (int)ConstTFLobbyPlayer( GSObj().members( i ) ).BMatchPlayer();
		}
	}
	else
	{
		ret += GSObj().members_size();
	}

	// For counting member & pending
	if ( pOutNumMember )
		{ *pOutNumMember = ret; }

	if ( bIncludePending )
	{
		if ( bOnlyMatchPlayers )
		{
			for ( int i = 0; i < GSObj().pending_members_size(); i++ )
			{
				ret += (int)ConstTFLobbyPlayer( GSObj().pending_members( i ) ).BMatchPlayer();
			}
		}
		else
		{
			ret += GSObj().pending_members_size();
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
int CTFLobbyShared::GetNumPendingPlayers() const
{
	return GSObj().pending_members_size();
}

//-----------------------------------------------------------------------------
const CSteamID CTFLobbyShared::GetPendingPlayer( int i ) const
{
	if ( !BAssertValidPendingPlayerIndex( i ) )
		return k_steamIDNil;

	return GSObj().pending_members( i ).id();
}

//-----------------------------------------------------------------------------
CTFLobbyShared::EPendingType CTFLobbyShared::GetPendingPlayerType( int i ) const
{
	NoteUnused(i);
	// We don't support join requests to lobbies
	return ePending_Invite;
}

//-----------------------------------------------------------------------------
int CTFLobbyShared::GetPendingPlayerIndexBySteamID( const CSteamID &steamID ) const
{
	for ( int i = 0; i < GSObj().pending_members_size(); i++ )
	{
		if ( steamID == GSObj().pending_members( i ).id() )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
ConstTFLobbyPlayer CTFLobbyShared::GetPendingPlayerDetails( int i ) const
{
	const CTFLobbyPlayerProto *pObj = nullptr;
	if ( BAssertValidPendingPlayerIndex( i ) )
		{ pObj = &GSObj().pending_members( i ); }
	return ConstTFLobbyPlayer( *pObj );
}

//-----------------------------------------------------------------------------
bool CTFLobbyShared::BAssertValidPendingPlayerIndex( int iPendingPlayerIndex ) const
{
	bool bValidPendingPlayerIndex = iPendingPlayerIndex >= 0 && iPendingPlayerIndex < GSObj().pending_members_size();
	Assert( bValidPendingPlayerIndex );
	return bValidPendingPlayerIndex;
}

//-----------------------------------------------------------------------------
void CTFLobbyShared::SpewDebugSummary() const
{
	Msg( "CTFLobbyShared: ID:%016llx  %d member(s), %d pending\n",
	     GetGroupID(), GetNumMembers(), GetNumPendingPlayers() );
	for ( int i = 0; i < GetNumMembers(); i++ )
	{
		ConstTFLobbyPlayer details = GetMemberDetails( i );
		Msg( "  Member[%d] %s  team = %s  type = %s\n",
		     i, GetMember( i ).Render(), TF_GC_TEAM_Name( details.GetTeam() ).c_str(),
		     CTFLobbyPlayerProto_Type_Name( details.Proto().type() ).c_str() );
	}
	for ( int i = 0; i < GetNumPendingPlayers(); i++ )
	{
		ConstTFLobbyPlayer details = GetPendingPlayerDetails( i );
		Msg( "  Pending[%d] %s  team = %s  type = %s\n",
		     i, GetPendingPlayer( i ).Render(), TF_GC_TEAM_Name( details.GetTeam() ).c_str(),
		     CTFLobbyPlayerProto_Type_Name( details.Proto().type() ).c_str() );
	}
}

#ifdef USE_MVM_TOUR
//-----------------------------------------------------------------------------
const char *CTFLobbyShared::GetMannUpTourName() const
{
	if ( !IsMannUpGroup( GetMatchGroup() ) )
		return NULL;
	return GSObj().mannup_tour_name().c_str();
}
#endif // USE_MVM_TOUR

//-----------------------------------------------------------------------------
