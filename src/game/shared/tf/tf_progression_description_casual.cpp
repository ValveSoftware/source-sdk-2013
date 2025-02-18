//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_progression_description_casual.h"
#include "tf_rating_data.h"

#ifdef CLIENT_DLL
	#include "basemodel_panel.h"
	#include "animation.h"
	#include "tf_item_inventory.h"
#endif

#ifdef CLIENT_DLL
extern ConVar tf_test_pvp_rank_xp_change;
#endif

CCasualProgressionDesc::CCasualProgressionDesc()
	: IProgressionDesc( "models/vgui/12v12_badge.mdl"
							, "resource/ui/PvPCasualRankPanel.res"
							, "TF_Competitive_Level" )
{
	struct StepInfo_t
	{
		float m_flAvgGamerPerLevel;
		const char* m_pszLevelUpSound;
	};

	const StepInfo_t stepInfo[] =	{ { 1.5f,	"MatchMaking.LevelOneAchieved" }
		, { 2.5f,	"MatchMaking.LevelTwoAchieved" }
		, { 4.f,	"MatchMaking.LevelThreeAchieved" }
		, { 6.f,	"MatchMaking.LevelFourAchieved" }
		, { 9.f,	"MatchMaking.LevelFiveAchieved" }
		, { 14.f,	"MatchMaking.LevelSixAchieved" } };

	uint32 nNumLevels = m_nLevelsPerStep * m_nSteps * m_nPrestigeLevels;
	const uint32 nPrestigeSize = GetLevelsPerPrestige();

	uint32 nEndXPForLevel = 0;
	for( uint32 i=0; i<nNumLevels; ++i )
	{
		const uint32 nStep = ( i % nPrestigeSize ) / m_nLevelsPerStep;
		const uint32 nPrestige = i / nPrestigeSize;
		LevelInfo_t& level = m_vecLevels[ m_vecLevels.AddToTail() ];
		level.m_nDisplayLevel = ( i % nPrestigeSize ) + 1;	// This loop is 0-based, but users will start at level 1
		level.m_nLevelNum = i + 1;	// This loop is 0-based, but users will start at level 1
		level.m_nStartXP = nEndXPForLevel; // Use the previous level's end as our start

											// We want the last level to have the same starting and ending XP value so the progress bar looks filled
											// as soon as you hit max level.
		if ( level.m_nLevelNum != nNumLevels )
		{

			float flStepXP = stepInfo[ nStep ].m_flAvgGamerPerLevel;

			nEndXPForLevel += flStepXP * m_nAverageXPPerGame;
		}

		level.m_nEndXP = nEndXPForLevel;
		level.m_pszLevelTitle = CFmtStr( "#TF_Casual_Rank_%d", nPrestige );
		level.m_pszLevelUpSound = stepInfo[ nStep ].m_pszLevelUpSound;
		level.m_pszLobbyBackgroundImage = "competitive/12v12_background001"; // All the same in casual
	}
}

#ifdef CLIENT_DLL
void CCasualProgressionDesc::SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const
{
	if ( !pModelPanel )
		return;

	int nLevelIndex = level.m_nLevelNum - 1;
	int nSkin = nLevelIndex / m_nLevelsPerStep;
	int nStarsBodyGroup	= ( ( nLevelIndex ) % 5 ) + 1;
	int nBulletsBodyGroup = 0;
	int nPlatesBodyGroup = 0;
	int nBannerBodyGroup = 0;

	switch( ( ( nLevelIndex ) / 5 ) % 5 )
	{
	case 0:
		nBulletsBodyGroup	= 0;
		nPlatesBodyGroup	= 0;
		nBannerBodyGroup	= 0;
		break;
	case 1:
		nBulletsBodyGroup	= 1;
		nPlatesBodyGroup	= 0;
		nBannerBodyGroup	= 0;
		break;
	case 2:
		nBulletsBodyGroup	= 2;
		nPlatesBodyGroup	= 1;
		nBannerBodyGroup	= 0;
		break;
	case 3:
		nBulletsBodyGroup	= 3;
		nPlatesBodyGroup	= 2;
		nBannerBodyGroup	= 1;
		break;
	case 4:
		nBulletsBodyGroup	= 4;
		nPlatesBodyGroup	= 3;
		nBannerBodyGroup	= 1;
		break;
	}

	EnsureBadgePanelModel( pModelPanel );

	int nBody = 0;
	CStudioHdr studioHDR( pModelPanel->GetStudioHdr(), g_pMDLCache );

	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "bullets" ), nBulletsBodyGroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "plates" ), nPlatesBodyGroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "banner" ), nBannerBodyGroup );
	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "stars" ), nStarsBodyGroup );

	int nLogoValue = 0;
	// The logo bodygroup will change depending on the campaign status
// 	if ( TFInventoryManager() )
// 	{
// 		CTFPlayerInventory *pInv = TFInventoryManager()->GetInventoryForPlayer( steamID );
// 		static CSchemaItemDefHandle pItemDef_ActivatedCampaign3Pass( "Activated Campaign 3 Pass" );
// 		if ( pInv && pInv->GetFirstItemOfItemDef( pItemDef_ActivatedCampaign3Pass->GetDefinitionIndex(), pInv ) != NULL  )
// 		{
// 			nLogoValue = 1;
// 		}
// 	}

	::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "logo" ), nLogoValue );

	pModelPanel->SetBody( nBody );
	pModelPanel->SetSkin( nSkin );
}

void CCasualProgressionDesc::GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const
{
	wchar_t wszLevel[ 16 ];
	wchar_t wszPrestige[ 16 ];
	int nPrestige = ( ( level.m_nLevelNum - 1 ) / GetLevelsPerPrestige() ) + 1;
	_snwprintf( wszLevel, ARRAYSIZE( wszLevel ), L"%d", level.m_nDisplayLevel );
	_snwprintf( wszPrestige, ARRAYSIZE( wszPrestige ), L"%d", nPrestige );
	if ( nPrestige == 1 )
	{
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#TF_Competitive_LevelTier1" );
		g_pVGuiLocalize->ConstructString( wszOutString, wszOutStringSize, wpszFormat, 1, wszLevel );
	}
	else
	{
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( m_pszLevelToken );
		g_pVGuiLocalize->ConstructString( wszOutString, wszOutStringSize, wpszFormat, 2, wszLevel, wszPrestige );
	}
}
#endif // CLIENT_DLL

