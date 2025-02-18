//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v2/tf_store_page2.h"
#include "store/v2/tf_store_preview_item2.h"
#include "c_tf_freeaccount.h"
#include "store/store_panel.h"
#include "store/tf_store.h"
#include "navigationpanel.h"
#include "econ/store/store_page_new.h"
#include "econ_item_system.h"
#include "c_tf_gamestats.h"
#include "vgui_controls/TextImage.h"
#include "econ_item_description.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static const int kNumSortTypes = 5;
ItemSortTypeData_t g_StoreSortTypes[ kNumSortTypes ] =
{
	{ "#Store_SortType_DateNewest", kEconStoreSortType_DateNewest },
	{ "#Store_SortType_DateOldest", kEconStoreSortType_DateOldest },
	{ "#Store_SortType_HighestPrice", kEconStoreSortType_Price_HighestToLowest },
	{ "#Store_SortType_LowestPrice", kEconStoreSortType_Price_LowestToHighest },
	{ "#Store_SortType_Alphabetical", kEconStoreSortType_Name_AToZ },
};

class CClassFilterTooltip : public vgui::BaseTooltip
{
public:
	CClassFilterTooltip( CTFStorePage2 *pStorePage )
	: BaseTooltip( pStorePage )
	, m_pStorePage( pStorePage )
	{
	}

	CTFStorePage2 *m_pStorePage;

	virtual void SetText(const char *text)
	{
		m_pStorePage->m_pClassFilterTooltipLabel->SetText( text );
	}

	virtual void ShowTooltip(Panel *currentPanel)
	{
		int x = 0;
		int y = currentPanel->GetTall();
		currentPanel->LocalToScreen( x, y );
		m_pStorePage->ScreenToLocal( x, y );

		// The tooltip wants to be centered around the given panel, but it's constrained by the left and right boundaries
		// of the navigation panel.
		if ( m_pStorePage->m_pClassFilterButtons )
		{
			// Right side of tooltip should not pass right boundary of nav panel
			int aClassFilterNavPos[2] = { 0, 0 };
			m_pStorePage->m_pClassFilterButtons->GetPos( aClassFilterNavPos[0], aClassFilterNavPos[1] );
			const int nTipWide = m_pStorePage->m_pClassFilterTooltipLabel->GetWide();
			x = clamp(
				x + ( currentPanel->GetWide() - nTipWide ) / 2,
				aClassFilterNavPos[0],
				aClassFilterNavPos[0] + m_pStorePage->m_pClassFilterButtons->GetWide() - nTipWide );
		}

		m_pStorePage->m_pClassFilterTooltipLabel->SetPos( x, y );
		m_pStorePage->m_pClassFilterTooltipLabel->SetVisible( true );
	}
	virtual void HideTooltip()
	{
		m_pStorePage->m_pClassFilterTooltipLabel->SetVisible( false );
	}
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage2::CTFStorePage2(Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile )
:	BaseClass( parent, pPageData, pPreviewItemResFile ),
	m_pSubcategoriesFilterCombo( NULL ),
	m_pSortByCombo( NULL ),
	m_pHomeCategoryTabs( NULL ),
	m_pClassFilterButtons( NULL ),
	m_pNameFilterTextEntry( NULL ),
	m_pSubcategoriesFilterLabel( NULL ),
	m_iCurrentSubcategory( 2 ), // Switch to Featured items tab on home page when store launches; BRETT SAID I COULD DO THIS
	m_pClassFilterTooltipLabel( NULL ),
	m_pClassFilterTooltip( NULL ),
	m_flFilterItemTime( 0.0f )
{
	const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();

	if ( IsHomePage() )
	{
		bool bAtLeastOneItemIsOnSale = false;
		if ( pPriceSheet )
		{
			const CEconStorePriceSheet::StoreEntryMap_t& mapStoreEntries = pPriceSheet->GetEntries();
			FOR_EACH_MAP_FAST( mapStoreEntries, i )
			{
				if ( mapStoreEntries[i].IsOnSale( EconUI()->GetStorePanel()->GetCurrency() ) )
				{
					bAtLeastOneItemIsOnSale = true;
					break;
				}
			}
		}

		m_pHomeCategoryTabs = new CNavigationPanel( this, "ItemCategoryTabs" );

		FOR_EACH_VEC( m_pPageData->m_vecSubcategories, i )
		{
			// Skip over adding the "On Sale!" tab if no items are currently on sale.
			const char *pchName = m_pPageData->m_vecSubcategories[i]->m_pchName;
			bool bIsOnSaleCategory = m_pPageData->m_vecSubcategories[i]->m_unID == CEconStoreCategoryManager::k_CategoryID_OnSale;

			// Skip over the "Top Sellers" tab as we're replacing it with the 'Starter Packs' tab for now
			bool bIsPopularCategory = m_pPageData->m_vecSubcategories[i]->m_unID == CEconStoreCategoryManager::k_CategoryID_Popular;

			// Skip over the "New" tab as we're replacing it with the 'Featured' tab for now
			bool bIsNew = m_pPageData->m_vecSubcategories[i]->m_unID == CEconStoreCategoryManager::k_CategoryID_New;

			// We include all of the sale items in the Featured tab currently, so we don't need to add these categories back in at the moment
			bAtLeastOneItemIsOnSale = false;

			if ( ( !bIsPopularCategory && !bIsOnSaleCategory && !bIsNew ) || ( bAtLeastOneItemIsOnSale && bIsOnSaleCategory ) )
			{
				m_pHomeCategoryTabs->AddButton( i, pchName );
			}
		}
	}
}

//-----------------------------------------------------------------------------
CTFStorePage2::~CTFStorePage2()
{
	delete m_pClassFilterTooltip;
	m_pClassFilterTooltip = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnPostCreate()
{
	BaseClass::OnPostCreate();

	m_bShouldDeletePreviewPanel = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStorePage2::HasSubcategories() const
{
	return m_pPageData && m_pPageData->HasSubcategories();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pSubcategoriesFilterCombo )
	{
		m_pSubcategoriesFilterCombo->SetVisible( HasSubcategories() );
	}

	if ( m_pSubcategoriesFilterLabel )
	{
		m_pSubcategoriesFilterLabel->SetVisible( HasSubcategories() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pNameFilterTextEntry = FindControl<vgui::TextEntry>( "NameFilterTextEntry" );
	if ( m_pNameFilterTextEntry )
	{
		m_pNameFilterTextEntry->AddActionSignalTarget( this );
	}

	m_pSortByCombo = dynamic_cast< ComboBox * >( FindChildByName( "SortFilterComboBox" ) );
	if ( m_pSortByCombo )
	{
		m_pSortByCombo->RemoveAll();
		vgui::HFont hFont = pScheme->GetFont( "HudFontSmallestBold", true );
		m_pSortByCombo->SetFont( hFont );
		KeyValues *pKeyValues = new KeyValues( "data" );
		for ( int i = 0; i < ARRAYSIZE(g_StoreSortTypes); i++ )
		{
			pKeyValues->SetInt( "sortby", i );
			m_pSortByCombo->AddItem( g_StoreSortTypes[i].szSortDesc, pKeyValues );
		}
		pKeyValues->deleteThis();
		m_pSortByCombo->ActivateItemByRow( 0 );
	}

	m_pSubcategoriesFilterCombo = dynamic_cast< ComboBox * >( FindChildByName( "SubcategoryFilterComboBox" ) );
	if ( m_pSubcategoriesFilterCombo )
	{
		vgui::HFont hFont = pScheme->GetFont( "HudFontSmallestBold", true );
		m_pSubcategoriesFilterCombo->SetFont( hFont );

		m_pSubcategoriesFilterCombo->RemoveAll();

		if ( m_pPageData )
		{
			// Add "all items" explicitly
			KeyValuesAD kvAllItems( "data" );
			kvAllItems->SetInt( "index", GetNumSubcategories() );
			m_pSubcategoriesFilterCombo->AddItem( "#Store_ClassFilter_None", kvAllItems );

			FOR_EACH_VEC( m_pPageData->m_vecSubcategories, i )
			{
				KeyValues *pData = new KeyValues( "data" );
				pData->SetInt( "index", i );

				m_pSubcategoriesFilterCombo->AddItem( m_pPageData->m_vecSubcategories[i]->m_pchName, pData );

				pData->deleteThis();
			}
		}

		// Move to "All items" selected
		m_pSubcategoriesFilterCombo->ActivateItemByRow( 0 );

		m_pSubcategoriesFilterCombo->GetComboButton()->SetFgColor( Color( 117,107,94,255 ) ); 
		m_pSubcategoriesFilterCombo->GetComboButton()->SetDefaultColor( Color( 117,107,94,255), Color( 0,0,0,0) );
		m_pSubcategoriesFilterCombo->GetComboButton()->SetArmedColor( Color( 117,107,94,255), Color( 0,0,0,0) );
		m_pSubcategoriesFilterCombo->GetComboButton()->SetDepressedColor( Color( 117,107,94,255), Color( 0,0,0,0) );
	}

	m_pClassFilterButtons = dynamic_cast< CNavigationPanel * >( FindChildByName( "ClassFilterNavPanel" ) );
	m_pSubcategoriesFilterLabel = dynamic_cast< CExLabel * >( FindChildByName( "SubcategoryFiltersLabel" ) );

	m_pClassFilterTooltipLabel = dynamic_cast< CExLabel * >( FindChildByName( "ClassFilterTooltipLabel" ) );
	if ( m_pClassFilterTooltipLabel && m_pClassFilterButtons )
	{
		m_pClassFilterTooltip = new CClassFilterTooltip(this);
		for ( int i = 0 ; i < m_pClassFilterButtons->NumButtons() ; ++i )
		{
			CExButton *pButton = m_pClassFilterButtons->GetButton( i );
			CUtlString sSaveText = pButton->GetEffectiveTooltipText();
			pButton->SetTooltip( m_pClassFilterTooltip, sSaveText );
		}
	}

	// Setup title text in home page
	if ( IsHomePage() && g_pVGuiLocalize )
	{
		CExLabel *pTitleLabel = dynamic_cast<CExLabel *>( FindChildByName( "TitleLabel" ) );
		wchar_t *pHomePageTitle = g_pVGuiLocalize->Find( "#Store_HomePageTitle" );
		wchar_t *pRedText = g_pVGuiLocalize->Find( "#Store_HomePageTitleRedText" );

		if ( pTitleLabel && pHomePageTitle && pRedText )
		{
			const store_promotion_spend_for_free_item_t *pPromotion = EconUI()->GetStorePanel()->GetPriceSheet()->GetStorePromotion_SpendForFreeItem();
		
			ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
			AssertMsg( eCurrency >= k_ECurrencyUSD && eCurrency < k_ECurrencyMax, "Invalid currency!" );
			int iPriceThreshold = pPromotion->m_rgusPriceThreshold[ eCurrency ];
			wchar_t wszPriceThreshold[ kLocalizedPriceSizeInChararacters ];
			MakeMoneyString( wszPriceThreshold, ARRAYSIZE( wszPriceThreshold ), iPriceThreshold, EconUI()->GetStorePanel()->GetCurrency() );
		
			static wchar_t wszText[512];
			g_pVGuiLocalize->ConstructString_safe( wszText, pHomePageTitle, 2, pRedText, wszPriceThreshold );

			pTitleLabel->SetText( wszText );
			TextImage *pTextImage = pTitleLabel->GetTextImage();
			const wchar_t *pFound = wcsstr( wszText, pRedText );
			if ( pTextImage && pFound )
			{
				const int iRedTextPos = pFound - wszText;
				const int nRedTextLen = wcslen( pRedText );
				pTextImage->ClearColorChangeStream();
				pTextImage->AddColorChange( Color(200,80,60,255), iRedTextPos );
				pTextImage->AddColorChange( pTitleLabel->GetFgColor(), iRedTextPos + nRedTextLen );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFStorePage2::GetPageResFile( void ) 
{ 
	if ( IsHomePage() )
	{
		Assert( ShouldUseNewStore() );

		return "Resource/UI/econ/store/v2/StoreHome_Premium.res";
	}

	return m_pPageData->m_pchPageRes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnPageShow( void )
{
	BaseClass::OnPageShow();

	if ( m_pClassFilterButtons )
	{
		m_pClassFilterButtons->UpdateButtonSelectionStates( 0 );
	}

	ClearNameFilter( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnItemDetails( vgui::Panel *panel )
{
	CStoreItemControlsPanel *pControlsPanel = dynamic_cast< CStoreItemControlsPanel * >( panel );
	if ( pControlsPanel )
	{
		const econ_store_entry_t *pEntry = pControlsPanel->GetItem();
		if ( pEntry && m_pPreviewPanel )
		{
			ShowPreviewWindow( pEntry->GetItemDefinitionIndex() );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFStorePage2::OnItemDefDetails( KeyValues *pData )
{
	ShowPreviewWindow( pData->GetInt("ItemDefIndex", -1) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::ShowPreviewWindow( item_definition_index_t usDefIndex )
{
	if ( m_pPreviewPanel )
	{
		CEconItemView itemData;
		itemData.Init( usDefIndex, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		itemData.SetClientItemFlags( kEconItemFlagClient_Preview );

		m_pPreviewPanel->PreviewItem( 0, &itemData );
		m_pPreviewPanel->SetState( PS_ITEM );	// Adding this, since without it, only the item icon shows up
		m_pPreviewPanel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnNavButtonSelected( KeyValues *pData )
{
	Panel *pPanel = (Panel *)pData->GetPtr( "panel" );

	if ( pPanel == m_pClassFilterButtons )
	{
		const int iFilter = pData->GetInt( "userdata", -1 );	AssertMsg( iFilter >= 0, "Bad filter" );
		if ( iFilter < 0 )
			return;

		SetFilter( iFilter );
		m_iCurrentPage = 0;
		UpdateModelPanels();

		if ( m_pCheckoutButton )
		{
			m_pCheckoutButton->RequestFocus();
		}

		C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_page_2_nav(class)", CFmtStr( "%i", iFilter ).Access() );
	}
	else if ( pPanel == m_pHomeCategoryTabs )
	{
		int iSelectedTab = pData->GetInt( "userdata", -1 );
		if ( iSelectedTab < 0 )
			return;

		if ( !m_pPageData || !m_pPageData->m_vecSubcategories.IsValidIndex( iSelectedTab ) )
			return;

		m_iCurrentSubcategory = iSelectedTab;

		FOR_EACH_VEC( m_vecItemPanels, i )
		{
			// Delete the old one
			m_vecItemPanels[i].m_pStorePricePanel->MarkForDeletion();

			// Create a new one and cache it
			CStorePricePanel *pPricePanel = CreatePricePanel( i );
			pPricePanel->InvalidateLayout();
			pPricePanel->SetMouseInputEnabled( false );
			pPricePanel->SetKeyBoardInputEnabled( false );

			m_vecItemPanels[i].m_pStorePricePanel = pPricePanel;

			// Setup the mouse handler
			m_vecItemPanels[i].m_pItemControlsPanel->SetMouseHoverHandler( pPricePanel );
		}

		m_iCurrentPage = 0;

		UpdateFilteredItems();
		UpdateModelPanels();

		C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_page_2_nav(category)", CFmtStr( "%i", iSelectedTab ).Access() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CTFStorePage2::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::TextEntry *pTextEntry = dynamic_cast<vgui::TextEntry *>( pPanel );

	if ( pTextEntry )
	{
		if ( pTextEntry == m_pNameFilterTextEntry )
		{
			m_wNameFilter.RemoveAll();
			if ( m_pNameFilterTextEntry->GetTextLength() )
			{
				m_wNameFilter.EnsureCount( m_pNameFilterTextEntry->GetTextLength() + 1 );
				m_pNameFilterTextEntry->GetText( m_wNameFilter.Base(), m_wNameFilter.Count() * sizeof(wchar_t) );
				V_wcslower( m_wNameFilter.Base() );
			}
			m_flFilterItemTime = gpGlobals->curtime + 0.5f;
			return;
		}
	}

	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );

	if ( pComboBox )
	{
		if ( pComboBox == m_pSubcategoriesFilterCombo )
		{
			// the class selection combo box changed, update class details
			KeyValues *pUserData = m_pSubcategoriesFilterCombo->GetActiveItemUserData();
			if ( !pUserData )
				return;

			// Update current subcategory filter
			m_iCurrentSubcategory = pUserData->GetInt( "index", 0 );
			m_bFilterDirty = true;

			m_iCurrentPage = 0;
			UpdateModelPanels();

			if ( m_pCheckoutButton )
			{
				m_pCheckoutButton->RequestFocus();
			}

			C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_page_2_nav(subcategories)", CFmtStr( "%i", m_iCurrentSubcategory ).Access() );
		}

		else if ( pComboBox == m_pSortByCombo )
		{
			// the class selection combo box changed, update class details
			KeyValues *pUserData = m_pSortByCombo->GetActiveItemUserData();
			if ( !pUserData )
				return;

			int iSortTypeSelectionIndex = pUserData->GetInt( "sortby", -1 );
			m_bFilterDirty = true;
			if ( iSortTypeSelectionIndex >= 0 )
			{
				eEconStoreSortType iSortType = (eEconStoreSortType)g_StoreSortTypes[iSortTypeSelectionIndex].iSortType;

				UpdateFilteredItems();
				UpdateModelPanels();

				C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_page_2_nav(sort_by)", CFmtStr( "%i", iSortType ).Access() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStorePage2::GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters )
{
	BaseClass::GetFiltersForDef( pDef, pVecFilters );
}

bool CTFStorePage2::FindAndSelectEntry( const econ_store_entry_t *pEntry )
{
	m_iCurrentSubcategory = GetAllSubcategoriesIndex();
	m_bFilterDirty = true;
	if ( BaseClass::FindAndSelectEntry( pEntry ) )
	{
		ivgui()->PostMessage( GetVPanel(), new KeyValues( "ItemDefDetails", "ItemDefIndex", pEntry->GetItemDefinitionIndex() ), GetVPanel() );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::ClearNameFilter( bool bUpdateModelPanels )
{
	// don't do anything if we don't have any filter
	if ( m_wNameFilter.Count() == 0 )
		return;

	m_wNameFilter.RemoveAll();
	if( m_pNameFilterTextEntry )
	{
		m_pNameFilterTextEntry->SetText( "" );
	}

	if ( bUpdateModelPanels )
	{
		m_flFilterItemTime = gpGlobals->curtime + 0.1f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFStorePage2::GetAllSubcategoriesIndex() const
{
	return m_pPageData ? m_pPageData->GetNumSubcategories() : 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::UpdateFilteredItems()
{
	if ( m_bFilterDirty )
	{
		// Make sure list of unfiltered items is sorted
		m_pSortByCombo = dynamic_cast< ComboBox * >( FindChildByName( "SortFilterComboBox" ) );
		if ( m_pSortByCombo )
		{
			KeyValues *pUserData = m_pSortByCombo->GetActiveItemUserData();
			if ( pUserData )
			{
				int iSortTypeSelectionIndex = pUserData->GetInt( "sortby", -1 );
				if ( iSortTypeSelectionIndex >= 0 )
				{
					eEconStoreSortType iSortType = (eEconStoreSortType)g_StoreSortTypes[iSortTypeSelectionIndex].iSortType;
					CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheetForEdit();
					pPriceSheet->SetEconStoreSortType( iSortType );

					CEconStoreCategoryManager::StoreCategory_t *pPageData = const_cast< CEconStoreCategoryManager::StoreCategory_t * >( m_pPageData );
					pPageData->m_vecEntries.SetLessContext( pPriceSheet );
					pPageData->m_vecEntries.RedoSort( true );
				}
			}
		}
	}

	if ( !IsHomePage() )
	{
		BaseClass::UpdateFilteredItems();
		return;
	}

	m_FilteredEntries.Purge();

	// Subcategories on the home page are special cases, as they aren't based on
	// an item's tags.
	const StoreCategoryID_t unSubcategoryID = m_pPageData->m_vecSubcategories[ m_iCurrentSubcategory ]->m_unID;

	CStorePanel *pStorePanel = EconUI()->GetStorePanel();
	if ( !pStorePanel )
		return;

	FOR_EACH_VEC( m_vecItemPanels, idx )
	{
		m_vecItemPanels[idx].m_pItemModelPanel->SetShowQuantity( unSubcategoryID != CEconStoreCategoryManager::k_CategoryID_Popular );
	}

	if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_Popular )
	{
		const CUtlVector<uint32>& popularItems = pStorePanel->GetPopularItems();
		for ( int i = 0; i < MIN( m_vecItemPanels.Count(), popularItems.Count() ); ++i )
		{
			const econ_store_entry_t *pEntry = pStorePanel->GetPriceSheet()->GetEntry( popularItems[i] );
			m_FilteredEntries.AddToTail( pEntry );
		}
	}
	else if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_New )
	{
		// Add all new items
		const CUtlMap< uint16, econ_store_entry_t > &mapEntries = pStorePanel->GetPriceSheet()->GetEntries();
		FOR_EACH_MAP_FAST( mapEntries, i )
		{
			const econ_store_entry_t *pCurEntry = &mapEntries[i];
			if ( pCurEntry->m_bNew )
			{
				m_FilteredEntries.AddToTail( pCurEntry );
			}
		}
	}
	else if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_OnSale )
	{
		ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

		// Add all entries that are on sale
		const CEconStorePriceSheet::StoreEntryMap_t &mapEntries = pStorePanel->GetPriceSheet()->GetEntries();
		FOR_EACH_MAP_FAST( mapEntries, i )
		{
			const econ_store_entry_t *pCurEntry = &mapEntries[i];

			if ( pCurEntry->IsOnSale( eCurrency ) )
			{
				m_FilteredEntries.AddToTail( pCurEntry );
			}
		}
	}
	else if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_Featured )
	{
		const CEconStorePriceSheet::FeaturedItems_t& vecFeaturedItems = pStorePanel->GetPriceSheet()->GetFeaturedItems();
		FOR_EACH_VEC( vecFeaturedItems, i )
		{
			const econ_store_entry_t *pEntry = pStorePanel->GetPriceSheet()->GetEntry( vecFeaturedItems[i] );
			if ( pEntry )
			{
				m_FilteredEntries.AddToTail( pEntry );
			}
			else
			{
				AssertMsg( 0, "trying to add featured item that's not in the store price sheet.\n" );
			}
		}
	}
	else if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_ClassBundles )
	{
		// Let's find the class bundles
		const CEconStorePriceSheet::StoreEntryMap_t &mapEntries = pStorePanel->GetPriceSheet()->GetEntries();
		FOR_EACH_MAP_FAST( mapEntries, i )	
		{
			const econ_store_entry_t *pCurEntry = &mapEntries[i];

			if ( pCurEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_ClassBundles ) )
			{
				m_FilteredEntries.AddToTail( pCurEntry );
			}
		}

		// Sort by date to get the weapon bundles before the keyless crates
		//extern int ItemNameSortComparator( const econ_store_entry_t *const *ppEntryA, const econ_store_entry_t *const *ppEntryB );
		extern int FirstSaleDateSortComparator( const econ_store_entry_t *const *ppItemA, const econ_store_entry_t *const *ppItemB );

		m_FilteredEntries.Sort( &FirstSaleDateSortComparator );

	}
	else if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_Taunts )
	{
		const CEconStorePriceSheet::StoreEntryMap_t &mapEntries = pStorePanel->GetPriceSheet()->GetEntries();
		FOR_EACH_MAP_FAST( mapEntries, i )
		{
			const econ_store_entry_t *pCurEntry = &mapEntries[i];

			if ( pCurEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Taunts ) )
			{
				m_FilteredEntries.AddToTail( pCurEntry );
			}
		}
	}
	else
	{
		AssertMsg( 0, "Subcategory has no defined behavior in code" );
	}

	// If we're either "New" category or the "On Sale" category or the "Taunts" category, sort our contents
	// by sale date.
	if ( unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_New || unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_OnSale || unSubcategoryID == CEconStoreCategoryManager::k_CategoryID_Taunts )
	{
		extern int FirstSaleDateSortComparator( const econ_store_entry_t *const *ppItemA, const econ_store_entry_t *const *ppItemB );

		m_FilteredEntries.Sort( &FirstSaleDateSortComparator );
	}

	m_bFilterDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStorePage2::DoesEntryFilterPassSecondaryFilter( const econ_store_entry_t *pEntry )
{
	if ( !DoesEntryFilterPassSubcategoryFilter( pEntry ) )
	{
		return false;
	}

	if ( m_wNameFilter.Count() > 0 )
	{
		CEconItemView itemData;
		itemData.Init( pEntry->GetItemDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		itemData.SetClientItemFlags( kEconItemFlagClient_Preview | kEconItemFlagClient_StoreItem );
		return DoesItemPassSearchFilter( itemData.GetDescription(), m_wNameFilter.Base() );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStorePage2::DoesEntryFilterPassSubcategoryFilter( const econ_store_entry_t *pEntry )
{
	Assert( pEntry );
	Assert( m_pPageData );

	// Make sure pages without subcategories can still function
	if ( !HasSubcategories() )
		return true;

	// "All subcategories" item selected?
	if ( m_iCurrentSubcategory == GetAllSubcategoriesIndex() )
		return true;

	if ( !m_pPageData->m_vecSubcategories.IsValidIndex( m_iCurrentSubcategory ) )
		return false;

	// Get the subcategory ID
	const StoreCategoryID_t unSubCategoryID = m_pPageData->m_vecSubcategories[ m_iCurrentSubcategory ]->m_unID;

	// If the store entry is covered by the currently selected category, return true.
//	return pEntry->m_vecTagIds.Find( unSubCategoryID ) != pEntry->m_vecTagIds.InvalidIndex();
	return pEntry->IsListedInCategory( unSubCategoryID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::UpdateFilterComboBox( void )
{
	BaseClass::UpdateFilterComboBox();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStorePage2::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_flFilterItemTime && gpGlobals->curtime >= m_flFilterItemTime )
	{
		m_bFilterDirty = true;
		UpdateFilteredItems();
		UpdateModelPanels();
		m_flFilterItemTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePreviewItemPanel *CTFStorePage2::CreatePreviewPanel( void )
{
	return new CTFStorePreviewItemPanel2( EconUI()->GetStorePanel(), m_pPreviewItemResFile, "storepreviewitem", this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel* CTFStorePage2::CreatePricePanel( int iIndex )
{
	if ( m_pPageData &&
		 m_pPageData->m_bIsHome &&
		 HasSubcategories() &&
		 m_pPageData->m_vecSubcategories.IsValidIndex( m_iCurrentSubcategory ) &&
		 m_pPageData->m_vecSubcategories[ m_iCurrentSubcategory ]->m_unID == CEconStoreCategoryManager::k_CategoryID_Popular )
	{
		return vgui::SETUP_PANEL( new CStorePricePanel_Popular( this, "StorePrice", iIndex + 1 ) );
	}

	return BaseClass::CreatePricePanel( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnAddItemToCart( KeyValues *pData )
{
	item_definition_index_t iItemDef = (item_definition_index_t)pData->GetInt( "item_def", INVALID_ITEM_DEF_INDEX );
	
	AddItemToCartHelper( GetPageName(), iItemDef, (ECartItemType)pData->GetInt( "cart_add_type", kCartItem_Purchase ) );
	UpdateCart();

	// Turn the free slots indicator red if we can't fit everything.
	UpdateBackpackLabel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel && IsVisible() && pItemPanel->HasItem() )
	{
		FOR_EACH_VEC( m_vecItemPanels, i )
		{
			if ( m_vecItemPanels[i].m_pItemModelPanel == pItemPanel )
			{
				ShowPreviewWindow( m_vecItemPanels[i].m_pItemModelPanel->GetItem()->GetItemDefIndex() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage2::OnItemPanelMouseDoublePressed( vgui::Panel *panel )
{
	// Do nothing
}
