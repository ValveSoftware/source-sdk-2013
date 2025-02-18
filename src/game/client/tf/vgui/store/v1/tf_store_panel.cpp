//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v1/tf_store_page.h"
#include "store/v1/tf_store_panel.h"
#include "store/store_page_halloween.h"
#include "store/store_page_new.h"
#include "store/v1/tf_store_page_maps.h"
#include "store/store_viewcart.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePanel1::CTFStorePanel1( vgui::Panel *parent ) : CTFBaseStorePanel(parent)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel1::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel1::OnThink()
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel1::PostTransactionCompleted( void )
{
	BaseClass::PostTransactionCompleted();
}

//-----------------------------------------------------------------------------
// Purpose: Static store page factory.
//-----------------------------------------------------------------------------
CStorePage *CTFStorePanel1::CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData )
{
	if ( pPageData )
	{
		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_SpecialPromo" ) )
			return new CTFStorePage_SpecialPromo( this, pPageData );

		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_Maps" ) )
			return new CTFStorePage_Maps( this, pPageData );

		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_Popular" ) )
			return new CTFStorePage_Popular( this, pPageData );
	}

	// Default, standard store page.
	return new CTFStorePage1( this, pPageData );
}
