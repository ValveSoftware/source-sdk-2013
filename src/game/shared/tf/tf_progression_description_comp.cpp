//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_progression_description_comp.h"
#include "tf_rating_data.h"

#ifdef CLIENT_DLL
	#include "basemodel_panel.h"
	#include "animation.h"
#endif

#ifdef CLIENT_DLL
extern ConVar tf_test_pvp_rank_xp_change;
#endif

CDrilloProgressionDesc::CDrilloProgressionDesc()
	: IProgressionDesc( "models/vgui/competitive_badge.mdl"
						, "resource/ui/PvPCompRankPanel.res"
						, "TF_Competitive_Rank" )
{
	// Bucket 1
	m_vecLevels.AddToTail( { 1,		1,	0,								1300,					"#TF_Competitive_Rank_1",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	m_vecLevels.AddToTail( { 2,		2,	m_vecLevels.Tail().m_nEndXP,	1350,					"#TF_Competitive_Rank_2",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	m_vecLevels.AddToTail( { 3,		3,	m_vecLevels.Tail().m_nEndXP,	1400,					"#TF_Competitive_Rank_3",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	// Bucket 2
	m_vecLevels.AddToTail( { 4,		4,	m_vecLevels.Tail().m_nEndXP,	1450,					"#TF_Competitive_Rank_4",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	m_vecLevels.AddToTail( { 5,		5,	m_vecLevels.Tail().m_nEndXP,	1500,					"#TF_Competitive_Rank_5",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	m_vecLevels.AddToTail( { 6,		6,	m_vecLevels.Tail().m_nEndXP,	1550,					"#TF_Competitive_Rank_6",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	// Bucket 3
	m_vecLevels.AddToTail( { 7,		7,	m_vecLevels.Tail().m_nEndXP,	1600,					"#TF_Competitive_Rank_7",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	m_vecLevels.AddToTail( { 8,		8,	m_vecLevels.Tail().m_nEndXP,	1650,					"#TF_Competitive_Rank_8",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	m_vecLevels.AddToTail( { 9,		9,	m_vecLevels.Tail().m_nEndXP,	1700,					"#TF_Competitive_Rank_9",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	// Bucket 4
	m_vecLevels.AddToTail( { 10,	10,	m_vecLevels.Tail().m_nEndXP,	1750,					"#TF_Competitive_Rank_10", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	m_vecLevels.AddToTail( { 11,	11,	m_vecLevels.Tail().m_nEndXP,	1800,					"#TF_Competitive_Rank_11", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	m_vecLevels.AddToTail( { 12,	12,	m_vecLevels.Tail().m_nEndXP,	1850,					"#TF_Competitive_Rank_12", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	// Bucket 5
	m_vecLevels.AddToTail( { 13,	13,	m_vecLevels.Tail().m_nEndXP,	kGlicko1MaxRatingValue, "#TF_Competitive_Rank_13", "MatchMaking.RankFiveAchieved",	"competitive/comp_background_tier005a" } );
}

const LevelInfo_t& CDrilloProgressionDesc::GetLevelForRating( uint32 nExperience ) const
{
	FixmeMMRatingBackendSwapping(); // Hard-coded drillo

									// The client may not have a rating yet, in which case they see 0 until they've been in a match. For level
									// purposes, return minimum.
	return IProgressionDesc::GetLevelForRating( nExperience == 0 ? k_unDrilloRating_Ladder_Min : nExperience );
}

#ifdef CLIENT_DLL
void CDrilloProgressionDesc::GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const
{
	wchar_t wszCount[ 16 ];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", level.m_nDisplayLevel );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( m_pszLevelToken );
	g_pVGuiLocalize->ConstructString( wszOutString, wszOutStringSize, wpszFormat, 2, wszCount, g_pVGuiLocalize->Find( level.m_pszLevelTitle ) );
}

void CDrilloProgressionDesc::SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const
{
	if ( !pModelPanel )
		return;

	int nLevelIndex = level.m_nLevelNum - 1;
	int nSkin = nLevelIndex;
	int nSkullsBodygroup	= ( nLevelIndex % 6 );
	int nSparkleBodygroup	= 0;
	if ( level.m_nLevelNum == 18 ) nSparkleBodygroup = 1;
	EnsureBadgePanelModel( pModelPanel );

	int nBody = 0;
	CStudioHdr studioHDR( pModelPanel->GetStudioHdr(), g_pMDLCache );

	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "skulls" ), nSkullsBodygroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "sparkle" ), nSparkleBodygroup );

	pModelPanel->SetBody( nBody );
	pModelPanel->SetSkin( nSkin );
}
#endif // CLIENT_DLL



CGlickoProgressionDesc::CGlickoProgressionDesc()
	: IProgressionDesc( "models/vgui/competitive_badge.mdl"
						, "resource/ui/PvPCompRankPanel.res"
						, "TF_Competitive_Rank" )
{
	// Bucket 1
	m_vecLevels.AddToTail( { 1,		1,	0,								1300,					"#TF_Competitive_Rank_1",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	m_vecLevels.AddToTail( { 2,		2,	m_vecLevels.Tail().m_nEndXP,	1350,					"#TF_Competitive_Rank_2",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	m_vecLevels.AddToTail( { 3,		3,	m_vecLevels.Tail().m_nEndXP,	1400,					"#TF_Competitive_Rank_3",  "MatchMaking.RankOneAchieved",	"competitive/comp_background_tier001a" } );
	// Bucket 2
	m_vecLevels.AddToTail( { 4,		4,	m_vecLevels.Tail().m_nEndXP,	1450,					"#TF_Competitive_Rank_4",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	m_vecLevels.AddToTail( { 5,		5,	m_vecLevels.Tail().m_nEndXP,	1500,					"#TF_Competitive_Rank_5",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	m_vecLevels.AddToTail( { 6,		6,	m_vecLevels.Tail().m_nEndXP,	1550,					"#TF_Competitive_Rank_6",  "MatchMaking.RankTwoAchieved",	"competitive/comp_background_tier002a" } );
	// Bucket 3
	m_vecLevels.AddToTail( { 7,		7,	m_vecLevels.Tail().m_nEndXP,	1600,					"#TF_Competitive_Rank_7",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	m_vecLevels.AddToTail( { 8,		8,	m_vecLevels.Tail().m_nEndXP,	1650,					"#TF_Competitive_Rank_8",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	m_vecLevels.AddToTail( { 9,		9,	m_vecLevels.Tail().m_nEndXP,	1700,					"#TF_Competitive_Rank_9",  "MatchMaking.RankThreeAchieved",	"competitive/comp_background_tier003a" } );
	// Bucket 4
	m_vecLevels.AddToTail( { 10,	10,	m_vecLevels.Tail().m_nEndXP,	1750,					"#TF_Competitive_Rank_10", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	m_vecLevels.AddToTail( { 11,	11,	m_vecLevels.Tail().m_nEndXP,	1800,					"#TF_Competitive_Rank_11", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	m_vecLevels.AddToTail( { 12,	12,	m_vecLevels.Tail().m_nEndXP,	1850,					"#TF_Competitive_Rank_12", "MatchMaking.RankFourAchieved",	"competitive/comp_background_tier004a" } );
	// Bucket 5
	m_vecLevels.AddToTail( { 13,	13,	m_vecLevels.Tail().m_nEndXP,	kGlicko1MaxRatingValue, "#TF_Competitive_Rank_13", "MatchMaking.RankFiveAchieved",	"competitive/comp_background_tier005a" } );
	Assert( m_vecLevels.Count() == kGlickoRanks );
}

#ifdef CLIENT_DLL
void CGlickoProgressionDesc::GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const
{
	wchar_t wszCount[ 16 ];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", level.m_nDisplayLevel );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( m_pszLevelToken );
	g_pVGuiLocalize->ConstructString( wszOutString, wszOutStringSize, wpszFormat, 2, wszCount, g_pVGuiLocalize->Find( level.m_pszLevelTitle ) );
}

void CGlickoProgressionDesc::SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const
{
	if ( !pModelPanel )
		return;

	int nLevelIndex = level.m_nLevelNum - 1;
	int nSkin = nLevelIndex;

	int nStarBodyGroup, nSparkleBodygroup, nLaurelBodyGroup;

	// Max level has all the things turned on
	if ( level.m_nLevelNum == 13 )
	{
		nStarBodyGroup = 1;
		nSparkleBodygroup = 1;
		nLaurelBodyGroup = 1;
	}
	else
	{
		// In the standard progression, the ranks are chopped up into 4 buckets of 3
		// Each bucket gets its own skin
		// Each rank within a bucket gets |_Laurels_|_Star_|
		//						   Rank 1 |   No    |  No  |
		//						   Rank 2 |   Yes   |  No  |
		//						   Rank 3 |   Yes   |  Yes |
		nStarBodyGroup = nLevelIndex % 3 == 2 ? 1 : 0;
		nSparkleBodygroup = 0;
		nLaurelBodyGroup = nLevelIndex % 3 > 0 ? 1 : 0;
	}

	EnsureBadgePanelModel( pModelPanel );

	int nBody = 0;
	CStudioHdr studioHDR( pModelPanel->GetStudioHdr(), g_pMDLCache );

	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "star" ), bInPlacement ? 0 : nStarBodyGroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "sparkle" ), bInPlacement ? 0 : nSparkleBodygroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "laurels" ), bInPlacement ? 0 : nLaurelBodyGroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "main" ), bInPlacement ? 1 : 0 );

	pModelPanel->SetBody( nBody );
	pModelPanel->SetSkin( nSkin );
}
#endif // CLIENT_DLL

