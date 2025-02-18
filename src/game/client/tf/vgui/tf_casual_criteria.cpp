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

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern Color s_colorChallengeForegroundEnabled;
extern Color s_colorChallengeForegroundDisabled;
extern Color s_colorChallengeHeader;

extern const char *s_pszMatchGroups[];

ConVar tf_show_maps_details_explanation_session( "tf_show_maps_details_explanation_session", "0", FCVAR_HIDDEN );
ConVar tf_show_maps_details_explanation_count( "tf_show_maps_details_explanation_count", "2", FCVAR_ARCHIVE | FCVAR_HIDDEN );

using namespace vgui;

class CCasualCategory : public CExpandablePanel
{
	DECLARE_CLASS_SIMPLE( CCasualCategory, CExpandablePanel );

public:
	CCasualCategory( Panel *parent, const char *panelName, EGameCategory eCategory, Panel* pSignalHandler ) 
		: BaseClass( parent, panelName )
		, m_eCategory( eCategory )
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

		const SchemaGameCategory_t* pCategory = GetItemSchema()->GetGameCategory( m_eCategory );
		Assert( pCategory );
		if ( !pCategory )
			return;

		EditablePanel* pTopContainer = FindControl< EditablePanel >( "TopContainer", true );
		if ( pTopContainer )
		{
			// Set our dialog variables
			pTopContainer->SetDialogVariable( "title_token", g_pVGuiLocalize->Find( pCategory->m_pszLocalizedName ) );
			pTopContainer->SetDialogVariable( "desc_token", g_pVGuiLocalize->Find( pCategory->m_pszLocalizedDesc ) );
		}

		ImagePanel* pImagePanel = FindControl< ImagePanel >( "BGImage", true );
		if ( pImagePanel && pCategory && pCategory->m_pszListImage )
		{
			pImagePanel->SetImage( pCategory->m_pszListImage );
		}

		// Clear out the old map entries
		ClearMapEntries();

		EditablePanel* pMapsContainer = FindControl< EditablePanel >( "MapsContainer", true );

		if ( pMapsContainer )
		{
			int nYPos = 16;

			// Sort the maps alphabetically
			CUtlVector< const MapDef_t* > vecSortedMaps;
			vecSortedMaps.AddVectorToTail( pCategory->m_vecEnabledMaps );
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
			Assert( nCategoryIndex >= 0 || nGroupIndex >= 0 || nMapIndex >= 0 );
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

		if ( !pCat->IsCategoryValid() )
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

			bool bCatSelected = false;

			FOR_EACH_VEC( pCategory->m_vecEnabledMaps, k )
			{
				auto &criteria = GTFPartyClient()->GetEffectiveGroupCriteria();
				bool bMapSelected = criteria.IsCasualMapSelected( pCategory->m_vecEnabledMaps[ k ]->m_nDefIndex );
				m_bHasAMapSelected |= bMapSelected;
				bCatSelected = bCatSelected | bMapSelected;
				bGroupSelected = bGroupSelected | bCatSelected;

				// Update map check button state
				pListEntry->SetCheckButtonState( pCategory->m_vecEnabledMaps[ k ]->m_nDefIndex, bMapSelected, bLeader );

				// We're going to use this to setup the tooltip for total selected maps
				if ( bMapSelected )
				{
					vecSelectedMaps.AddToTail( pCategory->m_vecEnabledMaps[ k ] );
				}
			}

			if ( bCatSelected )
			{
				g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pListEntry, "CasualCategory_NotSelected" );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pListEntry, "CasualCategory_Selected" );
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pListEntry, "CasualCategory_Selected" );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pListEntry, "CasualCategory_NotSelected" );
			}

			pListEntry->InvalidateLayout();

			// Update the check button within the list entry
			CExCheckButton* pCheckButton = pListEntry->FindControl< CExCheckButton >( "CheckButton", true );
			if ( pCheckButton )
			{
				tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - if ( pCheckButton )", __FUNCTION__ );

				pCheckButton->RemoveActionSignalTarget( this );	// So we dont endlessly loop by checking
				pCheckButton->SetCheckButtonCheckable( true );	// So we can potentially check it on the next line
				pCheckButton->SetSelected( bCatSelected );
				pCheckButton->SetCheckButtonCheckable( bLeader );
				pCheckButton->SetTooltip( bLeader ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );
				pCheckButton->AddActionSignalTarget( this );	// So that we get user check messages

				KeyValues* pKVData = new KeyValues( "data" );
				pKVData->SetInt( "category_index", pCategory->m_eGameCategory );
				pCheckButton->SetData( pKVData );
			}
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
