//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_wardata.h"
#include "gcsdk/enumutils.h"
#include "schemainitutils.h"
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#endif
#ifdef GAME_DLL
	#include "tf_player.h"
#endif

using namespace GCSDK;

//-----------------------------------------------------------------------------
// Purpose: Get user's War Data by steamID and war ID.  Returns NULL if it 
//			doesnt exist or if the war is inactive and bLoadEvenIfWarInactive 
//			is false.  On the GC bLoadSOCacheIfNeeded will load the user's
//			SOCache if needed in order to try and get their war data
//-----------------------------------------------------------------------------
CWarData* GetPlayerWarData( const CSteamID& steamID, war_definition_index_t warDefIndex, bool bLoadEvenIfWarInactive
	)
{
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( warDefIndex );
	// If the war isn't active and we weren't told to ignore that, then return NULL
	if ( !pWarDef || ( !bLoadEvenIfWarInactive && !pWarDef->IsActive() ) )
		return NULL;

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );

	if ( pSOCache )
	{
		auto *pTypeCache = pSOCache->FindTypeCache( CWarData::k_nTypeID );
		if ( pTypeCache )
		{
			int nCount = pTypeCache->GetCount();
			for( int i=0; i < nCount; ++i )
			{
				CWarData* pWarData = static_cast< CWarData* >( pTypeCache->GetObject( i ) );
				if ( pWarData->Obj().war_id() == warDefIndex )
					return pWarData;
			}
		}
	}

	return NULL;
}

#ifdef CLIENT_DLL
CWarData* GetLocalPlayerWarData( war_definition_index_t warDefIndex )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		return NULL;
	}

	return GetPlayerWarData( steamapicontext->SteamUser()->GetSteamID(), warDefIndex, true );
}
#endif


CWarDefinition::CWarDefinition()
	: m_nDefIndex( INVALID_WAR_DEF_INDEX )
	, m_pszLocalizedWarname( NULL )
	, m_pszDefName( NULL )
	, m_mapSides( DefLessFunc( SidesMap_t::KeyType_t ) )
	, m_rtTimeEnd( 0 )
	, m_rtTimeStart( 0 )
{}

bool CWarDefinition::BInitFromKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors )
{
	m_nDefIndex = atoi( pKV->GetName() );
	SCHEMA_INIT_CHECK( m_nDefIndex != INVALID_WAR_DEF_INDEX, "Missing 'war_id' for war def %s", pKV->GetName() );

	m_pszDefName = pKV->GetString( "name" );
	SCHEMA_INIT_CHECK( m_nDefIndex != INVALID_WAR_DEF_INDEX, "Missing 'name' for war def %s", pKV->GetName() );

	const char *pszTime = pKV->GetString( "start_time", NULL );
	SCHEMA_INIT_CHECK( pszTime != NULL, "war definition %s does not have 'start_time'", pKV->GetName() );
	m_rtTimeStart = ( pszTime && pszTime[0] )
							? CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszTime )
							: RTime32(0);

	pszTime = pKV->GetString( "end_time", NULL );
	SCHEMA_INIT_CHECK( pszTime != NULL, "war definition %s does not have 'end_time'", pKV->GetName() );
	m_rtTimeEnd = ( pszTime && pszTime[0] )
							? CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszTime )
							: RTime32(0);
		
	KeyValues* pKVSidesBlock = pKV->FindKey( "sides" );
	FOR_EACH_TRUE_SUBKEY( pKVSidesBlock, pKVSide )
	{
		auto& side = m_mapSides[m_mapSides.Insert( atoi( pKVSide->GetName() ) ) ];
		side.BInitFromKV( pKV->GetName(), pKVSide, pVecErrors );
	}

	m_pszLocalizedWarname = pKV->GetString( "localized_name", NULL );
	SCHEMA_INIT_CHECK( m_pszLocalizedWarname != NULL, "war definition %s does not have 'localized_name'", pKV->GetName() );

	return SCHEMA_INIT_SUCCESS();
}

const CWarDefinition::CWarSideDefinition_t* CWarDefinition::GetSide( war_side_t nSide ) const
{
	if ( IsValidSide( nSide ) )
	{
		SidesMap_t::IndexType_t idx = m_mapSides.Find( nSide );
		if ( idx != m_mapSides.InvalidIndex() )
		{
			return &m_mapSides[ idx ];
		}
	}

	return NULL;
}

bool CWarDefinition::IsActive() const
{
	Assert( m_rtTimeEnd != 0 );
	Assert( m_rtTimeStart != 0 );
	if ( m_rtTimeEnd == 0 || m_rtTimeStart == 0 )
		return false;

	return CRTime::RTime32TimeCur() > m_rtTimeStart && CRTime::RTime32TimeCur() < m_rtTimeEnd;
}

bool CWarDefinition::IsValidSide( war_side_t nSide ) const
{
	FOR_EACH_MAP_FAST( m_mapSides, i )
	{
		if ( m_mapSides[i].m_nSideIndex == nSide )
			return true;
	}

	return false;
}

bool CWarDefinition::CWarSideDefinition_t::BInitFromKV( const char* pszContainingWarName, KeyValues *pKVSide, CUtlVector<CUtlString> *pVecErrors )
{
	m_nSideIndex = (war_side_t)atoi( pKVSide->GetName() );
	SCHEMA_INIT_CHECK( m_nSideIndex != INVALID_WAR_SIDE, "war definition %s has invalid side index: %d", pszContainingWarName, m_nSideIndex );

	m_pszLocalizedName = pKVSide->GetString( "localized_name", NULL );
	SCHEMA_INIT_CHECK( m_pszLocalizedName != NULL, "war definition %s side %s missing side localization name", pszContainingWarName, pKVSide->GetName() );

	m_pszLeaderboardName = pKVSide->GetString( "leaderboard_name", NULL );
	SCHEMA_INIT_CHECK( m_pszLocalizedName != NULL, "war definition %s side %s missing side leaderboard name", pszContainingWarName, pKVSide->GetName() );

	//TODO BRETT: Grab the leaderboard now?
	return SCHEMA_INIT_SUCCESS();
}

CWarData::CWarData()
{
	Obj().set_account_id( 0 );
	Obj().set_war_id( INVALID_WAR_DEF_INDEX );
	Obj().set_affiliation( INVALID_WAR_SIDE );
	Obj().set_points_scored( 0 );
}

#if defined( CLIENT_DLL ) || defined( GC )
CTFWarGlobalDataHelper::CTFWarGlobalDataHelper()
	: m_bInitialized( false )
	, m_mapWarStats( DefLessFunc( WarStatsMap_t::KeyType_t ) )
#ifdef CLIENT_DLL
	, m_flLastUpdateRequest( 0.f )
	, m_flLastUpdated( 0.f )
#endif
{
#ifdef CLIENT_DLL
	Init();
#endif
}

CGCMsgGC_War_GlobalStatsResponse* CTFWarGlobalDataHelper::FindOrCreateWarData( war_definition_index_t nWarDef, bool bCreateIfDoesntExist )
{
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( nWarDef );
	if ( !pWarDef )
		return NULL;

	if ( nWarDef == INVALID_WAR_DEF_INDEX )
		return NULL;

	CGCMsgGC_War_GlobalStatsResponse* pGlobalData = NULL;
		
	auto waridx = m_mapWarStats.Find( (war_definition_index_t)nWarDef );
	if ( waridx == m_mapWarStats.InvalidIndex() && bCreateIfDoesntExist )
	{
		waridx = m_mapWarStats.Insert( nWarDef );
		m_mapWarStats[ waridx ].set_war_id( nWarDef );
	}

	if ( waridx != m_mapWarStats.InvalidIndex() )
	{
		pGlobalData = &m_mapWarStats[ waridx ];
	}
		
	return pGlobalData;
}

CGCMsgGC_War_GlobalStatsResponse_SideScore* CTFWarGlobalDataHelper::FindOrCreateWarDataSide( war_side_t nWarSide, war_definition_index_t nWarDef, bool bCreateIfDoesntExist )
{
	// Make sure it's a valid war
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( nWarDef );
	if ( !pWarDef )
		return NULL;

	// Valid side on a valid awr
	if ( !pWarDef->IsValidSide( nWarSide ) )
		return NULL;

	// Find or create the war stats
	CGCMsgGC_War_GlobalStatsResponse* pWarStats = FindOrCreateWarData( nWarDef, bCreateIfDoesntExist );
	if ( !pWarStats )
		return NULL;

	// Find or create the side for the war
	CGCMsgGC_War_GlobalStatsResponse_SideScore* pSide = NULL;
	for( int i=0; i < pWarStats->side_scores_size(); ++i )
	{
		if ( pWarStats->side_scores( i ).side() == nWarSide )
		{
			pSide = pWarStats->mutable_side_scores( i );
			break;
		}
	}

	// Didn't already exist.  Create and initialize
	if ( pSide == NULL && bCreateIfDoesntExist )
	{
		pSide = pWarStats->add_side_scores();
		pSide->set_score( 0 );
		pSide->set_side( nWarSide );
	}

	return pSide;
}

//-----------------------------------------------------------------------------
// Purpose: On the GC, grab all records for the war and tally up the global stats
//			On the client, send a request for what the GC has
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::Init()
{

	RequestUpdateGlobalStats();
	RequestLeaderboard();
}

//-----------------------------------------------------------------------------
// Purpose: Set the stats.  We're initialized once we do this.
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::SetGlobalStats( const CGCMsgGC_War_GlobalStatsResponse& newData )
{
#ifdef CLIENT_DLL
	m_flLastUpdated = Plat_FloatTime();
#endif
	m_bInitialized = true;
	
	CGCMsgGC_War_GlobalStatsResponse* pExistingData = FindOrCreateWarData( newData.war_id(), true );
	if ( pExistingData )
	{
		pExistingData->CopyFrom( newData );
	}

#ifdef CLIENT_DLL
	IGameEvent *event = gameeventmanager->CreateEvent( "global_war_data_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}	
#endif
}

void CTFWarGlobalDataHelper::AddToSideScore( war_definition_index_t nWar, war_side_t nSide, uint32 nValue )
{
	Assert( nWar != INVALID_WAR_DEF_INDEX );
	Assert( nSide != INVALID_WAR_SIDE );
	
	CGCMsgGC_War_GlobalStatsResponse_SideScore* pSide = FindOrCreateWarDataSide( nSide, nWar, true );
	if ( pSide )
	{
		pSide->set_score( pSide->score() + nValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Grab the Spy stats.  Request an update if we're on the client and stale
//-----------------------------------------------------------------------------
uint64 CTFWarGlobalDataHelper::GetGlobalSideScore( war_definition_index_t nWar, war_side_t nSide ) 
{ 
	Assert( nWar != INVALID_WAR_DEF_INDEX );
	Assert( nSide != INVALID_WAR_SIDE );

#ifdef CLIENT_DLL
	CheckGlobalStatsStaleness();
#endif
	
	CGCMsgGC_War_GlobalStatsResponse_SideScore* pSide = FindOrCreateWarDataSide( nSide, nWar, true );
	if ( pSide )
	{
		return pSide->score();
	}

	Assert( false );
	return 0;
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Ask the GC for the global stats.
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::RequestUpdateGlobalStats()
{
	// Ask for new war stats
	GCSDK::CProtoBufMsg<CGCMsgGC_War_RequestGlobalStats> msg( k_EMsgGC_War_RequestGlobalStats );
	msg.Body().set_war_id( PYRO_VS_HEAVY_WAR_DEF_INDEX ); // TODO Brett: Get all the war data
	GCClientSystem()->BSendMessage( msg );
	m_flLastUpdateRequest = Plat_FloatTime();
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we're "stale".  If so, request new stats from the GC.
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::CheckGlobalStatsStaleness()
{
	float flTimeSinceLastRequest = Plat_FloatTime() - m_flLastUpdateRequest;
	if ( flTimeSinceLastRequest > 30.f )
	{
		RequestUpdateGlobalStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Requests the war leaderboard
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::RequestLeaderboard()
{
	//TODO BRETT loop all the wars and grab each one

	/*if ( steamapicontext && steamapicontext->SteamUserStats() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
		if ( pSOCache )
		{
			GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CWarData::k_nTypeID );
			if ( pTypeCache )
			{
				CWarData *pWarData = (CWarData*)pTypeCache->GetObject( 0 );
				if ( pWarData )
				{
					eSide = (EWarSides)pWarData->Obj().affiliation();
				}
			}
		}

		if ( eSide != k_EInvalidSide )
		{
			SteamAPICall_t apicall = steamapicontext->SteamUserStats()->FindLeaderboard( eSide == k_ESpy ? k_pszSpyVsEngyLeaderboard_Spy : k_pszSpyVsEngyLeaderboard_Engy );
			m_findLeaderboardCallback.Set( apicall, this, &CTFWarGlobalDataHelper::OnFindLeaderboard );
		}
	}*/	
}

//-----------------------------------------------------------------------------
// Purpose: Requests the war leaderboard
//-----------------------------------------------------------------------------
void CTFWarGlobalDataHelper::OnFindLeaderboard( LeaderboardFindResult_t *pResult, bool bIOFailure )
{
	m_findLeaderboardResults = *pResult;
	DownloadLeaderboard();
}

void CTFWarGlobalDataHelper::DownloadLeaderboard()
{
	if ( m_findLeaderboardResults.m_bLeaderboardFound )
	{
		// friends
		SteamAPICall_t apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( m_findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, 1, 10 );
		downloadLeaderboardCallbackFriends.Set( apicall, this, &CTFWarGlobalDataHelper::OnLeaderboardScoresDownloaded_Friends );			
		// global around user
		apicall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries( m_findLeaderboardResults.m_hSteamLeaderboard, k_ELeaderboardDataRequestGlobal, 1, 10 );
		downloadLeaderboardCallbackGlobal.Set( apicall, this, &CTFWarGlobalDataHelper::OnLeaderboardScoresDownloaded_Global );
	}
}

static void RetrieveLeaderboardEntries( LeaderboardScoresDownloaded_t &scores, CTFWarGlobalDataHelper::LeaderBoardEntries_t &entries )
{
	entries.m_bInitialized = true;
	entries.m_vecEntries.PurgeAndDeleteElements();
	entries.m_vecEntries.EnsureCapacity( scores.m_cEntryCount );
	for ( int i = 0; i < scores.m_cEntryCount; ++i )
	{
		LeaderboardEntry_t *leaderboardEntry = new LeaderboardEntry_t;
		if ( steamapicontext->SteamUserStats()->GetDownloadedLeaderboardEntry( scores.m_hSteamLeaderboardEntries, i, leaderboardEntry, NULL, 0 ) )
		{
			entries.m_vecEntries.AddToTail( leaderboardEntry );
		}
	}
}

void CTFWarGlobalDataHelper::OnLeaderboardScoresDownloaded_Global( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
{
	RetrieveLeaderboardEntries( *pResult, downloadedLeaderboardScoresGlobal );
}

void CTFWarGlobalDataHelper::OnLeaderboardScoresDownloaded_Friends( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure )
{
	RetrieveLeaderboardEntries( *pResult, downloadedLeaderboardScoresFriends );
}

class CGC_War_GlobalStatsResponse : public GCSDK::CGCClientJob
{
public:
	CGC_War_GlobalStatsResponse( GCSDK::CGCClient *pGCClient ) : GCSDK::CGCClientJob( pGCClient ) { }

	virtual bool BYieldingRunJobFromMsg( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CGCMsgGC_War_GlobalStatsResponse > msg( pNetPacket );

		GetWarData().SetGlobalStats( msg.Body() );

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_War_GlobalStatsResponse, "CGC_War_GlobalStatsResponse", k_EMsgGC_War_GlobalStatsResponse, GCSDK::k_EServerTypeGCClient );
#endif

CTFWarGlobalDataHelper& GetWarData()
{
	static CTFWarGlobalDataHelper s_WarData;
	return s_WarData;
}

#endif // defined( CLIENT_DLL ) || defined( GC )
