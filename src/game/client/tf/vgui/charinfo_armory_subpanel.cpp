//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "charinfo_armory_subpanel.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "c_tf_player.h"
#include "c_tf_gamestats.h"
#include "gamestringpool.h"
#include "tf_item_inventory.h"
#include "econ_item_system.h"
#include "iachievementmgr.h"
#include "store/store_panel.h"
#include "character_info_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_explanations_charinfo_armory_panel( "tf_explanations_charinfo_armory_panel", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );
ConVar tf_armory_page_skip( "tf_armory_page_skip", "10", FCVAR_ARCHIVE, "Number of pages to skip in the Mann Co. Catalog", true, 1, true, 100 );

const char *g_szArmoryFilterStrings[ARMFILT_TOTAL] =
{
	"#ArmoryFilter_AllItems",		// ARMFILT_ALL_ITEMS.
	"#ArmoryFilter_Weapons",		// ARMFILT_WEAPONS,
	"#ArmoryFilter_MiscItems",		// ARMFILT_MISCITEMS,
	"#ArmoryFilter_ActionItems",	// ARMFILT_ACTIONITEMS,
	"#ArmoryFilter_CraftItems",		// ARMFILT_CRAFTITEMS,
	"#ArmoryFilter_Tools",			// ARMFILT_TOOLS,
	"#ArmoryFilter_AllClass",		// ARMFILT_CLASS_ALL,
	"#ArmoryFilter_Scout",			// ARMFILT_CLASS_SCOUT,
	"#ArmoryFilter_Sniper",			// ARMFILT_CLASS_SNIPER,
	"#ArmoryFilter_Soldier",		// ARMFILT_CLASS_SOLDIER,
	"#ArmoryFilter_Demoman",		// ARMFILT_CLASS_DEMOMAN,
	"#ArmoryFilter_Medic",			// ARMFILT_CLASS_MEDIC,
	"#ArmoryFilter_Heavy",			// ARMFILT_CLASS_HEAVY,
	"#ArmoryFilter_Pyro",			// ARMFILT_CLASS_PYRO,
	"#ArmoryFilter_Spy",			// ARMFILT_CLASS_SPY,
	"#ArmoryFilter_Engineer",		// ARMFILT_CLASS_ENGINEER,
	"#ArmoryFilter_Donationitems",	// ARMFILT_DONATIONITEMS,

	"",								// ARMFILT_NUM_IN_DROPDOWN
	"Not Used",						// ARMFILT_CUSTOM
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CArmoryPanel::CArmoryPanel(Panel *parent, const char *panelName) : vgui::EditablePanel( parent, panelName )
{
	m_pSelectedItemModelPanel = new CItemModelPanel( this, "SelectedItemModelPanel" );
	m_pSelectedItemImageModelPanel = new CItemModelPanel( this, "SelectedItemImageModelPanel" );
	m_pThumbnailModelPanelKVs = NULL;
	m_bReapplyItemKVs = false;
	m_CurrentFilter = ARMFILT_ALL_ITEMS;
	m_OldFilter = ARMFILT_ALL_ITEMS;
	m_iFilterPage = 0;
	m_pViewSetButton = NULL;
	m_pStoreButton = NULL;
	m_bAllowGotoStore = false;

	m_pDataPanel = new vgui::EditablePanel( this, "DataPanel" );
	m_pDataTextRichText = NULL;

	m_pMouseOverItemPanel = new CItemModelPanel( this, "mouseoveritempanel" );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );
	m_pMouseOverTooltip->SetPositioningStrategy( IPTTP_BOTTOM_SIDE );

	m_pFilterComboBox = new vgui::ComboBox( this, "FilterComboBox", ARMFILT_NUM_IN_DROPDOWN, false );
	m_pFilterComboBox->AddActionSignalTarget( this );

	REGISTER_COLOR_AS_OVERRIDABLE( m_colThumbnailBG, "thumbnail_bgcolor" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colThumbnailBGMouseover, "thumbnail_bgcolor_mouseover" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colThumbnailBGSelected, "thumbnail_bgcolor_selected" );

	m_bEventLogging = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CArmoryPanel::~CArmoryPanel()
{
	if ( m_pThumbnailModelPanelKVs )
	{
		m_pThumbnailModelPanelKVs->deleteThis();
		m_pThumbnailModelPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/CharInfoArmorySubPanel.res" );

	m_bReapplyItemKVs = true;
	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	m_pDataTextRichText = dynamic_cast<CEconItemDetailsRichText*>( m_pDataPanel->FindChildByName( "Data_TextRichText" ) );
	m_pViewSetButton = dynamic_cast<CExButton*>( FindChildByName("ViewSetButton") );
	m_pStoreButton = dynamic_cast<CExButton*>( FindChildByName("StoreButton") );

	m_pDataTextRichText->SetURLClickedHandler( this );

	m_colSetName = GetSchemeColor( "ItemSetName", Color(255, 255, 255, 255), pScheme );

	SetupComboBox( NULL );
	UpdateSelectedItem();
	m_pFilterComboBox->SetBorder( NULL );

	m_pMouseOverItemPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::SetupComboBox( const char *pszCustomAddition )
{
	m_pFilterComboBox->RemoveAll();

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	vgui::HFont hFont = pScheme->GetFont( "HudFontSmallestBold", true );
	m_pFilterComboBox->SetFont( hFont );

	KeyValues *pKeyValues = new KeyValues( "data" );
	for ( int i = 0; i < ARMFILT_NUM_IN_DROPDOWN; i++ )
	{
		pKeyValues->SetInt( "setfilter", i );
		m_pFilterComboBox->AddItem( g_szArmoryFilterStrings[i], pKeyValues );
	}

	if ( pszCustomAddition )
	{
		pKeyValues->SetInt( "setfilter", ARMFILT_CUSTOM );
		m_pFilterComboBox->AddItem( g_pVGuiLocalize->Find( pszCustomAddition ), pKeyValues );

		// Start with the custom filter selected
		m_pFilterComboBox->SetNumberOfEditLines( ARMFILT_NUM_IN_DROPDOWN + 1 );
		m_pFilterComboBox->ActivateItemByRow( ARMFILT_NUM_IN_DROPDOWN );
	}
	else
	{
		m_pFilterComboBox->SetNumberOfEditLines( ARMFILT_NUM_IN_DROPDOWN );
		m_pFilterComboBox->ActivateItemByRow( 0 );
	}

	pKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "thumbnail_modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pThumbnailModelPanelKVs )
		{
			m_pThumbnailModelPanelKVs->deleteThis();
		}
		m_pThumbnailModelPanelKVs = new KeyValues("thumbnail_modelpanels_kv");
		pItemKV->CopySubkeys( m_pThumbnailModelPanelKVs );
	}
}

	//	C_CTF_GameStats.Event_Item( IE_ARMORY_EXITED );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnShowPanel( void )
{
	InvalidateLayout( true, true );

	m_pMouseOverItemPanel->SetVisible( false );

	UpdateSelectedItem();

	// If this is the first time we've opened the armory, start the armory explanations
	if ( !tf_explanations_charinfo_armory_panel.GetBool() && ShouldShowExplanations() )
	{
		m_flStartExplanationsAt = engine->Time() + 0.5;
	}

	SetVisible( true );

	if ( !m_bEventLogging )
	{
		C_CTF_GameStats.Event_Catalog( IE_ARMORY_ENTERED );
		m_bEventLogging = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Select the given item and jump to the appropriate page
//-----------------------------------------------------------------------------
void CArmoryPanel::JumpToItem( int iItemDef, armory_filters_t nFilter )
{
	// Setup filter and select iItemDef
	SetFilterTo( iItemDef, nFilter );

	// If we have an item def, find out what page it's on and move to it
	if ( iItemDef > 0 )
	{
		const CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( iItemDef );
		if ( pDef )
		{
			// Attempt to get item def from armory remap parameter
			int iArmoryRemap = pDef->GetArmoryRemap();
			if ( iArmoryRemap > 0 )
			{
				iItemDef = iArmoryRemap;

				SetSelectedItem( iItemDef );
			}

			// If the item specified is a stock item, find the upgradeable version of it instead
			else if ( pDef->GetQuality() == AE_NORMAL )
			{
				// Prepend the upgradeable string 
				char szTmpName[256];
				Q_snprintf( szTmpName, sizeof(szTmpName), "Upgradeable %s", pDef->GetDefinitionName() );

				pDef = ItemSystem()->GetStaticDataForItemByName( szTmpName );
				if ( pDef )
				{
					iItemDef = pDef->GetDefinitionIndex();
				}
			}
		}

		// Find and select the page iItemDef is on
		FOR_EACH_VEC( m_FilteredItemList, i )
		{
			if ( m_FilteredItemList[i] == (item_definition_index_t)iItemDef )
			{
				int iThumbnailsPerPage = (m_iThumbnailRows * m_iThumbnailColumns);
				m_iFilterPage = floor( (float)i / (float)iThumbnailsPerPage );
				break;
			}
		}
	}
	else
	{
		// Default behavior - select first item on first page
		m_iFilterPage = 0;
		SetSelectedItem( m_FilteredItemList[0] );
	}

	UpdateItemList();
	UpdateSelectedItem();

	m_pMouseOverItemPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Show the armory with one of the default filters set
//-----------------------------------------------------------------------------
void CArmoryPanel::ShowPanel( int iItemDef, armory_filters_t nFilter )
{
	JumpToItem( iItemDef, nFilter );
	OnShowPanel();
	m_pFilterComboBox->ActivateItemByRow( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Show the armory with a custom list of item definitions, and a custom filter string 
//-----------------------------------------------------------------------------
void CArmoryPanel::ShowPanel( const char *pszFilterString, CUtlVector<item_definition_index_t> *vecItems )
{
	m_CustomFilteredList = *vecItems;
	SetupComboBox( pszFilterString );
	ShowPanel( 0, ARMFILT_CUSTOM );

	// Move to the custom entry
	m_pFilterComboBox->ActivateItemByRow( ARMFILT_NUM_IN_DROPDOWN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnClosing()
{
	if ( m_bEventLogging )
	{
		C_CTF_GameStats.Event_Catalog( IE_ARMORY_EXITED );
		m_bEventLogging = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "prevpage" ) )
	{
		if ( m_iFilterPage > 0 )
		{
			m_iFilterPage--;
		}
		else
		{
			m_iFilterPage = ceil( m_FilteredItemList.Count() / (float)( m_iThumbnailRows * m_iThumbnailColumns ) ) - 1;
		}
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_stricmp( command, "prevpageskip" ) )
	{
		for ( int i = 0; i < tf_armory_page_skip.GetInt(); i++ )
		{
			if ( m_iFilterPage > 0 )
			{
				m_iFilterPage--;
			}
			else
			{
				m_iFilterPage = ceil( m_FilteredItemList.Count() / (float)( m_iThumbnailRows * m_iThumbnailColumns ) ) - 1;
			}
		}
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_stricmp( command, "skiptostart" ) )
	{
		m_iFilterPage = 0;
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_stricmp( command, "nextpage" ) )
	{
		m_iFilterPage++;
		if ( m_iFilterPage > ceil( m_FilteredItemList.Count() / (float)( m_iThumbnailRows * m_iThumbnailColumns ) ) - 1 )
		{
			m_iFilterPage = 0;
		}
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_stricmp( command, "nextpageskip" ) )
	{
		uint32 unNumPages = ceil( m_FilteredItemList.Count() / (float)( m_iThumbnailRows * m_iThumbnailColumns ) );
		m_iFilterPage = ( ( m_iFilterPage + tf_armory_page_skip.GetInt() ) % unNumPages );
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_stricmp( command, "skiptoend" ) )
	{
		m_iFilterPage = ceil( m_FilteredItemList.Count() / (float)( m_iThumbnailRows * m_iThumbnailColumns ) ) - 1;
		UpdateItemList();
		UpdateSelectedItem();
	}
	else if ( !Q_strnicmp( command, "back", 4 ) )
	{
		PostMessage( GetParent(), new KeyValues("ArmoryClosed") );
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( false, true );
		SetTall( YRES(400) );
		SetVisible( true );
	}
	else if ( !Q_stricmp( command, "openstore" ) )
	{
		// Only available in the loadout->catalog path. So we close down the character info, and move to the store.
		// Bit of a hack.
		EconUI()->CloseEconUI();

		int iItemDef = m_SelectedItem.IsValid() ? m_SelectedItem.GetItemDefIndex() : 0;
		EconUI()->OpenStorePanel( iItemDef, false );
		return;
	}
	else if ( !Q_stricmp( command, "wiki" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			if ( IsVisible() && m_SelectedItem.IsValid() )
			{
				// Determine which language we should use
				char uilanguage[ 64 ];
				uilanguage[0] = 0;
				engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
				ELanguage iLang = PchLanguageToELanguage( uilanguage );

				char szURL[512];
				Q_snprintf( szURL, sizeof( szURL ), "https://wiki.teamfortress.com/scripts/itemredirect.php?id=%d&lang=%s", m_SelectedItem.GetItemDefIndex(), GetLanguageICUName( iLang ) );
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );

				C_CTF_GameStats.Event_Catalog( IE_ARMORY_BROWSE_WIKI, NULL, &m_SelectedItem );
			}
		}
	}
	else if ( !Q_stricmp( command, "viewset" ) )
	{
		if ( m_SelectedItem.IsValid() )
		{
			const CEconItemSetDefinition *pItemSet = m_SelectedItem.GetStaticData()->GetItemSetDefinition();
			if ( pItemSet )
			{
				m_CustomFilteredList.Purge();
				FOR_EACH_VEC( pItemSet->m_iItemDefs, i )
				{
					m_CustomFilteredList.AddToTail( pItemSet->m_iItemDefs[i] );
				}

				SetupComboBox( pItemSet->m_pszLocalizedName );
				SetFilterTo( m_SelectedItem.GetItemDefIndex(), ARMFILT_CUSTOM );
			}
		}
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::MoveItem( int iDelta )
{
	int iIdx = m_FilteredItemList.Find( m_SelectedItem.GetItemDefIndex() );
	m_SelectedItem.Invalidate();

	if ( iIdx != m_FilteredItemList.InvalidIndex() )
	{
		iIdx += iDelta;
		if ( iIdx >= m_FilteredItemList.Count() )
		{
			iIdx = 0;
		}
		else if ( iIdx < 0 )
		{
			iIdx = m_FilteredItemList.Count() - 1;
		}

		if ( iIdx < m_FilteredItemList.Count() )
		{
			const CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_FilteredItemList[iIdx] );
			if ( pDef )
			{
				SetSelectedItem( pDef->GetDefinitionIndex() );
			}
		}
	}

	UpdateSelectedItem();
 }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::PerformLayout( void )
{
	if ( m_bReapplyItemKVs )
	{
		m_bReapplyItemKVs = false;

		if ( m_pThumbnailModelPanelKVs )
		{
			FOR_EACH_VEC( m_pThumbnailModelPanels, i )
			{
				m_pThumbnailModelPanels[i]->ApplySettings( m_pThumbnailModelPanelKVs );
				SetBorderForItem( m_pThumbnailModelPanels[i], false );
				m_pThumbnailModelPanels[i]->InvalidateLayout();
			}
		}
	}

	BaseClass::PerformLayout();

	if ( m_pThumbnailModelPanels.Count() > 0 && m_iThumbnailColumns )
	{
		int iThumbnailModelWide = m_pThumbnailModelPanels[0]->GetWide();
		int iThumbnailModelTall = m_pThumbnailModelPanels[0]->GetTall();
		FOR_EACH_VEC( m_pThumbnailModelPanels, i )
		{
			if ( m_pThumbnailModelPanels[i]->HasItem() )
			{
				m_pThumbnailModelPanels[i]->SetVisible( true );
			}

			int iXPos = ( i % m_iThumbnailColumns );
			int iYPos = ( i / m_iThumbnailColumns );

			int iX = m_iThumbnailX + (m_iThumbnailDeltaX * iXPos) + (iThumbnailModelWide * iXPos);
			int iY = m_iThumbnailY + (m_iThumbnailDeltaY * iYPos) + (iThumbnailModelTall * iYPos);
			m_pThumbnailModelPanels[i]->SetPos( iX, iY );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::SetFilterTo( int iItemDef, armory_filters_t nFilter )
{
	m_FilteredItemList.Purge();
	m_OldFilter = m_CurrentFilter;
	m_CurrentFilter = nFilter;
	m_iFilterPage = 0;

	if ( nFilter == ARMFILT_CUSTOM )
	{
		m_FilteredItemList = m_CustomFilteredList;
	}
	else
	{
		// First, build a list of all the items that match the filter
		const CEconItemSchema::SortedItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetSortedItemDefinitionMap();
		FOR_EACH_MAP( mapItemDefs, i )
		{
			const CTFItemDefinition *pDef = dynamic_cast<const CTFItemDefinition *>( mapItemDefs[i] );

			// Never show:
			//	- Hidden items
			//	- Items that don't have fixed qualities
			//	- Normal quality items
			//	- Items that haven't asked to be shown
			if ( pDef->IsHidden() || pDef->GetQuality() == k_unItemQuality_Any || pDef->GetQuality() == AE_NORMAL || !pDef->ShouldShowInArmory() )
				continue;

#ifdef DEBUG
			// In Debug, make sure that every item shows up in a filter other than the All Items list
			bool bFoundMatchingFilter = false;
			for ( int iFilter = ARMFILT_WEAPONS; iFilter < ARMFILT_NUM_IN_DROPDOWN; iFilter++ )
			{
				if ( DefPassesFilter(pDef,(armory_filters_t)iFilter) )
				{
					bFoundMatchingFilter = true;
					break;
				}
			}
			Assert( bFoundMatchingFilter );
#endif

			if ( DefPassesFilter( pDef, m_CurrentFilter ) )
			{
				m_FilteredItemList.AddToTail( pDef->GetDefinitionIndex() );

				if ( iItemDef == pDef->GetDefinitionIndex() )
				{
					SetSelectedItem( pDef->GetDefinitionIndex() );
				}
			}
		}
	}

	// Make sure our current item is in the list
	if ( m_SelectedItem.IsValid() )
	{
		if ( m_FilteredItemList.Find( m_SelectedItem.GetItemDefIndex() ) == m_FilteredItemList.InvalidIndex() )
		{
			m_SelectedItem.Invalidate();
		}
	}

	UpdateItemList();

	if ( m_CurrentFilter != m_OldFilter )
	{
		C_CTF_GameStats.Event_Catalog( IE_ARMORY_CHANGE_FILTER, g_szArmoryFilterStrings[m_CurrentFilter] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CArmoryPanel::DefPassesFilter( const CTFItemDefinition *pDef, armory_filters_t iFilter )
{
	bool bInList = false;

	switch (iFilter)
	{
	case ARMFILT_ALL_ITEMS:
		{
			bInList = true;
			break;
		}

	case ARMFILT_WEAPONS:
		{
			int iSlot = pDef->GetDefaultLoadoutSlot();
			bInList = ( iSlot == LOADOUT_POSITION_PRIMARY || iSlot == LOADOUT_POSITION_SECONDARY || iSlot == LOADOUT_POSITION_MELEE );
			break;
		}

	case ARMFILT_MISCITEMS:
		{
			bInList = (pDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_MISC);
			break;
		}

	case ARMFILT_ACTIONITEMS:
		{
			bInList = (pDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_ACTION);
			break;
		}

	case ARMFILT_CRAFTITEMS:
		{
			bInList = pDef->GetItemClass() && ( !V_strcmp( pDef->GetItemClass(), "craft_item" ) || !V_strcmp( pDef->GetItemClass(), "class_token" ) || !V_strcmp( pDef->GetItemClass(), "slot_token" ) );
			break;
		}

	case ARMFILT_TOOLS:
		{
			// For now, put the supply crates into the tool list, since it's the only item that shows up in no other lists
			bInList = pDef->GetItemClass() && ( !V_strcmp( pDef->GetItemClass(), "tool" ) || !V_strcmp( pDef->GetItemClass(), "supply_crate" ) );
			break;
		}

	case ARMFILT_CLASS_ALL:
		{
			bInList = pDef->CanBeUsedByAllClasses();
			break;
		}

	case ARMFILT_CLASS_SCOUT:
	case ARMFILT_CLASS_SNIPER:
	case ARMFILT_CLASS_SOLDIER:
	case ARMFILT_CLASS_DEMOMAN:
	case ARMFILT_CLASS_MEDIC:
	case ARMFILT_CLASS_HEAVY:
	case ARMFILT_CLASS_PYRO:
	case ARMFILT_CLASS_SPY:
	case ARMFILT_CLASS_ENGINEER:
		{
			// Don't show class/slot usage for class/slot tokens
			if ( pDef->GetItemClass() && !V_strcmp( pDef->GetItemClass(), "class_token" ) )
				break;

			bInList = ( !pDef->CanBeUsedByAllClasses() && pDef->CanBeUsedByClass( iFilter - ARMFILT_CLASS_SCOUT + 1 ) );
			break;
		}

	case ARMFILT_DONATIONITEMS:
		{
			// Don't show class/slot usage for class/slot tokens
			bInList = pDef->GetItemClass() && !V_strcmp( pDef->GetItemClass(), "map_token" );
			break;
		}
	}

	return bInList;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::UpdateItemList( void )
{
	int iMaxThumbnails = (m_iThumbnailRows * m_iThumbnailColumns);
	int iNumThumbnails = MIN( m_FilteredItemList.Count(), iMaxThumbnails );

	if ( m_pThumbnailModelPanels.Count() < iNumThumbnails )
	{
		for ( int i = m_pThumbnailModelPanels.Count(); i < iNumThumbnails; i++ )
		{
			CItemModelPanel *pPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, VarArgs("thumbnailmodelpanel%d", i) ) );
			pPanel->SetActAsButton( true, true );
			pPanel->ApplySettings( m_pThumbnailModelPanelKVs );
			SetBorderForItem( pPanel, false );
			m_pThumbnailModelPanels.AddToTail( pPanel );

			pPanel->SetTooltip( m_pMouseOverTooltip, "" );
		}
	}
	else if ( m_pThumbnailModelPanels.Count() > iMaxThumbnails )
	{
		FOR_EACH_VEC_BACK( m_pThumbnailModelPanels, i )
		{
			if ( i < iMaxThumbnails  )
				break;

			m_pThumbnailModelPanels[i]->MarkForDeletion();
			m_pThumbnailModelPanels.Remove( i );
		}
	}

	int iStartPos = (m_iFilterPage * iMaxThumbnails);

	CEconItemView *pItemData = new CEconItemView();
	FOR_EACH_VEC( m_pThumbnailModelPanels, i )
	{
		int iItemPos = iStartPos + i;
		if ( iItemPos >= m_FilteredItemList.Count() )
		{
			m_pThumbnailModelPanels[i]->SetItem( NULL );
			m_pThumbnailModelPanels[i]->SetVisible( false );
			continue;
		}

		pItemData->Init( m_FilteredItemList[iItemPos], AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, true );
		m_pThumbnailModelPanels[i]->SetItem( pItemData );
		m_pThumbnailModelPanels[i]->SetVisible( true );
	}
	delete pItemData;

	char szTmp[16];
	int nMaxPages = MAX( 1, ceil(m_FilteredItemList.Count() / (float)(m_iThumbnailRows * m_iThumbnailColumns)) );
	Q_snprintf(szTmp, 16, "%d/%d", m_iFilterPage+1, nMaxPages );
	SetDialogVariable( "thumbnailpage", szTmp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::UpdateSelectedItem( void )
{
	if ( !m_SelectedItem.IsValid() )
	{
		if ( m_FilteredItemList.Count() )
		{
			const CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_FilteredItemList[0] );
			if ( pDef )
			{
				SetSelectedItem( pDef->GetDefinitionIndex() );
			}
		}
	}

	if ( m_pSelectedItemModelPanel )
	{
		m_pSelectedItemModelPanel->SetItem( &m_SelectedItem );
		m_pSelectedItemModelPanel->InvalidateLayout( true );

		int iYDelta = YRES(10);

		// Resize & position the atribute background image
		int iItemX, iItemY, iItemW, iItemH;
		m_pSelectedItemModelPanel->GetBounds( iItemX, iItemY, iItemW, iItemH );

		// Never shrink the attribute background below a certain size
		int iNewPaperH = MAX( iItemH + (iYDelta*2), YRES(100) );
		int iNewPaperY = iItemY - iYDelta;

		int iNewY = iNewPaperY + iNewPaperH;

		// Reposition the data panel now that we know how big the item is
		int iX,iY;
		m_pDataTextRichText->GetPos( iX, iY );
		int iDataPanelX, iDataPanelY;
		m_pDataPanel->GetPos( iDataPanelX, iDataPanelY );
		int iRichTextYPosInDataPanel = iNewY - iDataPanelY;
		m_pDataTextRichText->SetBounds( iX, iRichTextYPosInDataPanel, m_pDataTextRichText->GetWide(), m_pDataPanel->GetTall() - iRichTextYPosInDataPanel );
	}
	if ( m_pSelectedItemImageModelPanel )
	{
		m_pSelectedItemImageModelPanel->SetItem( &m_SelectedItem );
	}

	FOR_EACH_VEC( m_pThumbnailModelPanels, i )
	{
		if ( !m_pThumbnailModelPanels[i]->IsVisible() || !m_pThumbnailModelPanels[i]->HasItem() )
			continue;

		bool bSelected = (m_SelectedItem.GetItemDefIndex() == m_pThumbnailModelPanels[i]->GetItem()->GetItemDefIndex() );
		if ( bSelected != m_pThumbnailModelPanels[i]->IsSelected() )
		{
			m_pThumbnailModelPanels[i]->SetSelected( bSelected );
			SetBorderForItem( m_pThumbnailModelPanels[i], false );
		}
	}

	UpdateDataBlock();

	if ( m_pViewSetButton )
	{
		m_pViewSetButton->SetVisible( false );
		if ( m_SelectedItem.IsValid() )
		{
			if ( m_SelectedItem.GetStaticData()->GetItemSetDefinition() )
			{
				m_pViewSetButton->SetVisible( true );
			}
		}
	}

	if ( m_pStoreButton ) 
	{
		bool bShowStoreButton = m_bAllowGotoStore && EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetPriceSheet() && EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( m_SelectedItem.GetItemDefIndex() );
		m_pStoreButton->SetVisible( bShowStoreButton );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::UpdateDataBlock( void )
{
	if ( !CalculateDataText() )
	{
		m_pDataPanel->SetVisible( false );
		return;
	}

	m_pDataPanel->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CArmoryPanel::CalculateDataText( void )
{
	if ( !m_SelectedItem.IsValid() )
		return false;
	CTFItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_SelectedItem.GetItemDefIndex() );
	if ( !pDef )
		return false;

	m_pDataTextRichText->UpdateDetailsForItem( pDef );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		CEconItemView *pItem = pItemPanel->GetItem();
		SetBorderForItem( pItemPanel, pItem != NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		SetBorderForItem( pItemPanel, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && pItemPanel->HasItem() && !pItemPanel->IsSelected() )
	{
		SetSelectedItem( pItemPanel->GetItem() );

		// Hide the mouseover panel now, so it doesn't obscure the rich text info
		m_pMouseOverItemPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::SetSelectedItem( CEconItemView* newItem )
{
	m_PreviousItem = m_SelectedItem;
	m_SelectedItem = *newItem;
	m_SelectedItem.SetClientItemFlags( kEconItemFlagClient_Preview );
	UpdateSelectedItem();

	if ( m_bEventLogging && m_SelectedItem.IsValid() && m_SelectedItem != m_PreviousItem )
	{
		C_CTF_GameStats.Event_Catalog( IE_ARMORY_SELECT_ITEM, g_szArmoryFilterStrings[m_CurrentFilter], &m_SelectedItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::SetSelectedItem( int newIndex )
{
	m_PreviousItem = m_SelectedItem;
	m_SelectedItem.Init( newIndex, AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, true );
	m_SelectedItem.SetClientItemFlags( kEconItemFlagClient_Preview );

	if ( m_bEventLogging && m_SelectedItem.IsValid() && m_SelectedItem != m_PreviousItem )
	{
		C_CTF_GameStats.Event_Catalog( IE_ARMORY_SELECT_ITEM, g_szArmoryFilterStrings[m_CurrentFilter], &m_SelectedItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	if ( !pItemPanel )
		return;

	// Store panels use backgrounds instead of borders
	pItemPanel->SetBorder( NULL );
	pItemPanel->SetPaintBackgroundEnabled( true );

	if ( pItemPanel->IsSelected() )
	{
		pItemPanel->SetBgColor( m_colThumbnailBGSelected );
	}
	else if ( bMouseOver )
	{
		pItemPanel->SetBgColor( m_colThumbnailBGMouseover );
	}
	else
	{
		pItemPanel->SetBgColor( m_colThumbnailBG );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CArmoryPanel::OnTextChanged( KeyValues *data )
{
	if ( !m_pFilterComboBox )
		return;

	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );

	if ( pComboBox == m_pFilterComboBox )
	{
		// the class selection combo box changed, update class details
		KeyValues *pUserData = m_pFilterComboBox->GetActiveItemUserData();
		if ( !pUserData )
			return;

		armory_filters_t nFilter = (armory_filters_t)pUserData->GetInt( "setfilter", -1 );
		if ( nFilter != armory_filters_t(-1) )
		{
			SetFilterTo( 0, nFilter );
			UpdateSelectedItem();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArmoryPanel::OnItemLinkClicked( KeyValues *pParams )
{
	const char *pURL = pParams->GetString( "url" );
	int iItemDef = atoi( pURL + 7 );
	JumpToItem( iItemDef, ARMFILT_ALL_ITEMS );
	m_pFilterComboBox->ActivateItemByRow( 0 );
}

