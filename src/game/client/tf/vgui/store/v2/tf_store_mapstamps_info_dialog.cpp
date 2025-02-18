//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v2/tf_store_mapstamps_info_dialog.h"
#include "tf_mouseforwardingpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DECLARE_BUILD_FACTORY( CTFMapStampsInfoDialog );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMapStampsInfoDialog::CTFMapStampsInfoDialog( vgui::Panel *pParent, const char *pName )
:	BaseClass( pParent, "MapStampsInfoDialog" )
{								   
	m_pBgPanel = new CMouseMessageForwardingPanel( this, "BgPanel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapStampsInfoDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/store/v2/StoreMapStampsInfoDialog.res" );

	m_pDlgFrame = dynamic_cast<EditablePanel *>( FindChildByName( "DialogFrame" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapStampsInfoDialog::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapStampsInfoDialog::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "close", 5 ) )
	{
		DoClose();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapStampsInfoDialog::OnMouseReleased(MouseCode code)
{
	BaseClass::OnMouseReleased( code );

	if ( m_pDlgFrame && !m_pDlgFrame->IsCursorOver() )
	{
		DoClose();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapStampsInfoDialog::DoClose()
{
	SetVisible( false );
	MarkForDeletion();
}