//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/store_page_halloween.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "c_tf_player.h"
#include "gamestringpool.h"
#include "tf_item_inventory.h"
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "store/store_panel.h"
#include "store_preview_item.h"
#include "store_viewcart.h"
#include "c_tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage_SpecialPromo::CTFStorePage_SpecialPromo( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData ) : BaseClass( parent, pPageData )
{
	pszResFile = pPageData->m_pchPageRes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*
void CTFStorePage_SpecialPromo::OrderItemsForDisplay( CUtlVector<const econ_store_entry_t *>& vecItems ) const
{
	vecItems.Sort( &ItemDisplayOrderSort_UseSortOverride );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage_Popular::CTFStorePage_Popular( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData ) : BaseClass( parent, pPageData )
{
}

//-----------------------------------------------------------------------------
// Purpose: The popular page draws its list of items from the overall popular items list.
//-----------------------------------------------------------------------------
void CTFStorePage_Popular::UpdateFilteredItems( void )
{
	m_FilteredEntries.Purge();
	m_vecFilterCounts.SetCount( GetNumPrimaryFilters() );
	if ( !m_vecFilterCounts.Count() )
		return;

	FOR_EACH_VEC( m_vecFilterCounts, i )
	{
		m_vecFilterCounts[i] = 0;
	}

	CStorePanel *pStorePanel = EconUI()->GetStorePanel();
	if ( !pStorePanel )
		return;

	// Add all popular items
	const CUtlVector<uint32>& popularItems = pStorePanel->GetPopularItems();

	for ( int i=0; i<popularItems.Count(); ++i )
	{
		const econ_store_entry_t *pEntry = pStorePanel->GetPriceSheet()->GetEntry( popularItems[i] );
		m_FilteredEntries.AddToTail( pEntry );
	}

	FOR_EACH_VEC( m_vecItemPanels, idx )
	{
		m_vecItemPanels[idx].m_pItemModelPanel->SetShowQuantity( false );
	}

	m_pFilterComboBox->SetVisible( false );
}