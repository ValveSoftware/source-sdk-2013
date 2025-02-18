//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFLADDERDATA_H
#define TFLADDERDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#if defined (CLIENT_DLL) || defined (GAME_DLL)
#include "gc_clientsystem.h"
#endif

#include "tf_matchmaking_shared.h"


//---------------------------------------------------------------------------------
// Purpose: The shared object that contains a ladder player's stats		
//---------------------------------------------------------------------------------
class CSOTFLadderData : public GCSDK::CProtoBufSharedObject< CSOTFLadderPlayerStats, k_EEConTypeLadderData >
{
public:
	CSOTFLadderData();
	CSOTFLadderData( uint32 unAccountID, ETFMatchGroup eMatchGroup );

};


CSOTFLadderData *YieldingGetPlayerLadderDataBySteamID( const CSteamID &steamID, ETFMatchGroup nMatchGroup );
const CSOTFLadderData *GetLocalPlayerLadderData( ETFMatchGroup nMatchGroup );	// TODO: GetSeasonID()

//---------------------------------------------------------------------------------
// Purpose: The shared object that contains stats from a specific match - for match history on the client
//---------------------------------------------------------------------------------
class CSOTFMatchResultPlayerInfo : public GCSDK::CProtoBufSharedObject< CSOTFMatchResultPlayerStats, k_EEConTypeMatchResultPlayerInfo >
{
public:
	CSOTFMatchResultPlayerInfo();
};

void GetLocalPlayerMatchHistory( ETFMatchGroup nMatchGroup, CUtlVector < CSOTFMatchResultPlayerStats > &vecMatchesOut );

#endif // TFLADDERDATA_H
