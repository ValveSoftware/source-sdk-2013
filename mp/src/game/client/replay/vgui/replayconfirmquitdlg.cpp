//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayconfirmquitdlg.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "ienginevgui.h"
#include "replay/genericclassbased_replay.h"
#include "replaybrowserrenderdialog.h"
#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------

ConVar replay_quitmsg_dontaskagain( "replay_quitmsg_dontaskagain", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "The replay system will ask you to render your replays on quit, unless this cvar is 1.", true, 0, true, 1 );

//-----------------------------------------------------------------------------

CReplayConfirmQuitDialog::CReplayConfirmQuitDialog( Panel *pParent )
:	BaseClass( pParent, "confirmquitdlg" ),
	m_pDontShowAgain( NULL ),
	m_pQuitButton( NULL )
{
	SetScheme( "ClientScheme" );
	InvalidateLayout( true, true );
}

void CReplayConfirmQuitDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	// Link in TF scheme
	extern IEngineVGui *enginevgui;
	vgui::HScheme pTFScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( pTFScheme );
	SetProportional( true );

	BaseClass::ApplySchemeSettings( vgui::scheme()->GetIScheme( pTFScheme ) );

	LoadControlSettings( "Resource/UI/replaybrowser/confirmquitdlg.res", "GAME" );

	m_pDontShowAgain = dynamic_cast< CheckButton * >( FindChildByName( "DontShowThisAgainCheckbox" ) );
	m_pQuitButton = dynamic_cast< CExButton * >( FindChildByName( "QuitButton" ) );

	if ( m_pQuitButton )
	{
		m_pQuitButton->GetTextImage()->ClearColorChangeStream();
		m_pQuitButton->GetTextImage()->AddColorChange( Color(200,80,60,255), 0 );
	}
}

void CReplayConfirmQuitDialog::OnCommand( const char *pCommand )
{
	// Store the setting of our "never show this again" checkbox if the user picked anything
	// except cancel.
	if ( !FStrEq( pCommand, "cancel" ) && m_pDontShowAgain && m_pDontShowAgain->IsSelected() )
	{
		replay_quitmsg_dontaskagain.SetValue( 1 );
	}

	if ( FStrEq( pCommand, "rendernow_delay" ) )
	{
		// Delete this
		SetVisible( false );
		MarkForDeletion();

		// Render all unrendered replays now
		ReplayUI_ShowRenderDialog( NULL, REPLAY_HANDLE_INVALID, true, -1 );
	}
	else if ( FStrEq( pCommand, "rendernow" ) )
	{
		// Sometimes this message comes in just before input is processed when using a controller
		// Refire after a delay
		PostMessage( this, new KeyValues( "Command", "command", "rendernow_delay" ), 0.001f );
	}
	else if ( FStrEq( pCommand, "quit" ) )
	{
		MarkForDeletion();
		engine->ClientCmd_Unrestricted( "quit\n" );
	}
	else if ( FStrEq( pCommand, "cancel" ) )
	{
		MarkForDeletion();
	}
	else if ( FStrEq( pCommand, "gotoreplays"))
	{
		// "Go to replays"
		MarkForDeletion();
		engine->ClientCmd( "replay_reloadbrowser" );
	}
}

void CReplayConfirmQuitDialog::OnKeyCodeTyped( vgui::KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		OnCommand( "cancel" );
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

void CReplayConfirmQuitDialog::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( GetBaseButtonCode( code ) == KEY_XBUTTON_B )
	{
		OnCommand( "cancel" );
	}
	else if ( GetBaseButtonCode( code ) == KEY_XBUTTON_A )
	{
		OnCommand( "quit" );
	}
	else if ( GetBaseButtonCode( code ) == KEY_XBUTTON_X )
	{
		if ( m_pDontShowAgain )
		{
			m_pDontShowAgain->SetSelected( !m_pDontShowAgain->IsSelected() );
		}
	}
	else if ( GetBaseButtonCode( code ) == KEY_XBUTTON_Y )
	{
		OnCommand( "gotoreplays" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

bool ReplayUI_ShowConfirmQuitDlg()
{
	if ( replay_quitmsg_dontaskagain.GetBool() )
		return false;

	CReplayConfirmQuitDialog *pConfirmQuitDlg = vgui::SETUP_PANEL( new CReplayConfirmQuitDialog( NULL ) );
	if ( pConfirmQuitDlg )
	{
		vgui::surface()->PlaySound( "replay\\replaydialog_warn.wav" );

		// Display the panel!
		pConfirmQuitDlg->SetVisible( true );
		pConfirmQuitDlg->MakePopup();
		pConfirmQuitDlg->MoveToFront();
		pConfirmQuitDlg->SetKeyBoardInputEnabled( true );
		pConfirmQuitDlg->SetMouseInputEnabled( true );
		TFModalStack()->PushModal( pConfirmQuitDlg );
	}

	return true;
}

#endif