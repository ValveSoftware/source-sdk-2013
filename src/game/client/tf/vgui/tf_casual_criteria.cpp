//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_party.h"
#include "tf_casual_criteria.h"
#include "tf_controls.h"
#include <vgui_controls/AnimationController.h>
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_matchmaking_dashboard.h"
#include "vgui_controls/ProgressBar.h"
#include "tf_gc_client.h"
#include "clientmode_tf.h"
#include "tf_partyclient.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "checksum_crc.h"
#include "rtime.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern Color s_colorChallengeForegroundEnabled;
extern Color s_colorChallengeForegroundDisabled;
extern Color s_colorChallengeHeader;

extern const char *s_pszMatchGroups[];

ConVar tf_show_maps_details_explanation_session( "tf_show_maps_details_explanation_session", "0", FCVAR_HIDDEN );
ConVar tf_show_maps_details_explanation_count( "tf_show_maps_details_explanation_count", "2", FCVAR_ARCHIVE | FCVAR_HIDDEN );

using namespace vgui;

// Rotation resets Monday at 19:00 UTC.
static const uint32 k_unFeaturedEpoch = 1704135600;
static const uint32 k_unFeaturedRotationSeconds = 7 * 24 * 60 * 60;
static const int k_nFeaturedMapCount = 6;
static const int k_nFeaturedMaxPerCategory = 2;
static const int k_nFeaturedSeasonalCount = 3;

struct FeaturedCandidate_t
{
	const MapDef_t* m_pMap;
	EGameCategory m_eCategory;
	bool m_bSeasonal;
	uint32 m_unScore;
};

static uint32 GetFeaturedRotation( void )
{
	uint32 unNow = steamapicontext && steamapicontext->SteamUtils() ? steamapicontext->SteamUtils()->GetServerRealTime() : CRTime::RTime32TimeCur();
	return unNow > k_unFeaturedEpoch ? ( unNow - k_unFeaturedEpoch ) / k_unFeaturedRotationSeconds : 0;
}

static void GetFeaturedMaps( CUtlVector< const MapDef_t* >& vecMaps )
{
	uint32 unRotation = GetFeaturedRotation();

	const SchemaGameCategory_t* pStockFeatured = GetItemSchema()->GetGameCategory( kGameCategory_Featured );

	CUtlVector< FeaturedCandidate_t > vecCandidates;
	const MMGroupMap_t& mapMMGroups = GetItemSchema()->GetMMGroupMap();
	FOR_EACH_MAP( mapMMGroups, i )
	{
		const SchemaMMGroup_t* pGroup = mapMMGroups[ i ];
		bool bSeasonalGroup = pGroup->m_eMMGroup == kMatchmakingType_SpecialEvents;
		if ( ( pGroup->m_eMMGroup != kMatchmakingType_Core && pGroup->m_eMMGroup != kMatchmakingType_Alternative && !bSeasonalGroup ) || !pGroup->m_bitsValidMMGroups.IsBitSet( k_eTFMatchGroup_Casual_12v12 ) )
			continue;

		FOR_EACH_VEC( pGroup->m_vecModes, j )
		{
			const SchemaGameCategory_t* pCategory = pGroup->m_vecModes[ j ];
			if ( !pCategory->PassesRestrictions() || ( bSeasonalGroup && pCategory->m_eGameCategory != kGameCategory_Halloween && pCategory->m_eGameCategory != kGameCategory_Christmas ) )
				continue;

			FOR_EACH_VEC( pCategory->m_vecEnabledMaps, k )
			{
				const MapDef_t* pMap = pCategory->m_vecEnabledMaps[ k ];
				if ( bSeasonalGroup && pStockFeatured && pStockFeatured->m_vecMaps.HasElement( pMap ) )
					continue;

				bool bDuplicate = false;
				FOR_EACH_VEC( vecCandidates, c )
				{
					bDuplicate |= vecCandidates[ c ].m_pMap == pMap;
				}
				if ( bDuplicate )
					continue;

				CRC32_t crc;
				CRC32_Init( &crc );
				CRC32_ProcessBuffer( &crc, &unRotation, sizeof( unRotation ) );
				CRC32_ProcessBuffer( &crc, &pMap->m_nDefIndex, sizeof( pMap->m_nDefIndex ) );
				CRC32_Final( &crc );

				FeaturedCandidate_t candidate = { pMap, pCategory->m_eGameCategory, bSeasonalGroup, crc };
				vecCandidates.AddToTail( candidate );
			}
		}
	}

	vecCandidates.SortPredicate( []( const FeaturedCandidate_t& left, const FeaturedCandidate_t& right ) -> bool
	{
		if ( left.m_unScore != right.m_unScore )
			return left.m_unScore < right.m_unScore;

		return left.m_pMap->m_nDefIndex < right.m_pMap->m_nDefIndex;
	} );

	int anCategoryCount[ eNumGameCategories ] = { 0 };
	int nSeasonalCount = 0;
	for ( int nPass = 0; nPass < 3; ++nPass )
	{
		FOR_EACH_VEC( vecCandidates, i )
		{
			if ( vecMaps.Count() >= k_nFeaturedMapCount )
				return;

			const FeaturedCandidate_t& candidate = vecCandidates[ i ];
			if ( vecMaps.HasElement( candidate.m_pMap ) )
				continue;

			if ( candidate.m_bSeasonal && nSeasonalCount >= k_nFeaturedSeasonalCount )
				continue;

			if ( nPass == 0 && !candidate.m_bSeasonal )
				continue;

			if ( nPass == 1 && ( candidate.m_bSeasonal || anCategoryCount[ candidate.m_eCategory ] >= k_nFeaturedMaxPerCategory ) )
				continue;

			vecMaps.AddToTail( candidate.m_pMap );
			++anCategoryCount[ candidate.m_eCategory ];
			nSeasonalCount += candidate.m_bSeasonal;
		}
	}
}

class CCasualCategory : public CExpandablePanel
{
	DECLARE_CLASS_SIMPLE( CCasualCategory, CExpandablePanel );

public:
	CCasualCategory( Panel *parent, const char *panelName, EGameCategory eCategory, Panel* pSignalHandler ) 
		: BaseClass( parent, panelName )
		, m_eCategory( eCategory )
		, m_unFeaturedRotation( 0 )
		, m_flNextFeaturedCheck( 0 )
		, pToggleButton( NULL )
		, m_mapMapPanels( DefLessFunc( uint32 ) )
		, m_pSignalHandler( pSignalHandler )
	{}

	~CCasualCategory()
	{
		// Clear out the old map entries
		ClearMapEntries();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/MatchmakingCategoryPanel.res" );

		bool bFeatured = m_eCategory == eNumGameCategories;
		const SchemaGameCategory_t* pCategory = bFeatured ? NULL : GetItemSchema()->GetGameCategory( m_eCategory );
		Assert( bFeatured || pCategory );
		if ( !bFeatured && !pCategory )
			return;

		EditablePanel* pTopContainer = FindControl< EditablePanel >( "TopContainer", true );
		if ( pTopContainer )
		{
			// Set our dialog variables
			pTopContainer->SetDialogVariable( "title_token", g_pVGuiLocalize->Find( bFeatured ? "#TF_Casual_Featured" : pCategory->m_pszLocalizedName ) );
			pTopContainer->SetDialogVariable( "desc_token", g_pVGuiLocalize->Find( bFeatured ? "#TF_Casual_Featured_Desc" : pCategory->m_pszLocalizedDesc ) );
		}

		ImagePanel* pImagePanel = FindControl< ImagePanel >( "BGImage", true );
		if ( pImagePanel && pCategory && pCategory->m_pszListImage )
		{
			pImagePanel->SetImage( pCategory->m_pszListImage );
		}
		else if ( pImagePanel && bFeatured )
		{
			pImagePanel->SetImage( "casual/gametype_featured_campaign3" );
		}

		// Clear out the old map entries
		ClearMapEntries();

		EditablePanel* pMapsContainer = FindControl< EditablePanel >( "MapsContainer", true );

		if ( pMapsContainer )
		{
			int nYPos = 16;

			// Sort the maps alphabetically
			CUtlVector< const MapDef_t* > vecSortedMaps;
			if ( bFeatured )
			{
				m_unFeaturedRotation = GetFeaturedRotation();
				m_vecFeaturedMaps.Purge();
				GetFeaturedMaps( m_vecFeaturedMaps );
				vecSortedMaps.AddVectorToTail( m_vecFeaturedMaps );
			}
			else
			{
				vecSortedMaps.AddVectorToTail( pCategory->m_vecEnabledMaps );
			}
			vecSortedMaps.SortPredicate( []( const MapDef_t* pLeft, const MapDef_t* pRight ) -> bool
			{
				// Localized map name first
				int nLocaleCmp = wcscoll( g_pVGuiLocalize->Find( pLeft->pszMapNameLocKey ), g_pVGuiLocalize->Find( pRight->pszMapNameLocKey ) );
				if ( nLocaleCmp != 0 )
					return nLocaleCmp < 0;

				// Map file name second
				int nStrCmp = V_strcasecmp( pLeft->pszMapName, pRight->pszMapName );
				if ( nStrCmp != 0 )
					return nStrCmp < 0;

				// Defindex third
				return pLeft->m_nDefIndex < pRight->m_nDefIndex;
			} );

			FOR_EACH_VEC( vecSortedMaps, i )
			{
				const MapDef_t* pMap = vecSortedMaps[ i ];

				// Load control settings
				EditablePanel* pMapEntry = new EditablePanel( pMapsContainer, "MatchmakingCategoryMapPanel" );
				pMapEntry->LoadControlSettings( "resource/ui/MatchmakingCategoryMapPanel.res" );
				pMapEntry->SetAutoDelete( false );

				CExCheckButton* pCheckButton = pMapEntry->FindControl< CExCheckButton >( "MapCheckbutton" );
				if ( pCheckButton )
				{
					KeyValues* pKVData = new KeyValues( "data" );
					pKVData->SetInt( "map_index", pMap->m_nDefIndex );
					pCheckButton->SetData( pKVData );
					pCheckButton->AddActionSignalTarget( m_pSignalHandler );
				}

				// Add to map so we can look it up
				m_mapMapPanels.Insert( pMap->m_nDefIndex, pMapEntry );

				// Update label
				pMapEntry->SetDialogVariable( "title_token", g_pVGuiLocalize->Find( pMap->pszMapNameLocKey ) );

				bool bOdd = i % 2 == 1;
				int nXPos = bOdd ? GetWide() * 0.5f : 0;
				pMapEntry->SetPos( nXPos, nYPos );
				
				nYPos += bOdd || i == vecSortedMaps.Count() - 1 ? pMapEntry->GetTall() : 0;
			}

			pMapsContainer->SetTall( nYPos + 10 );
			pMapsContainer->SetAutoResize( PIN_BOTTOMRIGHT, Panel::AUTORESIZE_NO, 0, 0, 0, 0 );
			
			// We want to be able to expand to this height
			m_nExpandedHeight = nYPos + m_nCollapsedHeight;
		}

		// Snag the button for later
		pToggleButton = FindControl< CExImageButton >( "EntryToggleButton", true );
	}

	virtual void OnToggleCollapse( bool bIsExpanded ) OVERRIDE
	{
		if ( bIsExpanded )
		{
			PostActionSignal( new KeyValues( "CategoryExpanded", "category", m_eCategory ) );
		}

		BaseClass::OnToggleCollapse( bIsExpanded );
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		SetControlVisible( "EntryToggleButtonCollapsed", !BIsExpanded(), true );
		SetControlVisible( "EntryToggleButtonExpanded", BIsExpanded(), true );

		if ( pToggleButton )
		{
			pToggleButton->SetImageArmed( BIsExpanded() ? "glyph_collapse" : "glyph_expand" );
			pToggleButton->SetImageDefault( BIsExpanded() ? "glyph_collapse" : "glyph_expand" );
		} 

		// Update progress bars
		FOR_EACH_MAP_FAST( m_mapMapPanels, i )
		{
			int nMapIndex = m_mapMapPanels.Key( i );
			auto healthData = GTFGCClientSystem()->GetHealthDataForMap( nMapIndex );

			// Update bars with latest health data
			ProgressBar* pProgress = m_mapMapPanels[ i ]->FindControl< ProgressBar >( "HealthProgressBar", true );
			if ( pProgress )
			{
				pProgress->SetProgress( healthData.m_flRatio );
				pProgress->SetFgColor( healthData.m_colorBar );
			}
		}
	}

	void SetCheckButtonState( uint32 nMapDefIndex, bool bSelected, bool bClickable )
	{
		auto idx = m_mapMapPanels.Find( nMapDefIndex );
		if ( idx != m_mapMapPanels.InvalidIndex() )
		{
			EditablePanel* pMapEntry = m_mapMapPanels[ idx ];
			CExCheckButton* pMapCheckButton = pMapEntry->FindControl< CExCheckButton >( "MapCheckbutton", true );
			if ( pMapCheckButton )
			{
				// Update button state without sending signals
				pMapCheckButton->SetSilentMode( true );
				pMapCheckButton->SetCheckButtonCheckable( true );
				pMapCheckButton->SetSelected( bSelected );
				pMapCheckButton->SetCheckButtonCheckable( bClickable );
				pMapCheckButton->SetTooltip( bClickable ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );
				pMapCheckButton->SetSilentMode( false );
			}

			if ( g_pClientMode && g_pClientMode->GetViewport() )
			{
				if ( bSelected )
				{
					g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pMapEntry, "HealthProgressBar_NotSelected" );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pMapEntry, "HealthProgressBar_Selected" );
				}
				else
				{
					g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pMapEntry, "HealthProgressBar_Selected" );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pMapEntry, "HealthProgressBar_NotSelected" );
				}
			}
		}
	}

	bool BFeaturedRotationChanged() const
	{
		return m_eCategory == eNumGameCategories && m_unFeaturedRotation != GetFeaturedRotation();
	}

	bool BFeaturedMapsChanged() const
	{
		CUtlVector< const MapDef_t* > vecMaps;
		GetFeaturedMaps( vecMaps );
		if ( vecMaps.Count() != m_vecFeaturedMaps.Count() )
			return true;

		FOR_EACH_VEC( vecMaps, i )
		{
			if ( vecMaps[ i ] != m_vecFeaturedMaps[ i ] )
				return true;
		}

		return false;
	}

	bool BFeaturedMapsChangedThrottled()
	{
		if ( m_eCategory != eNumGameCategories || Plat_FloatTime() < m_flNextFeaturedCheck )
			return false;

		m_flNextFeaturedCheck = Plat_FloatTime() + 10.0;
		return BFeaturedMapsChanged();
	}

	const CUtlVector< const MapDef_t* >& GetFeaturedMapList() const
	{
		return m_vecFeaturedMaps;
	}

	bool UpdateSelection( const CUtlVector< const MapDef_t* >& vecMaps, bool bClickable, CUtlVector< const MapDef_t* >& vecSelectedMaps )
	{
		bool bCatSelected = false;

		FOR_EACH_VEC( vecMaps, k )
		{
			auto &criteria = GTFPartyClient()->GetEffectiveGroupCriteria();
			bool bMapSelected = criteria.IsCasualMapSelected( vecMaps[ k ]->m_nDefIndex );
			bCatSelected = bCatSelected | bMapSelected;

			SetCheckButtonState( vecMaps[ k ]->m_nDefIndex, bMapSelected, bClickable );

			if ( bMapSelected )
			{
				vecSelectedMaps.AddToTail( vecMaps[ k ] );
			}
		}

		if ( bCatSelected )
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( this, "CasualCategory_NotSelected" );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "CasualCategory_Selected" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( this, "CasualCategory_Selected" );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "CasualCategory_NotSelected" );
		}

		InvalidateLayout();

		CExCheckButton* pCheckButton = FindControl< CExCheckButton >( "CheckButton", true );
		if ( pCheckButton )
		{
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - if ( pCheckButton )", __FUNCTION__ );

			pCheckButton->RemoveActionSignalTarget( m_pSignalHandler );
			pCheckButton->SetCheckButtonCheckable( true );
			pCheckButton->SetSelected( bCatSelected );
			pCheckButton->SetCheckButtonCheckable( bClickable );
			pCheckButton->SetTooltip( bClickable ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );
			pCheckButton->AddActionSignalTarget( m_pSignalHandler );

			KeyValues* pKVData = new KeyValues( "data" );
			pKVData->SetInt( m_eCategory == eNumGameCategories ? "featured_index" : "category_index", m_eCategory );
			pCheckButton->SetData( pKVData );
		}

		return bCatSelected;
	}

private:
	void ClearMapEntries()
	{
		// Clear out the old map entries
		FOR_EACH_MAP_FAST( m_mapMapPanels, i )
		{
			m_mapMapPanels[ i ]->MarkForDeletion();
		}
		m_mapMapPanels.Purge();
	}

	const EGameCategory m_eCategory;
	uint32 m_unFeaturedRotation;
	double m_flNextFeaturedCheck;
	CUtlVector< const MapDef_t* > m_vecFeaturedMaps;
	CExImageButton* pToggleButton;
	Panel* m_pSignalHandler;
	CUtlMap< uint32, EditablePanel* > m_mapMapPanels;
};

DECLARE_BUILD_FACTORY( CCasualCriteriaPanel );
CCasualCriteriaPanel::CCasualCriteriaPanel( vgui::Panel *pParent, const char* pszName ) 
	: EditablePanel( pParent, pszName )
	, m_fontCategoryListItem( 0 )
	, m_fontGroupHeader( 0 )
	, m_flCompetitiveRankProgress( -1.f )
	, m_flCompetitiveRankPrevProgress( -1.f )
	, m_flRefreshPlayerListTime( -1.f )
	, m_bCompetitiveRankChangePlayedSound( false )
	, m_mapGroupPanels( DefLessFunc( EMatchmakingGroupType ) )
	, m_mapCategoryPanels( DefLessFunc( EGameCategory ) )
	, m_bCriteriaDirty( true )
{
	GTFPartyClient()->LoadSavedCasualCriteria();

	ListenForGameEvent( "matchmaker_stats_updated" );
	ListenForGameEvent( "party_criteria_changed" );
}

CCasualCriteriaPanel::~CCasualCriteriaPanel()
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCasualCriteriaPanel::OnThink()
{
	BaseClass::OnThink();

	auto idxFeatured = m_mapCategoryPanels.Find( eNumGameCategories );
	if ( idxFeatured != m_mapCategoryPanels.InvalidIndex() && ( ( (CCasualCategory*)m_mapCategoryPanels[ idxFeatured ] )->BFeaturedRotationChanged() || ( (CCasualCategory*)m_mapCategoryPanels[ idxFeatured ] )->BFeaturedMapsChangedThrottled() ) )
	{
		m_bCriteriaDirty = true;
	}

	if ( m_bCriteriaDirty )
	{
		WriteCategories();
	}
}

void CCasualCriteriaPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "matchmaker_stats_updated" ) )
	{
		m_bCriteriaDirty = true;
		return;
	}
	if ( FStrEq( event->GetName(), "party_criteria_changed" ) )
	{
		m_bCriteriaDirty = true;
		return;
	}
}

//-----------------------------------------------------------------------------
void CCasualCriteriaPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	CExImageButton *pButton = FindControl<CExImageButton>( "RestoreCasualSearchCriteria", true );
	if ( pButton )
	{
		pButton->SetTooltip( GetDashboardTooltip( k_eLargeFont ), "#TF_Casual_Tip_Restore" );
	}

	pButton = FindControl<CExImageButton>( "SaveCasualSearchCriteria", true );
	if ( pButton )
	{
		pButton->SetTooltip( GetDashboardTooltip( k_eLargeFont ), "#TF_Casual_Tip_Save" );
	}
}

void CCasualCriteriaPanel::OnCommand( const char *command )
{
	if ( FStrEq( "show_explanations", command ) )
	{
		ShowDashboardExplanation( "CasualIntro" );
	}
	else if ( FStrEq( command, "restore_search_criteria" ) )
	{
		if ( GTFGCClientSystem() )
		{
			GTFPartyClient()->LoadSavedCasualCriteria();
		}
		return;
	}
	else if ( FStrEq( command, "save_search_criteria" ) )
	{
		if ( GTFGCClientSystem() )
		{
			GTFPartyClient()->SaveCasualCriteria();
		}
		return;
	}
}

//-----------------------------------------------------------------------------
void CCasualCriteriaPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	m_mapGroupPanels.Purge();
	m_mapCategoryPanels.Purge();

	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/MatchmakingCasualCriteria.res" );
	m_bCriteriaDirty = true;
	
	m_fontCategoryListItem = pScheme->GetFont( "HudFontSmallest", true );
	m_fontGroupHeader = pScheme->GetFont( "HudFontSmallestBold", true );
}

void CCasualCriteriaPanel::OnCategoryExpanded( KeyValues* pKVParams )
{
	if ( !tf_show_maps_details_explanation_session.GetBool() && tf_show_maps_details_explanation_count.GetInt() > 0 )
	{
		tf_show_maps_details_explanation_count.SetValue( tf_show_maps_details_explanation_count.GetInt() - 1 );
		tf_show_maps_details_explanation_session.SetValue( true );

		CExplanationPopup* pPopup = ShowDashboardExplanation( "MapSelectionDetailsExplanation" );
		if ( pPopup )
		{
			// Move the callout to be pointing at the first map in the category they clicked
			Panel* pCatPanel = reinterpret_cast<vgui::Panel *>( pKVParams->GetPtr( "panel" ) );
			int nX, nY;
			nX = pCatPanel->GetWide() * 0.15f;	// Magically slighty in the left column
			nY = pCatPanel->GetTall() * 1.2f;	// Magically slighty on the first row
			pCatPanel->LocalToScreen( nX, nY );
			pPopup->SetCalloutInParentsX( nX );
			pPopup->SetCalloutInParentsY( nY );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A check box got checked!  Search criteria
//-----------------------------------------------------------------------------
void CCasualCriteriaPanel::OnCheckButtonChecked( vgui::Panel* panel )
{
	//if ( m_iWritingPanel > 0 )
	//	return;

	CExCheckButton* pCheckButton = dynamic_cast< CExCheckButton* >( panel );
	if ( pCheckButton )
	{
		bool bSelected = pCheckButton->IsSelected();

		pCheckButton->RemoveActionSignalTarget( this );
		pCheckButton->SetSelected( false );
		pCheckButton->AddActionSignalTarget( this );

		if ( GTFPartyClient()->BControllingPartyActions() )
		{
			int nMapIndex = pCheckButton->GetData()->GetInt( "map_index", -1 );
			int nCategoryIndex = pCheckButton->GetData()->GetInt( "category_index", -1 );
			int nGroupIndex = pCheckButton->GetData()->GetInt( "group_index", -1 );
			int nFeaturedIndex = pCheckButton->GetData()->GetInt( "featured_index", -1 );
			Assert( nCategoryIndex >= 0 || nGroupIndex >= 0 || nMapIndex >= 0 || nFeaturedIndex >= 0 );
			if ( nGroupIndex >= 0 )
			{
				EMatchmakingGroupType eGroup = EMatchmakingGroupType( nGroupIndex );
				GTFPartyClient()->MutLocalGroupCriteria().SetCasualGroupSelected( eGroup, bSelected );
			}
			else if ( nCategoryIndex >= 0 )
			{
				EGameCategory eCategory = EGameCategory( nCategoryIndex );
				GTFPartyClient()->MutLocalGroupCriteria().SetCasualCategorySelected( eCategory, bSelected );
			}
			else if ( nMapIndex >= 0 )
			{
				GTFPartyClient()->MutLocalGroupCriteria().SetCasualMapSelected( nMapIndex, bSelected );
			}
			else if ( nFeaturedIndex >= 0 )
			{
				auto idxFeatured = m_mapCategoryPanels.Find( eNumGameCategories );
				if ( idxFeatured != m_mapCategoryPanels.InvalidIndex() )
				{
					const CUtlVector< const MapDef_t* >& vecFeaturedMaps = ( (CCasualCategory*)m_mapCategoryPanels[ idxFeatured ] )->GetFeaturedMapList();
					FOR_EACH_VEC( vecFeaturedMaps, i )
					{
						GTFPartyClient()->MutLocalGroupCriteria().SetCasualMapSelected( vecFeaturedMaps[ i ]->m_nDefIndex, bSelected );
					}
				}
			}
		}

		m_bCriteriaDirty = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Write all the category controls settings
//-----------------------------------------------------------------------------
void CCasualCriteriaPanel::WriteCategories( void )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !m_bCriteriaDirty )
		return;

	m_bCriteriaDirty = false;

	bool bLeader = GTFPartyClient()->BControllingPartyActions();
	m_bHasAMapSelected = false;

	CScrollableList* pScrollableList = FindControl< CScrollableList >( "GameModesList", true );
	if ( !pScrollableList )
		return;

	CUtlVector< const MapDef_t* > vecSelectedMaps;

	pScrollableList->SetMouseInputEnabled( true );

	const MMGroupMap_t& mapMMGroups = GetItemSchema()->GetMMGroupMap();

	int nCategory = 0;
	// Go through every MM group, and create a label for it and all its categories underneath
	FOR_EACH_MAP( mapMMGroups, i )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - FOR_EACH_MAP( mapMMGroups, i )", __FUNCTION__ );

		const SchemaMMGroup_t* pCat = mapMMGroups[ i ];
		
		if ( !pCat->m_bitsValidMMGroups.IsBitSet( k_eTFMatchGroup_Casual_12v12 ) )
		{
			continue;
		}

		if ( !pCat->IsCategoryValid() && pCat->m_eMMGroup != kMatchmakingType_SpecialEvents )
		{
			continue;
		}

		++nCategory;

		bool bGroupSelected = false;

		EditablePanel* pGroupPanel = NULL;
		CExCheckButton* pTitleLabel = NULL;
		auto idx = m_mapGroupPanels.Find( pCat->m_eMMGroup );
		// Create the check button/label if not created yet
		if ( idx == m_mapGroupPanels.InvalidIndex() )
		{
			pGroupPanel = new EditablePanel( pScrollableList, "MatchmakingGroupPanel" );
			pGroupPanel->LoadControlSettings( "resource/ui/MatchMakingGroupPanel.res" );
			pTitleLabel = pGroupPanel->FindControl< CExCheckButton >( "Checkbutton" );
			pTitleLabel->SetText( pCat->m_pszLocalizedName  );

			KeyValues* pKVData = new KeyValues( "data" );
			pKVData->SetInt( "group_index", pCat->m_eMMGroup );
			pTitleLabel->SetData( pKVData );

			pScrollableList->AddPanel( pGroupPanel, nCategory > 1 ? 2 : -4 );
			m_mapGroupPanels.Insert( pCat->m_eMMGroup, pGroupPanel );
		}
		else
		{
			pGroupPanel = (EditablePanel*)m_mapGroupPanels[ idx ];
			pTitleLabel = pGroupPanel->FindControl< CExCheckButton >( "Checkbutton" );
		}

		// Category items.
		FOR_EACH_VEC( pCat->m_vecModes, j )
		{
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - FOR_EACH_VEC( pCat->m_vecModes, j )", __FUNCTION__ );

			const SchemaGameCategory_t* pCategory = pCat->m_vecModes[ j ];

			if ( !pCategory->PassesRestrictions() )
			{
				continue;
			}

			CCasualCategory* pListEntry = NULL;
			auto idxCat = m_mapCategoryPanels.Find( pCategory->m_eGameCategory );
			// Create the entry if it doesnt exist yet
			if ( idxCat == m_mapCategoryPanels.InvalidIndex() )
			{
				pListEntry = new CCasualCategory( pScrollableList, "MatchmakingCategoryPanel", pCategory->m_eGameCategory, this );
				pListEntry->AddActionSignalTarget( this );
				pListEntry->MakeReadyForUse();

				pScrollableList->AddPanel( pListEntry, j > 0 ? 5 : 0 );
				m_mapCategoryPanels.Insert( pCategory->m_eGameCategory, pListEntry );
			}
			else
			{
				pListEntry = (CCasualCategory*)m_mapCategoryPanels[ idxCat ];
			}

			bool bCatSelected = pListEntry->UpdateSelection( pCategory->m_vecEnabledMaps, bLeader, vecSelectedMaps );
			m_bHasAMapSelected |= bCatSelected;
			bGroupSelected = bGroupSelected | bCatSelected;
		}

		if ( pCat->m_eMMGroup == kMatchmakingType_SpecialEvents )
		{
			CCasualCategory* pFeaturedEntry = NULL;
			auto idxFeatured = m_mapCategoryPanels.Find( eNumGameCategories );
			if ( idxFeatured == m_mapCategoryPanels.InvalidIndex() )
			{
				pFeaturedEntry = new CCasualCategory( pScrollableList, "MatchmakingCategoryPanel", eNumGameCategories, this );
				pFeaturedEntry->AddActionSignalTarget( this );
				pFeaturedEntry->MakeReadyForUse();

				pScrollableList->AddPanel( pFeaturedEntry, 5 );
				m_mapCategoryPanels.Insert( eNumGameCategories, pFeaturedEntry );
			}
			else
			{
				pFeaturedEntry = (CCasualCategory*)m_mapCategoryPanels[ idxFeatured ];
			}

			if ( pFeaturedEntry->BFeaturedRotationChanged() || pFeaturedEntry->BFeaturedMapsChanged() )
			{
				pFeaturedEntry->InvalidateLayout( false, true );
			}

			m_bHasAMapSelected |= pFeaturedEntry->UpdateSelection( pFeaturedEntry->GetFeaturedMapList(), bLeader, vecSelectedMaps );
		}

		// Update check button state
		pTitleLabel->RemoveActionSignalTarget( this );
		pTitleLabel->SetCheckButtonCheckable( true );
		pTitleLabel->SetSelected( bGroupSelected );
		pTitleLabel->SetCheckButtonCheckable( bLeader );
		pTitleLabel->SetTooltip( bLeader ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );
		pTitleLabel->AddActionSignalTarget( this );
	}

	EditablePanel* pPlayListPanel = FindControl< EditablePanel >( "PlaylistBGPanel", true );
	if ( pPlayListPanel )
	{
		// Setup the "X maps selected" label
		const char* pszToken = vecSelectedMaps.Count() == 1 ? "TF_Casual_SelectedMaps_Singular" : "TF_Casual_SelectedMaps_Plural";
		pPlayListPanel->SetDialogVariable( "selected_maps_count", LocalizeNumberWithToken( pszToken, vecSelectedMaps.Count() ) );
	}

// TODO	m_pContainer->SetNextButtonEnabled( m_bHasAMapSelected );
}
