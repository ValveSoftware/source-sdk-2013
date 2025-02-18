//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  TF specific GC based party
//
//=============================================================================

#ifndef TF_PARTY_H
#define TF_PARTY_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/gcsdk_auto.h"
#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#include "party.h"
#include "tf_matchmaking_shared.h"
#include "tf_matchcriteria.h"


class CTFLobby;

class CTFParty : public GCSDK::CProtoBufSharedObject<CSOTFParty, k_EProtoObjectTFParty>, public GCSDK::IParty
{
	typedef GCSDK::CProtoBufSharedObject<CSOTFParty, k_EProtoObjectTFParty> BaseClass;

public:
	CTFParty();
	virtual ~CTFParty();

	const static int k_nTypeID = k_EProtoObjectTFParty;

	virtual bool BShouldDeleteByCache() const OVERRIDE { return false; }
	virtual GCSDK::PlayerGroupID_t GetGroupID() const OVERRIDE { return Obj().party_id(); }

	// Parties are their own SharedObject for all involved
	virtual CSharedObject* GetSharedObjectForMember( const CSteamID &steamID ) OVERRIDE { return this; }
	// Ambiguous between ISharedObject and IPlayerGroup.
	virtual void Dump() const OVERRIDE { return BaseClass::Dump(); }

	virtual const CSteamID GetLeader() const OVERRIDE;

	uint64 GetAssociatedLobbyID() const { return Obj().associated_lobby_id(); }
	ETFMatchGroup GetAssociatedLobbyMatchGroup() const { return Obj().associated_lobby_match_group(); }

	virtual int GetNumMembers() const OVERRIDE { return Obj().member_ids_size(); }
	virtual const CSteamID GetMember( int i ) const OVERRIDE;
	virtual int GetMemberIndexBySteamID( const CSteamID &steamID ) const OVERRIDE;
#ifdef CLIENT_DLL
	int GetClientCentricMemberIndexBySteamID( const CSteamID &steamID ) const;
	CSteamID GetClientCentricMemberSteamIDByIndex( int nIndex ) const;
#endif
	const CSOTFPartyMember_Activity& GetMemberActivity( int i ) const;
	bool BMemberRequestingLobbyStandby( int i ) const;

	const ConstRefTFPerPlayerMatchCriteria GetMemberMatchCriteria( int i ) const;
	RefTFPerPlayerMatchCriteria MutMemberMatchCriteria( int i );

	virtual int GetNumPendingPlayers() const OVERRIDE { return Obj().pending_members_size(); }
	virtual const CSteamID GetPendingPlayer( int i ) const OVERRIDE;
	virtual CSteamID GetPendingPlayerInviter( int i ) const OVERRIDE;
	virtual EPendingType GetPendingPlayerType( int i ) const OVERRIDE;
	virtual int GetPendingPlayerIndexBySteamID( const CSteamID &steamID ) const OVERRIDE;

	int GetNumQueueEntries() const { return Obj().matchmaking_queues_size(); }
	ETFMatchGroup GetQueueEntryMatchGroup( int i ) const { return Obj().matchmaking_queues(i).match_group(); }
	RTime32 GetQueueEntryStartTime( int i ) const { return Obj().matchmaking_queues(i).queued_time(); }
	int GetQueueEntryIdxByMatchGroup( ETFMatchGroup eMatchGroup ) const;
	bool BQueuedForMatchGroup( ETFMatchGroup eMatchGroup ) const { return GetQueueEntryIdxByMatchGroup( eMatchGroup ) != -1; }

	void SpewDebug();

	bool BAnyMemberWithoutTicket() const;
	bool BAnyMemberWithoutCompetitiveAccess() const;
	bool BAnyMemberWithLowPriority( EMMPenaltyPool eType ) const;
	bool BAnyMembersBanned( EMMPenaltyPool eType ) const;
	bool BMembersIsBanned( const CSteamID& steamID, EMMPenaltyPool eType ) const;
	bool BMemberWithoutCompetitiveAccess( const CSteamID &steamID ) const;
	bool BMemberWithoutTourOfDutyTicket( const CSteamID& steamID ) const;

	const ConstRefTFGroupMatchCriteria GetGroupMatchCriteria() const
		{ return ConstRefTFGroupMatchCriteria( Obj().group_criteria() ); }

};

class CTFPartyInvite : public GCSDK::CProtoBufSharedObject<CSOTFPartyInvite, k_EProtoObjectTFPartyInvite>, public GCSDK::IPlayerGroupInvite
{
	typedef GCSDK::CProtoBufSharedObject<CSOTFPartyInvite, k_EProtoObjectTFPartyInvite> BaseClass;

public:
	const static int k_nTypeID = k_EProtoObjectTFPartyInvite;

	virtual GCSDK::PlayerGroupID_t GetGroupID() const OVERRIDE { return Obj().group_id(); }
	virtual CSteamID GetInviter() const OVERRIDE { return CSteamID( Obj().inviter() ); }
	virtual GCSDK::CSharedObject* GetSharedObject() OVERRIDE { return this; }

	int GetNumMembers() const { return Obj().members_size(); }
	const CSteamID GetMember( int i ) const;
	CTFParty::EPendingType GetType() const;

};
#endif // TF_PARTY_H
