//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v2/tf_store_panel2.h"
#include "store/v2/tf_store_page2.h"
#include "store/v2/tf_store_page_maps2.h"
#include "store/store_page_halloween.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePanel2::CTFStorePanel2( vgui::Panel *parent ) : CTFBaseStorePanel(parent)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel2::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel2::ShowPanel( bool bShow )
{
	BaseClass::ShowPanel( bShow );

	// Base class should turn this on
	if ( bShow && m_bOGSLogging )
	{
		EconUI()->Gamestats_Store( IE_STORE2_ENTERED );
	}

	Panel *pCheckOutButton = FindChildByName( "CheckOutButton" );
	if ( pCheckOutButton )
	{
		pCheckOutButton->RequestFocus();
	}
}

void CTFStorePanel2::OnAddToCart( void )
{
	Panel *pCheckOutButton = FindChildByName( "CheckOutButton" );
	if ( pCheckOutButton )
	{
		pCheckOutButton->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel2::OnThink()
{
	BaseClass::OnThink();
}

void CTFStorePanel2::OnKeyCodePressed( vgui::KeyCode code )
{
	// ESC cancels
	if ( code == KEY_XBUTTON_B )
	{
		OnCommand( "close" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePanel2::PostTransactionCompleted( void )
{
	BaseClass::PostTransactionCompleted();
}

//-----------------------------------------------------------------------------
// Purpose: Static store page factory.
//-----------------------------------------------------------------------------
CStorePage *CTFStorePanel2::CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData )
{
	if ( pPageData )
	{
		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_SpecialPromo" ) )
			return new CTFStorePage_SpecialPromo( this, pPageData );

		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_Maps" ) )
			return new CTFStorePage_Maps2( this, pPageData );

		if ( !Q_strcmp( pPageData->m_pchPageClass, "CStorePage_Popular" ) )
			return new CTFStorePage_Popular( this, pPageData );
	}

	// Default, standard store page.
	return new CTFStorePage2( this, pPageData );
}
