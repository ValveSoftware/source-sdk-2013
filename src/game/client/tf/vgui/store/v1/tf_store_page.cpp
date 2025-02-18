//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v1/tf_store_page.h"
#include "store/v1/tf_store_preview_item.h"
#include "c_tf_freeaccount.h"
#include "store/store_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage1::CTFStorePage1(Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile ) : BaseClass(parent, pPageData, pPreviewItemResFile)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFStorePage1::GetPageResFile( void ) 
{ 
	Assert( !"No code should currently reference the old store!" );

	return m_pPageData->m_pchPageRes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage1::OnPageShow( void )
{
	BaseClass::OnPageShow();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage1::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage1::OnItemDetails( vgui::Panel *panel )
{
	BaseClass::OnItemDetails( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage1::ShowPreview( int iClass, const econ_store_entry_t* pEntry )
{
	BaseClass::ShowPreview( iClass, pEntry );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage1::UpdateFilterComboBox( void )
{
	BaseClass::UpdateFilterComboBox();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStorePage1::GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters )
{
	return BaseClass::GetFiltersForDef( pDef, pVecFilters );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStorePage1::OnTick( void )
{
	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePreviewItemPanel *CTFStorePage1::CreatePreviewPanel( void )
{
	return new CTFStorePreviewItemPanel1( this, m_pPreviewItemResFile, "storepreviewitem", this );
}
