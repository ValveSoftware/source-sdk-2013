//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "tf_mapinfo.h"
#include <filesystem.h>
#include "GameEventListener.h"
#include "econ_item_system.h"
#include "tf_item_inventory.h"
#include "econ_contribution.h"
#include "tf_duel_summary.h"
#include "gc_clientsystem.h"
#include "tf_duckleaderboard.h"
#include "tf_gamerules.h"
#include "tf_matchmaking_shared.h"

#ifdef CLIENT_DLL
#include "usermessages.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef CLIENT_DLL
ConVar tf_duck_upload_rate( "tf_duck_upload_rate", "2400", FCVAR_DEVELOPMENTONLY );		// Make this DevOnly At ship and 60 seconds
#endif

//-----------------------------------------------------------------------------
int SortLeaderboardVec( LeaderboardEntry_t * const *p1, LeaderboardEntry_t * const *p2 )
{
	return ( *p2 )->m_nScore - ( *p1 )->m_nScore;
}
//-----------------------------------------------------------------------------

static void RetrieveLeaderboardEntries( LeaderboardScoresDownloaded_t &scores, CUtlVector< LeaderboardEntry_t* > &entries )
{
	entries.PurgeAndDeleteElements();
	entries.EnsureCapacity( scores.m_cEntryCount );
	for ( int i = 0; i < scores.m_cEntryCount; ++i )
	{
		LeaderboardEntry_t *leaderboardEntry = new LeaderboardEntry_t;
		if ( steamapicontext->SteamUserStats()->GetDownloadedLeaderboardEntry( scores.m_hSteamLeaderboardEntries, i, leaderboardEntry, NULL, 0 ) )
		{
			entries.AddToTail( leaderboardEntry );
		}
	}
}


CLeaderboardInfo::CLeaderboardInfo( const char *pLeaderboardName )
{
	m_pLeaderboardName = pLeaderboardName ? V_strdup( pLeaderboardName ) : NULL;
	memset( &findLeaderboardResults, 0, sizeof( findLeaderboardResults ) );
	iNumLeaderboardEntries = 0;
	m_kLeaderboardType = kMapLeaderboard;
	m_iMyScore = 0;
	m_bHasPendingUpdate = false;
	m_bLeaderboardFound = false;
}

CLeaderboardInfo::~CLeaderboardInfo()
{
	downloadedLeaderboardScoresGlobal.PurgeAndDeleteElements();
	downloadedLeaderboardScoresGlobalAroundUser.PurgeAndDeleteElements();
	downloadedLeaderboardScoresFriends.PurgeAndDeleteElements();
	delete m_pLeaderboardName;
}

void CLeaderboardInfo::RetrieveLeaderboardData()
{
	if ( steamapicontext && steamapicontext->SteamUserStats() )
	{
		if ( m_kLeaderboardType == kMapLeaderboard )
		{
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->FindLeaderboard( CFmtStr( "contributions_%s", m_pLeaderboardName ) );
			findLeaderboardCallback.Set( apicall, this, &CLeaderboardInfo::OnFindLeaderboard );
		}
		else if ( m_kLeaderboardType == kDuckLeaderboard || m_kLeaderboardType == kDuckStat )
		{
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->FindLeaderboard( m_pLeaderboardName );
			findLeaderboardCallback.Set( apicall, this, &CLeaderboardInfo::OnFindLeaderboard );
		}
		else if ( m_kLeaderboardType == kLadderLeaderboard )
		{
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->FindLeaderboard( m_pLeaderboardName );
			findLeaderboardCallback.Set( apicall, this, &CLeaderboardInfo::OnFindLeaderboard );
		}
	}
}

bool CLeaderboardInfo::DownloadLeaderboardData()
{
	if ( !findLeaderboardResults.m_bLeaderboardFound )
		return false;

	if ( m_kLeaderboardType == kMapLeaderboard )
	{
		SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestGlobal, 1, 5 );
		downloadLeaderboardCallbackGlobal.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedGlobal );
		apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestGlobalAroundUser, -2, 2 );
		downloadLeaderboardCallbackGlobalAroundUser.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedGlobalAroundUser );
		apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, 1, 5 );
		downloadLeaderboardCallbackFriends.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedFriends );
		return true;
	}

	if ( m_kLeaderboardType == kDuckLeaderboard )
	{
		SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, -6, 6 );
		downloadLeaderboardCallbackFriends.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedFriends );
		return true;
	}

	if ( m_kLeaderboardType == kDuckStat )
	{
		SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, 0, 0 );
		downloadLeaderboardCallbackFriends.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedFriends );
		return true;
	}

	if ( m_kLeaderboardType == kLadderLeaderboard )
	{
		SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestGlobal, 1, 100 );
		downloadLeaderboardCallbackGlobal.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedGlobal );
		apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, -45, 45 );
		downloadLeaderboardCallbackFriends.Set( apicall, this, &CLeaderboardInfo::OnLeaderboardScoresDownloadedFriends );
		return true;
	}

	return false;
}

void CLeaderboardInfo::OnFindLeaderboard( LeaderboardFindResult_t *pResult, bool bIOFailure )
{
	findLeaderboardResults = *pResult;
}

void CLeaderboardInfo::OnLeaderboardScoresDownloadedGlobal( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
{
	RetrieveLeaderboardEntries( *pResult, downloadedLeaderboardScoresGlobal );
	iNumLeaderboardEntries = steamapicontext->SteamUserStats()->GetLeaderboardEntryCount( findLeaderboardResults.m_hSteamLeaderboard );
}

void CLeaderboardInfo::OnLeaderboardScoresDownloadedGlobalAroundUser( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
{
	RetrieveLeaderboardEntries( *pResult, downloadedLeaderboardScoresGlobalAroundUser );
}

void CLeaderboardInfo::OnLeaderboardScoresDownloadedFriends( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
{
	RetrieveLeaderboardEntries( *pResult, downloadedLeaderboardScoresFriends );
	iNumLeaderboardEntries = steamapicontext->SteamUserStats()->GetLeaderboardEntryCount( findLeaderboardResults.m_hSteamLeaderboard );
	CSteamID localID;
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		localID = steamapicontext->SteamUser()->GetSteamID();
	}

	FOR_EACH_VEC( downloadedLeaderboardScoresFriends, i )
	{
		if ( downloadedLeaderboardScoresFriends[i]->m_steamIDUser == localID )
		{
			if ( m_iMyScore < downloadedLeaderboardScoresFriends[i]->m_nScore )
			{
				// First update on finding the leaderboard, any gotten kills need to add to accumulate
				if ( m_bLeaderboardFound == false )
				{
					if ( m_iMyScore > 0 )
					{
						m_bHasPendingUpdate = true;
					}
					m_iMyScore += downloadedLeaderboardScoresFriends[i]->m_nScore;
				}
				else
				{
					m_iMyScore = downloadedLeaderboardScoresFriends[i]->m_nScore;
				}
			}

			// Use My Saved Score
			downloadedLeaderboardScoresFriends[i]->m_nScore = m_iMyScore;
		}
	}

	downloadedLeaderboardScoresFriends.Sort( &SortLeaderboardVec );
	
	m_bLeaderboardFound = true;
}

void CLeaderboardInfo::SetMyScore( int score )
{
	m_iMyScore = score;

	if ( !m_bLeaderboardFound )
		return;

	// Update my leaderboard and resort
	iNumLeaderboardEntries = steamapicontext->SteamUserStats()->GetLeaderboardEntryCount( findLeaderboardResults.m_hSteamLeaderboard );
	CSteamID localID;
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		localID = steamapicontext->SteamUser()->GetSteamID();
	}

	FOR_EACH_VEC( downloadedLeaderboardScoresFriends, i )
	{
		if ( downloadedLeaderboardScoresFriends[i]->m_steamIDUser == localID )
		{
			if ( m_iMyScore > downloadedLeaderboardScoresFriends[i]->m_nScore )
			{
				// Use My Saved Score
				downloadedLeaderboardScoresFriends[i]->m_nScore = m_iMyScore;
				downloadedLeaderboardScoresFriends.Sort( &SortLeaderboardVec );
				break;
			}
		}
	}

}

//-----------------------------------------------------------------------------
class CMapInfoContainer : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:

	CMapInfoContainer()
	{
		memset( &m_findDuelLeaderboardResults, 0, sizeof( m_findDuelLeaderboardResults ) );
		m_flNextUpdateDuckScoreTime = Plat_FloatTime() + 10.0f;
#ifdef CLIENT_DLL
		m_flNextDuckScoresUploadTime = Plat_FloatTime() + tf_duck_upload_rate.GetFloat();
#endif 
		m_flNextLadderUpdateTime = Plat_FloatTime() + 10.f;
	}

	virtual char const *Name()
	{
		return "CMapInfoContainer";
	}

	~CMapInfoContainer()
	{
		m_vecMapInfos.PurgeAndDeleteElements();
		m_downloadedDuelLeaderboardScores_GlobalAroundUser.PurgeAndDeleteElements();
		m_downloadedDuelLeaderboardScores_Friends.PurgeAndDeleteElements();

		// For ducks
		m_vecDuckInfo.PurgeAndDeleteElements();
		// Ladders
		m_vecLadderLeaderboards.PurgeAndDeleteElements();
	}

#ifdef CLIENT_DLL

	virtual void LevelShutdownPreEntity() 
	{
		// upload scores on level leave
		//DuckUploadPendingScores();
	}

	// Gets called each frame
	virtual void Update( float frametime )
	{
		if ( m_flNextUpdateDuckScoreTime > 0 && m_flNextUpdateDuckScoreTime <  Plat_FloatTime() )
		{
			if ( DownloadDuckLeaderboard() )
			{
				m_flNextUpdateDuckScoreTime = -1.0f;
			}
		}

		if ( m_flNextLadderUpdateTime > 0.f && m_flNextLadderUpdateTime < Plat_FloatTime() )
		{
			if ( DownloadLadderLeaderboard() )
			{
				m_flNextLadderUpdateTime = -1.f;
			}
		}

		// Duck Journal is off, no longer uploading
		//if ( m_flNextDuckScoresUploadTime < Plat_FloatTime() )
		//{
		//	// Hard limit the rate players can update scores
		//	float flNextUpdateTime = tf_duck_upload_rate.GetFloat();
		//	if ( DuckUploadPendingScores() )
		//	{
		//		// Request new score
		//		m_flNextUpdateDuckScoreTime = Plat_FloatTime() + 10.0f;
		//	}
		//	// 4x as long if you don't have a Duck Journal
		//	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		//	if ( pPlayer )
		//	{
		//		static CSchemaAttributeDefHandle pAttr_DuckLevelBadge( "duck badge level" );
		//		if ( pAttr_DuckLevelBadge )
		//		{
		//			CTFWearable *pActionItem = pPlayer->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
		//			// Don't care about the level, just if the attribute is found
		//			if ( pActionItem && FindAttribute( pActionItem->GetAttributeContainer()->GetItem(), pAttr_DuckLevelBadge ) )
		//			{
		//				flNextUpdateTime *= 0.5f;
		//			}
		//		}
		//	}
		//	m_flNextDuckScoresUploadTime = Plat_FloatTime() + flNextUpdateTime;
		//}
	}
#endif // CLIENT_DLL

	//-----------------------------------------------------------------------------
	// for duels
	void DownloadDuelLeaderboard()
	{
		if ( m_findDuelLeaderboardResults.m_bLeaderboardFound )
		{
			// and start downloading the leaderboards
			// friends
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( m_findDuelLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, 1, 10 );
			m_downloadLeaderboardCallback_Friends.Set( apicall, this, &CMapInfoContainer::OnDuelLeaderboardScoresDownloaded_Friends );			
			// global around user
			apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( m_findDuelLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestGlobalAroundUser, -4, 5 );
			m_downloadLeaderboardCallback_GlobalAroundUser.Set( apicall, this, &CMapInfoContainer::OnDuelLeaderboardScoresDownloaded_GlobalAroundUser );
		}
	}

	void OnFindDuelLeaderboard( LeaderboardFindResult_t *pResult, bool bIOFailure )
	{
		m_findDuelLeaderboardResults = *pResult;
		DownloadDuelLeaderboard();
	}

	void OnDuelLeaderboardScoresDownloaded_GlobalAroundUser( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
	{
		RetrieveLeaderboardEntries( *pResult, m_downloadedDuelLeaderboardScores_GlobalAroundUser );		
	}

	void OnDuelLeaderboardScoresDownloaded_Friends( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
	{
		RetrieveLeaderboardEntries( *pResult, m_downloadedDuelLeaderboardScores_Friends );
	}

	// **************************************************************************************************************************
	bool DownloadLadderLeaderboard()
	{
		bool bDownloading = false;
		FOR_EACH_VEC( m_vecLadderLeaderboards, i )
		{
			bDownloading |= m_vecLadderLeaderboards[i]->DownloadLeaderboardData();
		}
		return bDownloading;
	}

	CLeaderboardInfo *GetLadderLeaderboard( const char *pszName )
	{
		FOR_EACH_VEC( m_vecLadderLeaderboards, i )
		{
			CLeaderboardInfo *pInfo = m_vecLadderLeaderboards[i];
			if ( pszName && pInfo && !V_strcmp( pszName, pInfo->GetLeaderboardName() ) )
			{
				return pInfo;
			}
		}

		return NULL;
	}

	// **************************************************************************************************************************
	bool DownloadDuckLeaderboard()
	{
		bool bDownloading = false;
		FOR_EACH_VEC( m_vecDuckInfo, i )
		{
			bDownloading |= m_vecDuckInfo[i]->DownloadLeaderboardData();
		}
		return bDownloading;
	}

	CLeaderboardInfo *GetDuckLeaderboard( const char* kName )
	{
		FOR_EACH_VEC( m_vecDuckInfo, i )
		{
			CLeaderboardInfo *pInfo = m_vecDuckInfo[i];
			if ( strstr( kName, pInfo->GetLeaderboardName() ) != NULL )
			{
				return pInfo;
			}
		}

		return NULL;
	}

	bool DuckUploadPendingScores()
	{
		return false;

		//CSteamID localID;

		//if ( !steamapicontext || !steamapicontext->SteamUser() )
		//	return false;

		//localID = steamapicontext->SteamUser()->GetSteamID();

		//bool bUpdatedScores = false;
		//for ( int i = 0; i < DUCK_NUM_LEADERBOARDS; ++i )
		//{
		//	CLeaderboardInfo *pLeaderboard = GetDuckLeaderboard( g_szDuckLeaderboardNames[i] );
		//	
		//	if ( pLeaderboard && pLeaderboard->IsLeaderboardFound() && pLeaderboard->HasPendingUpdate() )
		//	{
		//		pLeaderboard->SetHasPendingUpdate( false );
		//		bUpdatedScores = true;

		//		int iScoreCheck = RandomInt( INT_MAX / 2, INT_MAX );
		//		// Tell the GC to update our duck contribution
		//		GCSDK::CProtoBufMsg<CGCMsgGC_PlayerDuckLeaderboard_IndividualUpdate> msg( k_EMsgGC_DuckLeaderboard_IndividualUpdate );
		//		msg.Body().set_score( pLeaderboard->GetMyScore() );
		//		msg.Body().set_type( i );

		//		MD5Context_t md5Context;
		//		MD5Init( &md5Context );
		//		
		//		AccountID_t unAccountId = localID.GetAccountID();
		//		int nScore = pLeaderboard->GetMyScore();

		//		MD5Update( &md5Context, static_cast<const uint8 *>( (void *)&unAccountId ), sizeof( unAccountId ) );
		//		MD5Update( &md5Context, static_cast<const uint8 *>( (void *)&nScore ), sizeof( nScore ) );
		//		MD5Update( &md5Context, static_cast<const uint8 *>( (void *)&i ), sizeof( i ) );
		//		MD5Update( &md5Context, static_cast<const uint8 *>( (void *)&TF_DUCK_ID ), sizeof( TF_DUCK_ID ) );
		//		MD5Update( &md5Context, static_cast<const uint8 *>( (void *)&iScoreCheck ), sizeof( iScoreCheck ) );
		//		
		//		MD5Value_t md5Result;
		//		MD5Final( &md5Result.bits[0], &md5Context );
		//		msg.Body().set_score_id( &md5Result.bits[0], MD5_DIGEST_LENGTH );
		//		msg.Body().set_score_check( iScoreCheck );
		//		GCClientSystem()->BSendMessage( msg );
		//	}
		//}
		//return bUpdatedScores;
	}

	void DuckUpdateScore( int iIncrement, EDuckLeaderboardTypes kLeaderboard )
	{
		// Get Current Score
		CLeaderboardInfo *pLeaderboard = GetDuckLeaderboard( g_szDuckLeaderboardNames[kLeaderboard] );
		int iCurrentScore = pLeaderboard->GetMyScore();	
		int iNewScore = iCurrentScore + iIncrement;

#ifdef CLIENT_DLL
		int iOldLevel = iCurrentScore / DUCK_XP_SCALE;
		int iNewLevel = iNewScore / DUCK_XP_SCALE;
		
		if ( iNewLevel > iOldLevel )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "duck_xp_level_up" );
			if ( event )
			{
				event->SetInt( "level", iNewLevel );
				gameeventmanager->FireEventClientSide( event );
			}
		}
#endif

		// Set my new score
		pLeaderboard->SetMyScore( iNewScore );
		pLeaderboard->SetHasPendingUpdate( true );
	}

	//-----------------------------------------------------------------------------
	virtual bool Init()
	{
		ListenForGameEvent( "item_schema_initialized" );
		return true;
	}

	virtual void FireGameEvent( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "item_schema_initialized" ) != 0 )
			return;

		for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
		{
			CLeaderboardInfo *pInfo = new CLeaderboardInfo( GetItemSchema()->GetMasterMapDefByIndex( i )->pszMapName );
			pInfo->m_kLeaderboardType = kMapLeaderboard;
			m_vecMapInfos.AddToTail( pInfo );

			const MapDef_t *pMapDef = GetItemSchema()->GetMasterMapDefByName( pInfo->GetLeaderboardName() );
			if ( pMapDef && pMapDef->IsCommunityMap() )
			{
				// retrieve leaderboard info
				pInfo->RetrieveLeaderboardData();
			}
		}

		// find duel leaderboards
		if ( steamapicontext && steamapicontext->SteamUserStats() )
		{
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->FindLeaderboard( "duel_wins" );
			m_findLeaderboardCallback.Set( apicall, this, &CMapInfoContainer::OnFindDuelLeaderboard );
		}

		// find duck leaderboards
		for ( int i = 0; i < DUCK_NUM_LEADERBOARDS; i++ )
		{
			CLeaderboardInfo *pInfo = new CLeaderboardInfo( g_szDuckLeaderboardNames[ i ] );
			pInfo->m_kLeaderboardType = i == 0 ? kDuckLeaderboard : kDuckStat;
			m_vecDuckInfo.AddToTail( pInfo );

			// retrieve leaderboard info
			pInfo->RetrieveLeaderboardData();
		}

		// Ladder
		for ( int i = 0; i < k_eMatchGroupLeaderboard_Count; i++ )
		{
			EMatchGroupLeaderboard eLeaderboard = (EMatchGroupLeaderboard)i;
			CLeaderboardInfo *pInfo = new CLeaderboardInfo( GetMatchGroupLeaderboardName( eLeaderboard ) );
			pInfo->m_kLeaderboardType = kLadderLeaderboard;
			m_vecLadderLeaderboards.AddToTail( pInfo );

			// retrieve leaderboard info
			pInfo->RetrieveLeaderboardData();
		}
	}

public:

	CUtlVector< CLeaderboardInfo* > m_vecMapInfos;	
	// for duels
	CCallResult< CMapInfoContainer, LeaderboardFindResult_t > m_findLeaderboardCallback;
	CCallResult< CMapInfoContainer, LeaderboardScoresDownloaded_t > m_downloadLeaderboardCallback_GlobalAroundUser;
	CCallResult< CMapInfoContainer, LeaderboardScoresDownloaded_t > m_downloadLeaderboardCallback_Friends;
	LeaderboardFindResult_t m_findDuelLeaderboardResults;
	CUtlVector< LeaderboardEntry_t* > m_downloadedDuelLeaderboardScores_GlobalAroundUser;
	CUtlVector< LeaderboardEntry_t* > m_downloadedDuelLeaderboardScores_Friends;

	// For ducks
	CUtlVector< CLeaderboardInfo* > m_vecDuckInfo;
	float m_flNextUpdateDuckScoreTime;
	float m_flNextDuckScoresUploadTime;

	// Ladders
	CUtlVector< CLeaderboardInfo* > m_vecLadderLeaderboards;
	float m_flNextLadderUpdateTime;
};
CMapInfoContainer gMapInfoContainer;

static CLeaderboardInfo *FindMapInfo( const char *pMapName )
{
	FOR_EACH_VEC( gMapInfoContainer.m_vecMapInfos, i )
	{
		CLeaderboardInfo *pInfo = gMapInfoContainer.m_vecMapInfos[i];
		if ( strstr( pMapName, pInfo->GetLeaderboardName() ) != NULL )
			return pInfo;
	}
	return NULL;
}

//-----------------------------------------------------------------------------

bool Leaderboards_GetDuelWins( CUtlVector< LeaderboardEntry_t* > &scores, bool bGlobal )
{
	if ( gMapInfoContainer.m_findDuelLeaderboardResults.m_bLeaderboardFound )
	{
		if ( bGlobal )
		{
			scores = gMapInfoContainer.m_downloadedDuelLeaderboardScores_GlobalAroundUser;
		}
		else
		{
			scores = gMapInfoContainer.m_downloadedDuelLeaderboardScores_Friends;
		}
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
// DUCKS
void Leaderboards_GetDuckLeaderboardSteamIDs( CUtlVector< AccountID_t > &vecIds )
{
	vecIds.RemoveAll();
	FOR_EACH_VEC( gMapInfoContainer.m_vecDuckInfo, i )
	{
		FOR_EACH_VEC( gMapInfoContainer.m_vecDuckInfo[i]->downloadedLeaderboardScoresFriends, iEntry )
		{
			vecIds.AddToHead( gMapInfoContainer.m_vecDuckInfo[i]->downloadedLeaderboardScoresFriends[iEntry]->m_steamIDUser.GetAccountID() );
		}
	}
}

bool Leaderboards_GetDuckLeaderboard( CUtlVector< LeaderboardEntry_t* > &scores, const char* kName )
{
	CLeaderboardInfo *pLeaderboard = gMapInfoContainer.GetDuckLeaderboard( kName );
	if ( pLeaderboard && pLeaderboard->IsLeaderboardFound() )
	{
		scores = pLeaderboard->downloadedLeaderboardScoresFriends;
		return true;
	}
	return false;
}

int Leaderboards_GetDuckLeaderboardTotalEntryCount( const char* kName )
{
	CLeaderboardInfo *pLeaderboard = gMapInfoContainer.GetDuckLeaderboard( kName );
	if ( pLeaderboard )
	{
		return pLeaderboard->iNumLeaderboardEntries;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// DUCK Collected Message from Server
#ifdef CLIENT_DLL
USER_MESSAGE( EOTLDuckEvent )
{
	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	if ( TFGameRules() && TFGameRules()->HaveCheatsBeenEnabledDuringLevel() )
		return;

	// IsCreated, ID of Creator, ID of Victim, Count, IsGolden
	int iIsCreated	= (int)msg.ReadByte();
	int iCreatorId	= (int)msg.ReadByte();
	int iVictimId	= (int)msg.ReadByte();
	int iToucherId	= (int)msg.ReadByte();
	int iDuckTeam	= (int)msg.ReadByte();
	int iCount		= (int)msg.ReadByte();
	int iDuckFlags	= (int)msg.ReadByte();

	iDuckTeam = 0;
	iVictimId = 0;
	//iDuckFlags = 0;

	CBasePlayer *pCreator = UTIL_PlayerByIndex( iCreatorId );
	//CBasePlayer *pVictim = UTIL_PlayerByIndex( iVictimId );
	CBasePlayer *pToucher = UTIL_PlayerByIndex( iToucherId );

	if ( !pCreator )
	{
		iDuckFlags |= DUCK_FLAG_OBJECTIVE;
	}
	// If you were picked up, you need a toucher
	if ( iIsCreated == 0 && pToucher )
	{
		// if I picked them up
		if ( pToucher == pLocalPlayer )
		{
			// Offense
			if ( pCreator && pCreator->GetTeamNumber() == pLocalPlayer->GetTeamNumber() )
			{
				gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_PERSONAL_PICKUP_OFFENSE );
				gMapInfoContainer.DuckUpdateScore( iCount * DUCK_XP_WEIGHT_OFFENSE, TF_DUCK_SCORING_OVERALL_RATING );
			}
			//defense
			else if ( pCreator && pCreator->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
			{
				gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_PERSONAL_PICKUP_DEFENDED );
				gMapInfoContainer.DuckUpdateScore( iCount * DUCK_XP_WEIGHT_DEFENSE, TF_DUCK_SCORING_OVERALL_RATING );
			}

			// objective
			if ( iDuckFlags & DUCK_FLAG_OBJECTIVE )
			{
				gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_PERSONAL_PICKUP_OBJECTIVE );
				gMapInfoContainer.DuckUpdateScore( iCount* DUCK_XP_WEIGHT_OBJECTIVE, TF_DUCK_SCORING_OVERALL_RATING );
			}

			// bonus
			if ( iDuckFlags & DUCK_FLAG_BONUS )
			{
				gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_PERSONAL_BONUS_PICKUP );
				gMapInfoContainer.DuckUpdateScore( iCount* DUCK_XP_WEIGHT_BONUS, TF_DUCK_SCORING_OVERALL_RATING );
			}
		}
		// Teammate picks up a duck I made
		else if ( pCreator && pCreator == pLocalPlayer && pCreator->GetTeamNumber() == pToucher->GetTeamNumber() )
		{
			gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_TEAM_PICKUP_MY_DUCKS );
			gMapInfoContainer.DuckUpdateScore( iCount * DUCK_XP_WEIGHT_TEAMMATE, TF_DUCK_SCORING_OVERALL_RATING );
		}
	}
	// Duck Created
	else if ( iIsCreated != 0 )
	{
		// If this is the same as local player
		if ( pLocalPlayer == pCreator )
		{
			gMapInfoContainer.DuckUpdateScore( iCount, TF_DUCK_SCORING_PERSONAL_GENERATION );
			gMapInfoContainer.DuckUpdateScore( iCount * DUCK_XP_WEIGHT_GENERATION, TF_DUCK_SCORING_OVERALL_RATING );
		}
	}
}
#endif // CLIENT
//-----------------------------------------------------------------------------

void Leaderboards_Refresh()
{
	gMapInfoContainer.DownloadDuelLeaderboard();
	gMapInfoContainer.DownloadDuckLeaderboard();
	gMapInfoContainer.DownloadLadderLeaderboard();
}

void MapInfo_RefreshLeaderboard( const char *pMapName )
{
	CLeaderboardInfo *pInfo = FindMapInfo( pMapName );
	if ( pInfo )
	{
		pInfo->DownloadLeaderboardData();
	}
}

bool MapInfo_GetLeaderboardInfo( const char *pMapName, CUtlVector< LeaderboardEntry_t* > &scores, int &iNumLeaderboardEntries, uint32 unMinScores )
{
	CLeaderboardInfo *pInfo = FindMapInfo( pMapName );
	if ( pInfo && pInfo->findLeaderboardResults.m_bLeaderboardFound )
	{
		if ( (uint32)pInfo->downloadedLeaderboardScoresFriends.Count() >= unMinScores )
		{
			scores = pInfo->downloadedLeaderboardScoresFriends;
		}
		else if ( (uint32)pInfo->downloadedLeaderboardScoresGlobalAroundUser.Count() >= unMinScores )
		{
			scores = pInfo->downloadedLeaderboardScoresGlobalAroundUser;
		}
		else
		{
			scores = pInfo->downloadedLeaderboardScoresGlobal;
		}
		iNumLeaderboardEntries = pInfo->iNumLeaderboardEntries;
		return true;
	}
	return false;
}

static const char *FindMapNameForContributionDefinitionIndex( item_definition_index_t unContribDefIndex )
{
	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( pMapDef->mapStampDef && pMapDef->mapStampDef->GetDefinitionIndex() == unContribDefIndex )
			return pMapDef->pszMapName;
	}
	return NULL;
}

bool MapInfo_DidPlayerDonate( uint32 unAccountID, const char *pLevelName )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return false;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CSteamID steamID = localSteamID;
	steamID.SetAccountID( unAccountID );

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache == NULL )
		return false;

	GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CTFMapContribution::k_nTypeID );
	if ( pTypeCache == NULL )
		return false;

	char pchBaseMapName[ MAX_PATH ];
	Q_FileBase( pLevelName, pchBaseMapName, sizeof(pchBaseMapName) );

	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CTFMapContribution *pMapContribution = (CTFMapContribution*)( pTypeCache->GetObject( i ) );

		const char *pszMapName = FindMapNameForContributionDefinitionIndex( pMapContribution->Obj().def_index() );
		if ( pszMapName && FStrEq( pszMapName, pchBaseMapName ) )
			return true;
	}
	return false;
}

int MapInfo_GetDonationAmount( uint32 unAccountID, const char *pLevelName )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return 0;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CSteamID steamID = localSteamID;
	steamID.SetAccountID( unAccountID );

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache == NULL )
		return 0;

	GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CTFMapContribution::k_nTypeID );
	if ( pTypeCache == NULL )
		return 0;

	char pchBaseMapName[ MAX_PATH ];
	Q_FileBase( pLevelName, pchBaseMapName, sizeof(pchBaseMapName) );

	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CTFMapContribution *pMapContribution = (CTFMapContribution*)( pTypeCache->GetObject( i ) );

		const char *pszMapName = FindMapNameForContributionDefinitionIndex( pMapContribution->Obj().def_index() );
		if ( pszMapName && FStrEq( pszMapName, pchBaseMapName ) )
			return pMapContribution->Obj().contribution_level();
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Ladders
//-----------------------------------------------------------------------------
bool Leaderboards_GetLadderLeaderboard( CUtlVector< LeaderboardEntry_t* > &scores, const char *pszName, bool bGlobal )
{
	CLeaderboardInfo *pLeaderboard = gMapInfoContainer.GetLadderLeaderboard( pszName );
	if ( pLeaderboard && pLeaderboard->IsLeaderboardFound() )
	{
		scores = bGlobal ? pLeaderboard->downloadedLeaderboardScoresGlobal : pLeaderboard->downloadedLeaderboardScoresFriends;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void Leaderboards_LadderRefresh( void )
{
	gMapInfoContainer.DownloadLadderLeaderboard();
}
