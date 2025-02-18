//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v2/tf_store_page_maps2.h"
#include "store/v2/tf_store_mapstamps_info_dialog.h"
#include "store/store_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage_Maps2::CTFStorePage_Maps2( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData ) 
:	BaseClass( parent, pPageData, "Resource/UI/econ/store/v2/StorePreviewItemPanel_Maps.res" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage_Maps2::OnPageShow()
{
	BaseClass::OnPageShow();

	SetDetailsVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage_Maps2::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "maps_learnmore", 14 ) )
	{
		DisplayMapStampsDialog();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage_Maps2::DisplayMapStampsDialog()
{
	CTFMapStampsInfoDialog *pDlg = vgui::SETUP_PANEL( new CTFMapStampsInfoDialog( EconUI()->GetStorePanel() ) );
	pDlg->SetVisible( true );
	pDlg->InvalidateLayout( true, true );
	pDlg->SetKeyBoardInputEnabled(true);
	pDlg->SetMouseInputEnabled(true);
}
