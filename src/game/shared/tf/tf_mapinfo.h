//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_MAPINFO_H
#define TF_MAPINFO_H
#ifdef _WIN32
#pragma once
#endif

// Formally Mapinfo
enum ETFLeaderboardType
{
	kMapLeaderboard,
	kDuckLeaderboard,
	kDuckStat,
	kLadderLeaderboard,
};

class CLeaderboardInfo
{
public:
	CCallResult< CLeaderboardInfo, LeaderboardFindResult_t > findLeaderboardCallback;
	CCallResult< CLeaderboardInfo, LeaderboardScoresDownloaded_t > downloadLeaderboardCallbackGlobal;
	CCallResult< CLeaderboardInfo, LeaderboardScoresDownloaded_t > downloadLeaderboardCallbackGlobalAroundUser;
	CCallResult< CLeaderboardInfo, LeaderboardScoresDownloaded_t > downloadLeaderboardCallbackFriends;
	LeaderboardFindResult_t findLeaderboardResults;
	CUtlVector< LeaderboardEntry_t* > downloadedLeaderboardScoresGlobal;
	CUtlVector< LeaderboardEntry_t* > downloadedLeaderboardScoresGlobalAroundUser;
	CUtlVector< LeaderboardEntry_t* > downloadedLeaderboardScoresFriends;
	int iNumLeaderboardEntries;
	ETFLeaderboardType m_kLeaderboardType;

	CLeaderboardInfo( const char *pLeaderboardName );
	~CLeaderboardInfo();

	const char *GetLeaderboardName() { return m_pLeaderboardName; }

	void RetrieveLeaderboardData();
	bool DownloadLeaderboardData();
	void OnFindLeaderboard( LeaderboardFindResult_t *pResult, bool bIOFailure );
	void OnLeaderboardScoresDownloadedGlobal( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
	void OnLeaderboardScoresDownloadedGlobalAroundUser( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
	void OnLeaderboardScoresDownloadedFriends( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );

	void SetMyScore ( int score );
	int GetMyScore() { return m_iMyScore; }

	bool HasPendingUpdate() { return m_bHasPendingUpdate; }
	void SetHasPendingUpdate( bool bStatus ) { m_bHasPendingUpdate = bStatus; }

	bool IsLeaderboardFound() { return m_bLeaderboardFound; }

private:
	const char *m_pLeaderboardName;
	int m_iMyScore;
	bool m_bHasPendingUpdate;
	bool m_bLeaderboardFound;
};

/**
 * Refresh the leaderboard for the given map
 * @param pMapName
 */
void MapInfo_RefreshLeaderboard( const char *pMapName );

/**
 * Retrieve the leaderboard for the given map
 * @param pMapName
 * @param scores
 * @return true if the leaderboard info was retrieved, false otherwise
 */
bool MapInfo_GetLeaderboardInfo( const char *pMapName, CUtlVector< LeaderboardEntry_t* > &scores, int &iNumLeaderboardEntries, uint32 unMinScores );

/**
 * @param unAccountID
 * @param pLevelName
 * @return how many times the player has donated
 */
int MapInfo_GetDonationAmount( uint32 unAccountID, const char *pLevelName );

/**
 * @param unAccountID
 * @param pLevelName
 * return true if the player donated to the current map, false otherwise
 */
bool MapInfo_DidPlayerDonate( uint32 unAccountID, const char *pLevelName );

/**
 * Retrieve the duel wins leaderboard
 * @param scores
 * @param bGlobal
 * return true if the duel wins leaderboard were retrieved, false otherwise
 */
bool Leaderboards_GetDuelWins( CUtlVector< LeaderboardEntry_t* > &scores, bool bGlobal );

// Get a list of AccountID's for all people on the Duck Leaderboards
void Leaderboards_GetDuckLeaderboardSteamIDs( CUtlVector< AccountID_t > &vecIds );

/**
 * Retrieve the duel wins leaderboard
 * @param scores
 * @param bGlobal
 * return true if the duel wins leaderboard were retrieved, false otherwise
 */
bool Leaderboards_GetDuckLeaderboard( CUtlVector< LeaderboardEntry_t* > &scores, const char* kName );

// Get total number of entries for a leaderboard type
int Leaderboards_GetDuckLeaderboardTotalEntryCount( const char* kName );

/**
 * Refreshes leaderboards not associated with maps
 */
void Leaderboards_Refresh();

/**
* Retrieve the competitive ladder ratings leaderboard
* @param scores
* @param nType
* return true if the leaderboard were retrieved, false otherwise
*/
bool Leaderboards_GetLadderLeaderboard( CUtlVector< LeaderboardEntry_t* > &scores, const char *pszName, bool bGlobal );
void Leaderboards_LadderRefresh( void );
#endif // TF_MAPINFO_H
