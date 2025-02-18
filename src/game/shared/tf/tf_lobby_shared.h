//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The lobby shared object for gameservers, managed by CTFLobby
//
//=============================================================================

#ifndef TF_LOBBY_SHARED_H
#define TF_LOBBY_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#include "tf_matchmaking_shared.h"
#include "playergroup.h"
#include "tier1/utlrange.h"
#include "lobby.h"
#include "steam/steamclientpublic.h"

class ConstTFLobbyPlayer;
// On the GC, this is the base type that access's the parent's shared object. Elsewhere, it is the SO.
class CTFLobbyShared : public GCSDK::ILobby
{
public:
	virtual ~CTFLobbyShared() {}

	//
	// IPlayerGroup
	//
	// We don't store this in the object, not meaningful for lobbies
	virtual const CSteamID GetLeader() const OVERRIDE { return k_steamIDNil; }

	// Members
	virtual int GetNumMembers() const OVERRIDE { return GSObj().members_size(); }
	virtual const CSteamID GetMember( int i ) const OVERRIDE;
	virtual int GetMemberIndexBySteamID( const CSteamID &steamID ) const OVERRIDE;
	virtual ConstTFLobbyPlayer GetMemberDetails( int i ) const;
	bool BAssertValidMemberIndex( int iMemberIndex ) const;

	// A pretty-printed summary, superclases should also provide Dump(), which can use this
	virtual void SpewDebugSummary() const;

	// Enumerate all match players associated with this lobby.
	// This includes all members *and* pending members who match details.BMatchPlayer()
	class MatchPlayers_t : public CUtlIndexRange< int, MatchPlayers_t >
	{
		friend class CTFLobbyShared;
	public:
		MatchPlayers_t::index GetIndexBySteamID( CSteamID steamID ) const;
		ConstTFLobbyPlayer GetDetails( MatchPlayers_t::index idxMatchPlayer ) const;
		bool BIsMember( MatchPlayers_t::index idx ) const { return idx.value < m_nMemberCount; }
		bool BIsPending( MatchPlayers_t::index idx ) const { return !BIsMember(idx); }
		int Count() const { return end().value; }

	private:
		MatchPlayers_t( const CTFLobbyShared &lobby, ValueType_t nMemberCount, ValueType_t nCount )
			: CUtlIndexRange( 0, nCount ), m_nMemberCount( nMemberCount ), m_lobby( lobby ) {}
		ValueType_t m_nMemberCount;
		const CTFLobbyShared &m_lobby;
	};
	// Use MatchPlayers.begin() / MatchPlayers.end() or for ( auto idx : MatchPlayers() )
	// (Preventing inevitable mixups between Member and MatchPlayer indicies)
	MatchPlayers_t GatherMatchPlayers() const;

	// TODO(Multiqueue): Make these private or delete them in favor of Gather. AndPending is misleading if MatchPlayers always includes pending

	// Tally how many match players we have in our members list, not including pending invites.
	int CountCurrentMatchPlayers() const;
	// Tally how many match players we have in our members list, including pending invites.
	int CountCurrentAndPendingMatchPlayers() const;

	// Inline helpers
	CSOTFGameServerLobby::State GetState() const { return GSObj().state(); }
	const char *GetMissionName() const { return GSObj().mission_name().c_str(); }
	ETFMatchGroup GetMatchGroup() const { return GSObj().has_match_group() ? (ETFMatchGroup)GSObj().match_group() : k_eTFMatchGroup_Invalid; }
	uint64 GetMatchID( void ) const { return GSObj().match_id(); }
	uint32 GetFlags( void ) const { return GSObj().flags(); }
	const char *GetMapName() const { return GSObj().map_name().c_str(); }
	virtual GCSDK::PlayerGroupID_t GetGroupID() const OVERRIDE { return GSObj().lobby_id(); }
	bool GetLateJoinEligible() const { return GSObj().late_join_eligible(); }
	CSteamID GetServerID() const { return GSObj().server_id(); }
	bool HasConnect() const { return GSObj().has_connect(); }
	const char *GetConnect() const { return GSObj().connect().c_str(); }
	uint32_t GetLobbyMMVersion() const { return GSObj().lobby_mm_version(); }

#ifdef USE_MVM_TOUR
	// Returns name of tour that we are playing for.  Returns NULL if we are not playing for bragging rights!
	bool BHasMannUpTourName() const { return GSObj().has_mannup_tour_name(); }
	const char *GetMannUpTourName() const;
#endif // USE_MVM_TOUR

	// invites
	virtual int GetNumPendingPlayers() const OVERRIDE;
	// Not meaningful for lobbies
	virtual CSteamID GetPendingPlayerInviter( int ) const OVERRIDE { return k_steamIDNil; }
	virtual const CSteamID GetPendingPlayer( int i ) const OVERRIDE;
	virtual EPendingType GetPendingPlayerType( int i ) const OVERRIDE;
	virtual int GetPendingPlayerIndexBySteamID( const CSteamID &steamID ) const OVERRIDE;

	bool BAssertValidPendingPlayerIndex( int iPendingPlayerIndex ) const;
	ConstTFLobbyPlayer GetPendingPlayerDetails( int i ) const;

private:
	// Ugly version of the Count*Players calls
	int InternalCountPlayers( bool bOnlyMatchPlayers, bool bIncludePending, int *pOutNumMember = nullptr ) const;

	// Implementor points us to these
	virtual const CSOTFGameServerLobby &GSObj() const = 0;
	virtual CSOTFGameServerLobby &GSObj() = 0;
};

// Provides non-mutable TFLobbyPlayer functions
class ITFLobbyPlayer
{
public:
	const char *                     GetName() const { return Proto().name().c_str(); }
	CSteamID                         GetSteamID() const { return CSteamID( Proto().id() ); }
	RTime32                          GetLastConnectTime() const { return RTime32( Proto().last_connect_time() ); }
	TF_GC_TEAM                       GetTeam() const { return Proto().team(); }
	bool                             GetSquadSurplus() const { return Proto().squad_surplus(); }
	uint32_t                         GetBadgeLevel() const { return Proto().badge_level(); }
	CTFLobbyPlayerProto_ConnectState GetConnectState() const { return Proto().connect_state(); };
	double                           GetNormalizedRating() const { return Proto().normalized_rating(); };
	int                              GetRank() const { return Proto().rank(); }
	bool                             GetChatSuspension() const { return Proto().chat_suspension(); }

	bool BMatchPlayer() const { return Proto().type() == CTFLobbyPlayerProto_Type_MATCH_PLAYER; }

	// Purposefully hidden from GC to prevent mis-use -- this field is not synchronously updated, and should only be
	// looked at by server/clients.  GC code should always consult partymanager.
	GCSDK::PlayerGroupID_t           GetOriginalPartyID() const { return Proto().original_party_id(); };

	// Provided by CTFLobbyPlayer/RefTFLobbyPlayer
	virtual const CTFLobbyPlayerProto &Proto() const = 0;
};

// ITFLobbyPlayer that references a proto object elsewhere
class ConstTFLobbyPlayer : public ITFLobbyPlayer
{
public:
	ConstTFLobbyPlayer( const CTFLobbyPlayerProto &proto ) : m_proto( proto ) {}
	ConstTFLobbyPlayer( const ITFLobbyPlayer &other ) : m_proto( other.Proto() ) {}

	virtual const CTFLobbyPlayerProto &Proto() const final OVERRIDE { return m_proto; }
private:
	const CTFLobbyPlayerProto &m_proto;
};

class CTFLobbyInvite : public GCSDK::CProtoBufSharedObject<CTFLobbyInviteProto, k_EProtoObjectTFLobbyInvite>,
                       public GCSDK::IPlayerGroupInvite
{
public:
	typedef GCSDK::CProtoBufSharedObject<CTFLobbyInviteProto, k_EProtoObjectTFLobbyInvite> BaseClass;

	const static int k_nTypeID = k_EProtoObjectTFLobbyInvite;

	ETFMatchGroup GetMatchGroup() const { return Obj().match_group(); }

	// IPlayerGroupInvite
	virtual GCSDK::PlayerGroupID_t GetGroupID() const OVERRIDE { return Obj().lobby_id(); }
	virtual CSteamID GetInviter() const OVERRIDE { return k_steamIDNil; } // Not supported/used in lobby invites
	virtual GCSDK::CSharedObject* GetSharedObject() OVERRIDE { return this; }
};

#endif // TF_LOBBY_SHARED_H
