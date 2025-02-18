//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/tf_store_page_base.h"
//#include "store/v1/tf_store_preview_item.h"
#include "econ_item_inventory.h"
#include "store/store_viewcart.h"
#include "c_tf_freeaccount.h"
#include "rtime.h"
#include "econ_ui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern const char *g_aPlayerClassNames[TF_CLASS_MENU_BUTTONS];

const char *g_szClassFilterStrings[] =
{
	"",	// Undefined
	"#Store_Items_Scout",
	"#Store_Items_Sniper",
	"#Store_Items_Soldier",
	"#Store_Items_Demoman",
	"#Store_Items_Medic",
	"#Store_Items_HWGuy",
	"#Store_Items_Pyro",
	"#Store_Items_Spy",
	"#Store_Items_Engineer"
};

DECLARE_BUILD_FACTORY( CStorePreviewClassIcon );

ConVar tf_explanations_store( "tf_explanations_store", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePageBase::CTFStorePageBase(Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile ) : CStorePage(parent, pPageData, pPreviewItemResFile)
{
	m_flStartExplanationsAt = 0;

	// TF has an option for each class, all class items, all items, and an unowned item option. Let's make sure they all fit.
	if ( m_pFilterComboBox )
	{
		m_pFilterComboBox->SetNumberOfEditLines( 12 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::OnPageShow( void )
{
	BaseClass::OnPageShow();

	// If this is the first time we've opened the store, start the armory explanations
	if ( !tf_explanations_store.GetBool() && m_pPageData )
	{
		m_flStartExplanationsAt = engine->Time() + 0.5;
		vgui::ivgui()->AddTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "show_explanations" ) )
	{
		if ( !m_flStartExplanationsAt )
		{
			m_flStartExplanationsAt = engine->Time();
			vgui::ivgui()->AddTickSignal( GetVPanel() );
		}
		RequestFocus();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters )
{
	pVecFilters->AddToTail( FILTER_ALL_ITEMS );

	// Add item to unowned filter only if it doesn't belong to these categories.
	const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( pDef->GetDefinitionIndex() );
	if( !pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Tools ) &&
		!pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Maps ) &&
		!pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Bundles ) &&
		!pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Collections ) )
	{
		bool bItemOwned = false;
		int iCount = InventoryManager()->GetLocalInventory()->GetItemCount();
		for ( int i = 0; i < iCount; i++ )
		{
			if ( InventoryManager()->GetLocalInventory()->GetItem( i )->GetItemDefIndex() == pDef->GetDefinitionIndex() )
			{
				bItemOwned = true;
				break;
			}
		}

		if ( !bItemOwned )
		{
			pVecFilters->AddToTail( FILTER_UNOWNED_ITEMS );
		}
	}

	if ( pDef->CanBeUsedByAllClasses() )
		pVecFilters->AddToTail( FILTER_ALLCLASS_ITEMS );

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( pDef->CanBeUsedByClass( iClass ) )
			pVecFilters->AddToTail( iClass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::OnItemDetails( vgui::Panel *panel )
{
	CStoreItemControlsPanel *pControlsPanel = dynamic_cast< CStoreItemControlsPanel * >( panel );
	if ( pControlsPanel )
	{
		const econ_store_entry_t *pEntry = pControlsPanel->GetItem();
		if ( pEntry )
		{
			SelectItemPanel( pControlsPanel->GetItemModelPanel() );
			PostMessage( EconUI()->GetStorePanel(), new KeyValues("ArmoryOpened", "itemdef", pEntry->GetItemDefinitionIndex() ) );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::ShowPreview( int iClass, const econ_store_entry_t* pEntry )
{
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
	{
		iClass = TF_CLASS_SCOUT;
	}

	BaseClass::ShowPreview( iClass, pEntry );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePageBase::UpdateFilterComboBox( void )
{
	if ( !m_pFilterComboBox )
		return;

	wchar_t wzLocalized[256];
	wchar_t wszCount[16];

	m_pFilterComboBox->RemoveAll();

	// All items
	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "filter", FILTER_ALL_ITEMS );
	m_pFilterComboBox->AddItem( "#Store_ClassFilter_None", pKeyValues );

#if NEWFILTER
	// All classes
	int nCount = m_pPrimaryFilter->GetCountForFilterItem( FILTER_ALLCLASS_ITEMS );
	if ( nCount )
	{
		pKeyValues->SetInt( "filter", FILTER_ALLCLASS_ITEMS );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", nCount );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_ClassFilter_AllClasses" ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}

	// Individual classes
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		nCount = m_pPrimaryFilter->GetCountForFilterItem( iClass );
		if ( !nCount )
			continue;

		pKeyValues->SetInt( "filter", iClass );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", nCount );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( g_szClassFilterStrings[iClass] ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}

	// Unowned item filter
	nCount = m_pPrimaryFilter->GetCountForFilterItem( FILTER_UNOWNED_ITEMS );
	if ( nCount )
	{
		pKeyValues->SetInt( "filter", FILTER_UNOWNED_ITEMS );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", nCount );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_Items_Unowned" ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}

#else
	// All classes
	if ( m_vecFilterCounts[FILTER_ALLCLASS_ITEMS] )
	{
		pKeyValues->SetInt( "filter", FILTER_ALLCLASS_ITEMS );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", m_vecFilterCounts[FILTER_ALLCLASS_ITEMS] );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_ClassFilter_AllClasses" ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}

	// Individual classes
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( m_vecFilterCounts[iClass] == 0 )
			continue;

		pKeyValues->SetInt( "filter", iClass );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", m_vecFilterCounts[iClass] );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( g_szClassFilterStrings[iClass] ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}

	// Unowned item filter
	if ( m_vecFilterCounts[FILTER_UNOWNED_ITEMS] )
	{
		pKeyValues->SetInt( "filter", FILTER_UNOWNED_ITEMS );

		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", m_vecFilterCounts[FILTER_UNOWNED_ITEMS] );
		g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_Items_Unowned" ), 1, wszCount );
		m_pFilterComboBox->AddItem( wzLocalized, pKeyValues );
	}
#endif

	pKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStorePageBase::OnTick( void )
{
	BaseClass::OnTick();

	if ( m_flStartExplanationsAt && m_flStartExplanationsAt < engine->Time() )
	{
		m_flStartExplanationsAt = 0;

		tf_explanations_store.SetValue( 1 );

		CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( FindChildByName("StartExplanation") );
		if ( pPopup )
		{
			pPopup->Popup();
		}
	}

	if ( !m_flStartExplanationsAt )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

