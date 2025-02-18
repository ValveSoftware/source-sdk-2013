//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFWARDATA_H
#define TFWARDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#if defined (CLIENT_DLL) || defined (GAME_DLL)
	#include "gc_clientsystem.h"
#endif


//---------------------------------------------------------------------------------
// Purpose: The shared object that contains a user's stats for a war	
//---------------------------------------------------------------------------------
class CWarData : public GCSDK::CProtoBufSharedObject< CSOWarData, k_EEConTypeWarData >
{
public:
	CWarData();
};


#if defined( CLIENT_DLL ) || defined( GC )
//---------------------------------------------------------------------------------
// Purpose: Holds the global stats for a war
//
//			On the GC, the global stats are tabulated at load-time from all current
//			SQL records and is then modified in-memory.  
//
//			On the client, the global stats are requested whenever they are queried
//			but we limit how often we request.
//---------------------------------------------------------------------------------
class CTFWarGlobalDataHelper
{
public:
#ifdef CLIENT_DLL
	struct LeaderBoardEntries_t
	{
		LeaderBoardEntries_t() : m_bInitialized( false ) {}

		bool m_bInitialized;
		CUtlVector< LeaderboardEntry_t* > m_vecEntries;
	};
#endif // CLIENT_DLL

	CTFWarGlobalDataHelper();
	void Init();
	bool BIsInitialized() const { return m_bInitialized; }

	void AddToSideScore( war_definition_index_t nWar, war_side_t nSide, uint32 nValue );
	uint64 GetGlobalSideScore( war_definition_index_t nWar, war_side_t nSide );

	CGCMsgGC_War_GlobalStatsResponse* FindOrCreateWarData( war_definition_index_t nWarDef, bool bCreateIfDoesntExist );
	CGCMsgGC_War_GlobalStatsResponse_SideScore* FindOrCreateWarDataSide( war_side_t nWarSide, war_definition_index_t nWarDef, bool bCreateIfDoesntExist );

	void SetGlobalStats( const CGCMsgGC_War_GlobalStatsResponse& newData );

#ifdef CLIENT_DLL
	const LeaderBoardEntries_t& GetGlobalLeaderboardScores() const { return downloadedLeaderboardScoresGlobal; }
	const LeaderBoardEntries_t& GetFriendsLeaderboardScores() const { return downloadedLeaderboardScoresFriends; }
#endif // CLIENT_DLL

private:

#ifdef CLIENT_DLL
	void RequestUpdateGlobalStats();
	void CheckGlobalStatsStaleness();

	// Leaderboard functions
	void RequestLeaderboard();
	void OnFindLeaderboard( LeaderboardFindResult_t *pResult, bool bIOFailure );
	void DownloadLeaderboard();
	void OnLeaderboardScoresDownloaded_Global( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
	void OnLeaderboardScoresDownloaded_Friends( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
#endif // CLIENT_DLL

	bool m_bInitialized;

	typedef CUtlMap< war_definition_index_t, CGCMsgGC_War_GlobalStatsResponse > WarStatsMap_t;
	WarStatsMap_t m_mapWarStats;

#ifdef CLIENT_DLL
	float m_flLastUpdateRequest;
	float m_flLastUpdated;
	LeaderboardFindResult_t m_findLeaderboardResults;

	CCallResult< CTFWarGlobalDataHelper, LeaderboardFindResult_t > m_findLeaderboardCallback;
	LeaderBoardEntries_t downloadedLeaderboardScoresGlobal;
	LeaderBoardEntries_t downloadedLeaderboardScoresFriends;

	CCallResult< CTFWarGlobalDataHelper, LeaderboardScoresDownloaded_t > downloadLeaderboardCallbackGlobal;
	CCallResult< CTFWarGlobalDataHelper, LeaderboardScoresDownloaded_t > downloadLeaderboardCallbackFriends;
#endif // CLIENT_DLL
};

CTFWarGlobalDataHelper& GetWarData();
#endif // CLIENT_DLL || GC

CWarData* GetPlayerWarData( const CSteamID& steamID, war_definition_index_t warDefIndex, bool bLoadEvenIfWarInactive 
	);
#ifdef CLIENT_DLL
CWarData* GetLocalPlayerWarData( war_definition_index_t warDefIndex );
#endif

#endif // TFWARDATA_H
