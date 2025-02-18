//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v1/tf_store_preview_item.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePreviewItemPanel1::CTFStorePreviewItemPanel1( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner )
:	BaseClass( pParent, pResFile, "storepreviewitem", pOwner )
{								   
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::OnClassIconSelected( KeyValues *data )
{
	BaseClass::OnClassIconSelected( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::OnHideClassIconMouseover( void )
{
	BaseClass::OnHideClassIconMouseover();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::OnShowClassIconMouseover( KeyValues *data )
{
	BaseClass::OnShowClassIconMouseover( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry )
{
	BaseClass::PreviewItem( iClass, pItem, pEntry );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::SetState( preview_state_t iState )
{
	BaseClass::SetState( iState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::UpdateIcons( void )
{
	BaseClass::UpdateIcons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel1::OnTick( void )
{
	BaseClass::OnTick();
}
