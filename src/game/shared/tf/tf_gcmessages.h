//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file defines all of our over-the-wire net protocols for the
//			Game Coordinator for Team Fortress.  Note that we never use types
//			with undefined length (like int).  Always use an explicit type 
//			(like int32).
//
//=============================================================================

#ifndef TF_GCMESSAGES_H
#define TF_GCMESSAGES_H
#ifdef _WIN32
#pragma once
#endif

#include "language.h"

// Protobuf headers interfere with the valve min/max/malloc overrides. so we need to do all
// this funky wrapping to make the include happy.
#include <tier0/valve_minmax_off.h>

#include "tf_gcmessages.pb.h"

#include <tier0/valve_minmax_on.h>

#pragma pack( push, 1 )

//-----------------------------------------------------------------------------
// Type IDs for TF GC classes. These are part of the client-GC protocol and
// should not change if it can be helped
//-----------------------------------------------------------------------------
enum EGCTFProtoObjectTypes
{
	k_EProtoObjectTypesGameBase			= 2000,

//	k_EProtoObjectHeroStandings			= k_EProtoObjectTypesGameBase + 1,
//	k_EProtoObjectGameAccountClient		= k_EProtoObjectTypesGameBase + 2,
	k_EProtoObjectTFParty				= k_EProtoObjectTypesGameBase + 3,
	k_EProtoObjectTFGameServerLobby		= k_EProtoObjectTypesGameBase + 4,
//	k_EProtoObjectBetaParticipation		= k_EProtoObjectTypesGameBase + 5,
	k_EProtoObjectTFPartyInvite			= k_EProtoObjectTypesGameBase + 6,
	k_EProtoObjectTFRatingData			= k_EProtoObjectTypesGameBase + 7,
	k_EProtoObjectTFLobbyInvite			= k_EProtoObjectTypesGameBase + 8,
};

//=============================================================================
// Duel

// k_EMsgGC_Duel_Request
struct MsgGC_Duel_Request_t
{
	uint64 m_ulInitiatorSteamID;
	uint64 m_ulTargetSteamID;
	uint8 m_usAsPlayerClass;
};

// k_EMsgGC_Duel_Response
struct MsgGC_Duel_Response_t
{
	uint64 m_ulInitiatorSteamID;
	uint64 m_ulTargetSteamID;
	bool m_bAccepted;
	uint8 m_usAsPlayerClass;
};

// k_EMsgGC_Duel_Results
struct MsgGC_Duel_Results_t
{
	uint64 m_ulInitiatorSteamID;
	uint64 m_ulTargetSteamID;
	uint64 m_ulWinnerSteamID;
	uint16 m_usScoreInitiator;
	uint16 m_usScoreTarget;
	uint8 m_usEndReason;
};

// k_EMsgGC_Duel_Status
enum EGCDuelStatus
{
	kDuel_Status_Invalid					= -1,
	kDuel_Status_AlreadyInDuel_Inititator,
	kDuel_Status_AlreadyInDuel_Target,
	kDuel_Status_DuelBanned_Initiator,
	kDuel_Status_DuelBanned_Target,
	kDuel_Status_MissingSession,				// could be gameserver session or target client session
	kDuel_Status_Cancelled,
};
struct MsgGC_Duel_Status_t
{
	uint8 m_usStatus;
	uint64 m_ulInitiatorSteamID;
	uint64 m_ulTargetSteamID;
};

//=============================================================================

// k_EMsgGC_MM_RequestMatch
struct MsgGC_MM_RequestMatch_t
{
	uint32 m_unRequiredGameServerFlags;
	// string with map name
};

// k_EMsgGC_MM_RequestMatchResponse
struct MsgGC_MM_RequestMatchResponse_t
{
	bool m_bServerFound;
	uint32 m_iServerAddress;
	uint16 m_iServerPort;
};

// k_EMsgGC_MM_ReserveSpot
struct MsgGC_MM_ReserveSpot_t
{
	uint64 m_ulSteamID;
};

// k_EMsgGC_MM_LoadMap
struct MsgGC_MM_LoadMap_t
{
	// string with map name
};

struct MsgGCChatMessage_t
{
	// string sChannelName
	// string sPersonaName
	int32 m_cMsgLen;
	// binary message
};

//=============================================================================

#pragma pack( pop )

// Normal:
#define MATCHMAKING_SPEWLEVEL4 4
#define MATCHMAKING_SPEWLEVEL3 4
#define MATCHMAKING_SPEWLEVEL2 2

// Use these defines to crank up the spew level
//#define MATCHMAKING_SPEWLEVEL4 1
//#define MATCHMAKING_SPEWLEVEL3 1
//#define MATCHMAKING_SPEWLEVEL2 1

#endif
