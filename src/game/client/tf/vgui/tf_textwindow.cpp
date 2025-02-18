//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "inputsystem/iinputsystem.h"
#include "input.h"

#include "tf_textwindow.h"
#include <cdll_client_int.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/Panel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/ImagePanel.h>

#include "tf_controls.h"
#include "tf_shareddefs.h"

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTextWindow::CTFTextWindow( IViewPort *pViewPort ) : CTextWindow( pViewPort )
{
	m_pTFTextMessage = new CExRichText( this, "TFTextMessage" );

	SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFTextWindow::~CTFTextWindow()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::ApplySchemeSettings( IScheme *pScheme )
{
	Frame::ApplySchemeSettings( pScheme );  // purposely skipping the CTextWindow version

	if ( ::input->IsSteamControllerActive() )
	{
		if ( m_bCustomSvrPage )
		{
			LoadControlSettings( "Resource/UI/TextWindowCustomServer_SC.res" );
		}
		else
		{
			LoadControlSettings( "Resource/UI/TextWindow_SC.res" );
		}

		SetMouseInputEnabled( false );
	}
	else
	{
		if ( m_bCustomSvrPage )
		{
			LoadControlSettings( "Resource/UI/TextWindowCustomServer.res" );
		}
		else
		{
			LoadControlSettings( "Resource/UI/TextWindow.res" );
		}
		SetMouseInputEnabled( true );
	}


	if ( m_pHTMLMessage )
	{
		m_pHTMLMessage->SetBgColor( pScheme->GetColor( "HTMLBackground", Color( 255, 0, 0, 255 ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::Reset( void )
{
	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::OnThink()
{
	//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::SetData(KeyValues *data)
{
	m_bCustomSvrPage = data->GetBool( "customsvr" );
	InvalidateLayout( false, true );
	BaseClass::SetData( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::Update()
{
	CExLabel *pTitle = dynamic_cast<CExLabel *>( FindChildByName( "TFMessageTitle" ) );
	if ( pTitle )
	{
		pTitle->SetText( m_szTitle );
	}

	if ( m_pTFTextMessage )
	{
		m_pTFTextMessage->SetVisible( false );
	}

	BaseClass::Update();

	Panel *pOK = FindChildByName( "ok" );
	if ( pOK )
	{
		pOK->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------
void CTFTextWindow::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	if ( state )
	{
		Panel *pOK = FindChildByName( "ok" );
		if ( pOK )
		{
			pOK->RequestFocus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows the text window
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	// Force use to reevaluate our scheme, in case Steam Controller stuff has changed.
	InvalidateLayout( true, true );

	BaseClass::ShowPanel( bShow );

	if ( m_pViewPort )
	{
		m_pViewPort->ShowBackGround( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_A || code == STEAMCONTROLLER_A )
	{
		OnCommand( "okay" );		
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The background is painted elsewhere, so we should do nothing
//-----------------------------------------------------------------------------
void CTFTextWindow::PaintBackground()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );

	// Don't open up the mapinfo if it was a custom server html page
	if ( !Q_strcmp( command, "okay" ) && !m_bCustomSvrPage )
	{
		m_pViewPort->ShowPanel( PANEL_MAPINFO, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowText( const char *text )
{
	ShowTitleLabel( true );

	if ( m_pTFTextMessage )
	{
		m_pTFTextMessage->SetVisible( true );
		m_pTFTextMessage->SetText( text );
		m_pTFTextMessage->GotoTextStart();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowURL( const char *URL, bool bAllowUserToDisable )
{
	ShowTitleLabel( false );
	BaseClass::ShowURL( URL, bAllowUserToDisable );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowFile( const char *filename )
{
	ShowTitleLabel( false )	;
	BaseClass::ShowFile( filename );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowTitleLabel( bool show )
{
	CExLabel *pTitle = dynamic_cast<CExLabel *>( FindChildByName( "TFMessageTitle" ) );
	if ( pTitle )
	{
		pTitle->SetVisible( show );
	}
}
