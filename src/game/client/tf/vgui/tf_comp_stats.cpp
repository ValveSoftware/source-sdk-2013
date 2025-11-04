//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_party.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_mapinfo.h"
#include "tf_comp_stats.h"
#include "vgui/ISystem.h"
#include "tf_streams.h"
#include "tf_item_inventory.h"
#include "vgui_avatarimage.h"
#include "tf_badge_panel.h"
#include "tf_ladder_data.h"
#include "tf_match_description.h"
#include "tf_rating_data.h"

#include "tf_gc_client.h"
#include "tf_partyclient.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CCompStatsPanel;

#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>


class CMatchHistoryEntryPanel : public CExpandablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CMatchHistoryEntryPanel, CExpandablePanel );
	CMatchHistoryEntryPanel( Panel* pParent, const char *pszPanelname )
		: BaseClass( pParent, pszPanelname )
	{}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/MatchHistoryEntryPanel.res" );
	}

	void SetMatchData( const CSOTFMatchResultPlayerStats& stats )
	{
		EditablePanel* pContainer = FindControl< EditablePanel >( "Container" );
		if ( !pContainer )
			return;

		// Match date
		CRTime matchdate( stats.endtime() );
		char rtime_buf[k_RTimeRenderBufferSize];
		matchdate.Render( rtime_buf );
		pContainer->SetDialogVariable( "match_date", rtime_buf );

		// Map name
		const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByIndex( stats.map_index() );
		const char* pszMapToken = "#TF_Map_Unknown";
		if ( pMapDef )
		{
			pszMapToken = pMapDef->pszMapNameLocKey;
		}
		pContainer->SetDialogVariable( "map_name", g_pVGuiLocalize->Find( pszMapToken ) );

		// KD ratio
		float flKDRatio = stats.kills();
		if ( stats.deaths() > 0 )
		{
			flKDRatio /= (float)stats.deaths();
		}
		pContainer->SetDialogVariable( "kd_ratio", CFmtStr( "%.1f", flKDRatio ) );

		// Newer match stats contain the winning team.  Old ones do not.  If the team is specified,
		// then use that to determine whether or not to show "WIN" or "LOSS"
		if ( stats.winning_team() == TF_TEAM_RED || stats.winning_team() == TF_TEAM_BLUE )
		{
			bool bWin = stats.winning_team() == stats.team();;
			pContainer->SetControlVisible( "WinLabel", bWin );
			pContainer->SetControlVisible( "LossLabel", !bWin );
		}
		else
		{
			// Old match stats don't contain winning team, so we need to try to infer from our
			// rating change whether or not we won.  A no-rating change results in neither label
			// being shown.
			pContainer->SetControlVisible( "WinLabel", stats.display_rating_change() > 0 );
			pContainer->SetControlVisible( "LossLabel", stats.display_rating_change() < 0 );
		}

		EditablePanel* pStatsContainer = FindControl< EditablePanel >( "SlidingStatsContainer", true );
		if ( pStatsContainer )
		{
			pStatsContainer->SetDialogVariable( "stat_kills", LocalizeNumberWithToken( "TF_Competitive_Kills", stats.kills() ) );
			pStatsContainer->SetDialogVariable( "stat_deaths", LocalizeNumberWithToken( "TF_Competitive_Deaths", stats.deaths() ) );
			pStatsContainer->SetDialogVariable( "stat_damage", LocalizeNumberWithToken( "TF_Competitive_Damage", stats.damage() ) );
			pStatsContainer->SetDialogVariable( "stat_healing", LocalizeNumberWithToken( "TF_Competitive_Healing", stats.healing() ) );
			pStatsContainer->SetDialogVariable( "stat_support", LocalizeNumberWithToken( "TF_Competitive_Support", stats.support() ) );
			pStatsContainer->SetDialogVariable( "stat_score", LocalizeNumberWithToken( "TF_Competitive_Score", stats.score() ) );

			ScalableImagePanel* pMapImage = pStatsContainer->FindControl< ScalableImagePanel >( "BGImage", true );
			if ( pMapImage && pMapDef )
			{
				char imagename[ 512 ];
				Q_snprintf( imagename, sizeof( imagename ), "..\\vgui\\maps\\menu_thumb_%s", pMapDef->pszMapName );
				pMapImage->SetImage( imagename );
			}

			// Lambdas, wherefore art thou...
			SetupClassIcon( pStatsContainer, "scout", TF_CLASS_SCOUT, stats );
			SetupClassIcon( pStatsContainer, "soldier", TF_CLASS_SOLDIER, stats );
			SetupClassIcon( pStatsContainer, "pyro", TF_CLASS_PYRO, stats );
			SetupClassIcon( pStatsContainer, "demo", TF_CLASS_DEMOMAN, stats );
			SetupClassIcon( pStatsContainer, "heavy", TF_CLASS_HEAVYWEAPONS, stats );
			SetupClassIcon( pStatsContainer, "engineer", TF_CLASS_ENGINEER, stats );
			SetupClassIcon( pStatsContainer, "medic", TF_CLASS_MEDIC, stats );
			SetupClassIcon( pStatsContainer, "sniper", TF_CLASS_SNIPER, stats );
			SetupClassIcon( pStatsContainer, "spy", TF_CLASS_SPY, stats );

			SetupMedalForStat( pStatsContainer, stats.kills_medal(), "kills" );
			SetupMedalForStat( pStatsContainer, stats.damage_medal(), "damage" );
			SetupMedalForStat( pStatsContainer, stats.healing_medal(), "healing" );
			SetupMedalForStat( pStatsContainer, stats.support_medal(), "support" );
			SetupMedalForStat( pStatsContainer, stats.score_medal(), "score" );
		}
	}
private:

	void SetupMedalForStat( EditablePanel* pParent, int nStat, const char* pszStatName )
	{
		ScalableImagePanel* pMedalImage = pParent->FindControl< ScalableImagePanel >( CFmtStr( "%sMedal", pszStatName ) );
		if ( pMedalImage )
		{
			if ( nStat != StatMedal_None )
			{
				pMedalImage->SetImage( g_pszCompetitiveMedalImages[ nStat ] );
			}
			pMedalImage->SetVisible( nStat != StatMedal_None );
		}
	}

	void SetupClassIcon( EditablePanel* pParent, const char* pszClassName, int nBit, const CSOTFMatchResultPlayerStats& stats )
	{
		ScalableImagePanel* pClassIcon = pParent->FindControl< ScalableImagePanel >( CFmtStr( "%sIcon", pszClassName ), true );
		if( pClassIcon )
		{ 
			pClassIcon->SetImage( stats.classes_played() & (1<<nBit) ? CFmtStr( "class_icons/filter_%s_on", pszClassName ) : CFmtStr( "class_icons/filter_%s", pszClassName ) );
		} 
	}
};

DECLARE_BUILD_FACTORY( CMatchHistoryEntryPanel );

extern Color s_colorChallengeHeader;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCompStatsPanel::CCompStatsPanel( vgui::Panel *pParent, const char* pszName ) 
	: BaseClass( pParent, pszName )
	, m_pMatchHistoryScroller( NULL )
	, m_eMatchSortMethod( SORT_BY_DATE )
	, m_bDescendingMatchHistorySort( true )
{
	m_flCompetitiveRankProgress = -1.f;
	m_flCompetitiveRankPrevProgress = -1.f;
	m_flRefreshPlayerListTime = -1.f;
	m_bCompetitiveRankChangePlayedSound = false;
	m_bMatchHistoryLoaded = false;
	m_bMatchDataForLocalPlayerDirty = true;

	ListenForGameEvent( "gc_new_session" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCompStatsPanel::~CCompStatsPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::OnCommand( const char *command )
{

	if ( FStrEq( command, "medals_help" ) )
	{
		CExplanationPopup *pPopup = dynamic_cast< CExplanationPopup* >( GetParent()->FindChildByName( "MedalsHelp" ) );
		if ( pPopup )
		{
			pPopup->Popup();
		}
		return;
	}
	else if ( FStrEq( command, "show_leaderboards" ) )
	{		 
		SetControlVisible( "MatchHistoryCategories", false, true );
		SetControlVisible( "MatchHistoryContainer", false, true );

		return;
	}
	else if ( FStrEq( command, "show_match_history" ) )
	{
		m_bMatchDataForLocalPlayerDirty = true;

		SetControlVisible( "MatchHistoryCategories", true, true );
		SetControlVisible( "MatchHistoryContainer", true, true );
		
		return;
	}
	else if ( !Q_strncmp( "sort", command, 4 ) )
	{
		EMatchHistorySortMethods_t eNewMethod = (EMatchHistorySortMethods_t)atoi( command + 4 );

		if ( eNewMethod == m_eMatchSortMethod )
		{
			m_bDescendingMatchHistorySort = !m_bDescendingMatchHistorySort;
		}
		else
		{
			m_eMatchSortMethod = eNewMethod;
			m_bDescendingMatchHistorySort = true;
		}

		m_bMatchDataForLocalPlayerDirty = true;

		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::WriteControls()
{
	++m_iWritingPanel;

	bool bInQueue = GTFPartyClient()->BInAnyMatchQueue();
	SetControlVisible( "ScrollableContainer", !bInQueue );

	--m_iWritingPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_bMatchDataForLocalPlayerDirty )
	{
		UpdateMatchDataForLocalPlayer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventname = event->GetName();
	if ( !Q_stricmp( pszEventname, "gc_new_session" ) )
	{
		// This is loaded on demand by the GC - if we have a new session, we need to re-request
		if ( m_bMatchHistoryLoaded )
		{
			m_bMatchHistoryLoaded = false;
			m_bMatchDataForLocalPlayerDirty = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() != CSOTFMatchResultPlayerInfo::k_nTypeID )
		return;

	m_bMatchDataForLocalPlayerDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() != CSOTFMatchResultPlayerInfo::k_nTypeID )
		return;

	m_bMatchDataForLocalPlayerDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_bMatchDataForLocalPlayerDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompStatsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/CompStats.res" );

	EditablePanel* pPlaylistBGPanel = FindControl< EditablePanel >( "PlaylistBGPanel", true );

	m_pMatchHistoryScroller = pPlaylistBGPanel->FindControl< CScrollableList >( "MatchHistoryContainer" );
}

static int SortResult( const CSOTFMatchResultPlayerStats & a, const CSOTFMatchResultPlayerStats & b )
{
	return a.display_rating_change() < b.display_rating_change();
}

static int SortDate( const CSOTFMatchResultPlayerStats & a, const CSOTFMatchResultPlayerStats & b )
{
	return a.endtime() < b.endtime();
}

static int SortMap( const CSOTFMatchResultPlayerStats & a, const CSOTFMatchResultPlayerStats & b )
{
	const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByIndex( a.map_index() );
	const wchar_t* pszAMapName =  g_pVGuiLocalize->Find( pMapDef ? pMapDef->pszMapNameLocKey : "#TF_Map_Unknown" );

	pMapDef = GetItemSchema()->GetMasterMapDefByIndex( b.map_index() );
	const wchar_t* pszBMapName = g_pVGuiLocalize->Find( pMapDef ? pMapDef->pszMapNameLocKey : "#TF_Map_Unknown" );

	return wcscoll( pszAMapName, pszBMapName );
}

static int SortKDR( const CSOTFMatchResultPlayerStats & a, const CSOTFMatchResultPlayerStats & b )
{
	float flAKDRatio = a.kills();
	if ( a.deaths() > 0 )
	{
		flAKDRatio /= (float)a.deaths();
	}

	float flBKDRatio = b.kills();
	if ( b.deaths() > 0 )
	{
		flBKDRatio /= (float)b.deaths();
	}

	return ( flBKDRatio - flAKDRatio ) * 1000.f;
}

void CCompStatsPanel::UpdateMatchDataForLocalPlayer()
{
	m_bMatchDataForLocalPlayerDirty = false;

	CUtlVector < CSOTFMatchResultPlayerStats > vecMatches;
	GetLocalPlayerMatchHistory( k_eTFMatchGroup_Ladder_6v6, vecMatches );

	if ( !m_bMatchHistoryLoaded )
	{
		GCSDK::CProtoBufMsg< CMsgGCMatchHistoryLoad > msg( k_EMsgGCMatchHistoryLoad );
		msg.Body().set_match_group( k_eTFMatchGroup_Ladder_6v6 );
		GCClientSystem()->BSendMessage( msg );

		m_bMatchHistoryLoaded = true;
	}

	if ( m_pMatchHistoryScroller )
	{
		m_pMatchHistoryScroller->ClearAutoLayoutPanels();

		typedef int (*MatchSortFunc)( const CSOTFMatchResultPlayerStats & a, const CSOTFMatchResultPlayerStats & b );

		struct SortMethodData_t
		{
			MatchSortFunc m_pfnSort;
			CExButton* m_pSortButton;
		};

		EditablePanel* pPlaylistBGPanel = FindControl< EditablePanel >( "PlaylistBGPanel", true );
		Assert( pPlaylistBGPanel );
		if ( !pPlaylistBGPanel )
			return;

		SortMethodData_t sortMethods[ NUM_SORT_METHODS ] = { { &SortResult, pPlaylistBGPanel->FindControl< CExButton >( "ResultButton", true ) }
														   , { &SortDate, pPlaylistBGPanel->FindControl< CExButton >( "DateButton", true ) }
														   , { &SortMap, pPlaylistBGPanel->FindControl< CExButton >( "MapButton", true ) }
														   , { &SortKDR, pPlaylistBGPanel->FindControl< CExButton >( "KDRButton", true ) } };

		// Sort
		std::sort( vecMatches.begin(), vecMatches.end(), sortMethods[ m_eMatchSortMethod ].m_pfnSort );

		Label* pSortArrow = pPlaylistBGPanel->FindControl< Label >( "SortArrow", true );
		Assert( pSortArrow );
		if ( !pSortArrow )
			return;

		// Update controls
		for( int i=0; i<ARRAYSIZE( sortMethods ); ++i )
		{
			if( sortMethods[ i ].m_pSortButton )
			{
				bool bSelected = i == m_eMatchSortMethod;
				sortMethods[ i ].m_pSortButton->SetSelected( bSelected );

				if ( bSelected )
				{
					int nDummy, nX;
					sortMethods[ i ].m_pSortButton->GetContentSize( nX, nDummy );
					// Move the sort arrow to the right edge of the selected panel
					pSortArrow->SetPos( sortMethods[ i ].m_pSortButton->GetXPos() + nX , pSortArrow->GetYPos() );
					// Fixup the label to be an up or down arrow
					pSortArrow->SetText( m_bDescendingMatchHistorySort ? L"6" : L"5" );
				}
			}
		}

		// Potentially go backwards
		if ( !m_bDescendingMatchHistorySort )
		{
			FOR_EACH_VEC( vecMatches, i )
			{
				CMatchHistoryEntryPanel* pMatchEntryPanel = new CMatchHistoryEntryPanel( m_pMatchHistoryScroller, "MatchEntry" );
				pMatchEntryPanel->MakeReadyForUse();
				pMatchEntryPanel->SetMatchData( vecMatches[ i ] );
				m_pMatchHistoryScroller->AddPanel( pMatchEntryPanel, 5 );
			}
		}
		else
		{
			FOR_EACH_VEC_BACK( vecMatches, i )
			{
				CMatchHistoryEntryPanel* pMatchEntryPanel = new CMatchHistoryEntryPanel( m_pMatchHistoryScroller, "MatchEntry" );
				pMatchEntryPanel->MakeReadyForUse();
				pMatchEntryPanel->SetMatchData( vecMatches[ i ] );
				m_pMatchHistoryScroller->AddPanel( pMatchEntryPanel, 5 );
			}
		}

		m_pMatchHistoryScroller->MakeReadyForUse();
	}
}
