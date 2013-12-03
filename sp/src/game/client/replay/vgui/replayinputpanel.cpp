//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayinputpanel.h"
#include "replaybrowsermainpanel.h"
#include "replay/replay.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "ienginevgui.h"
#include "vgui_int.h"
#include "vgui/ISurface.h"
#include "iclientmode.h"
#include "replay/ireplaymanager.h"
#include "econ/econ_controls.h"

#if defined( TF_CLIENT_DLL )
#include "tf_item_inventory.h"
#endif

using namespace vgui;

//-----------------------------------------------------------------------------

static bool s_bPanelVisible = false;

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Player input dialog for a replay
//-----------------------------------------------------------------------------
class CReplayInputPanel	: public EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CReplayInputPanel, EditablePanel );

public:
	CReplayInputPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay );
	~CReplayInputPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( KeyCode code );
	virtual void OnKeyCodeTyped( KeyCode code );

	MESSAGE_FUNC( OnSetFocus, "SetFocus" );

private:
	Panel			*m_pDlg;
	TextEntry		*m_pTitleEntry;
	ReplayHandle_t	m_hReplay;
};

//-----------------------------------------------------------------------------
// Purpose: CReplayInputPanel implementation
//-----------------------------------------------------------------------------
CReplayInputPanel::CReplayInputPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay )
:	BaseClass( pParent, pName ),
	m_hReplay( hReplay ),
	m_pDlg( NULL ),
	m_pTitleEntry( NULL )
{
	SetScheme( "ClientScheme" );
	SetProportional( true );
}

CReplayInputPanel::~CReplayInputPanel()
{
}

void CReplayInputPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replayinputpanel.res", "GAME" );

	// Cache off the dlg pointer
	m_pDlg = FindChildByName( "Dlg" );

	// Setup some action sigsies
	m_pDlg->FindChildByName( "SaveButton" )->AddActionSignalTarget( this );
	m_pDlg->FindChildByName( "CancelButton" )->AddActionSignalTarget( this );

	m_pTitleEntry = static_cast< TextEntry * >( m_pDlg->FindChildByName( "TitleInput" ) );
	m_pTitleEntry->SelectAllOnFocusAlways( true );
	m_pTitleEntry->SetSelectionBgColor( GetSchemeColor( "Yellow", Color( 255, 255, 255, 255), pScheme ) );
	m_pTitleEntry->SetSelectionTextColor( Color( 255, 255, 255, 255 ) );

	if ( m_hReplay != REPLAY_HANDLE_INVALID )
	{
		CReplay *pReplay = g_pReplayManager->GetReplay( m_hReplay );
		m_pTitleEntry->SetText( pReplay->m_wszTitle );
	}
}

void CReplayInputPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	SetWide( ScreenWidth() );
	SetTall( ScreenHeight() );

	// Center
	m_pDlg->SetPos( ( ScreenWidth() - m_pDlg->GetWide() ) / 2, ( ScreenHeight() - m_pDlg->GetTall() ) / 2 );
}

void CReplayInputPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		OnCommand( "cancel" );
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CReplayInputPanel::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_ENTER )
	{
		OnCommand( "save" );
	}

	BaseClass::OnKeyCodePressed( code );
}

void CReplayInputPanel::OnSetFocus()
{
	m_pTitleEntry->RequestFocus();
}

void CReplayInputPanel::OnCommand( const char *command )
{
	bool bCloseWindow = false;
	bool bLocalPlayerDead = false;
	if ( !Q_strnicmp( command, "save", 4 ) )
	{
		if ( m_hReplay != REPLAY_HANDLE_INVALID )
		{
			// Store the title
			CReplay *pReplay = g_pReplayManager->GetReplay( m_hReplay );
			if ( pReplay )
			{
				m_pTitleEntry->GetText( pReplay->m_wszTitle, sizeof( pReplay->m_wszTitle ) );
			}

			// Cache to disk
			g_pReplayManager->FlagReplayForFlush( pReplay, false );

			// Add the replay to the browser
			CReplayBrowserPanel* pReplayBrowser = ReplayUI_GetBrowserPanel();
			if ( pReplayBrowser )
			{
				pReplayBrowser->OnSaveReplay( m_hReplay );
			}

			// Display a message - if we somehow disconnect, we can crash here if local player isn't checked
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				g_pClientMode->DisplayReplayMessage( pLocalPlayer->IsAlive() ? "#Replay_ReplaySavedAlive" : "#Replay_ReplaySavedDead", -1.0f, false, "replay\\saved.wav", false );

				// Check to see if player's dead - used later to determine if we should show items window
				bLocalPlayerDead = !pLocalPlayer->IsAlive();
			}
		}
		bCloseWindow = true;
	}
	else if ( !Q_strnicmp( command, "cancel", 6 ) )
	{
		bCloseWindow = true;
	}

	// Close the window?
	if ( bCloseWindow )
	{
		s_bPanelVisible = false;
		SetVisible( false );
		TFModalStack()->PopModal( this );
		MarkForDeletion();

		// This logic is perhaps a smidge of a hack.  We have to be careful about executing "gameui_hide"
		// since it will hide the item pickup panel.  If there are no items to be picked up, we can safely
		// hide the gameui panel, but we have to call CheckForRoomAndForceDiscard() (as ShowItemsPickedUp()
		// does if no items are picked up).  Otherwise, skip the "gameui_hide" call and show the item pickup
		// panel.
#if defined( TF_CLIENT_DLL )
		if ( TFInventoryManager()->GetNumItemPickedUpItems() == 0 )
		{
			TFInventoryManager()->CheckForRoomAndForceDiscard();
			engine->ClientCmd_Unrestricted( "gameui_hide" );
		}
		else if ( bLocalPlayerDead )
		{
			// Now show the items pickup screen if player's dead
			TFInventoryManager()->ShowItemsPickedUp();
		}
#endif
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsReplayInputPanelVisible()
{
	return s_bPanelVisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShowReplayInputPanel( ReplayHandle_t hReplay )
{
	vgui::DHANDLE< CReplayInputPanel > hReplayInputPanel;

	hReplayInputPanel = vgui::SETUP_PANEL( new CReplayInputPanel( NULL, "ReplayInputPanel", hReplay ) );
	hReplayInputPanel->SetVisible( true );
	hReplayInputPanel->MakePopup();
	hReplayInputPanel->MoveToFront();
	hReplayInputPanel->SetKeyBoardInputEnabled(true);
	hReplayInputPanel->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( hReplayInputPanel );
	engine->ClientCmd_Unrestricted( "gameui_hide" );
	s_bPanelVisible = true;
}

//-----------------------------------------------------------------------------
// Purpose: Test the replay input dialog
//-----------------------------------------------------------------------------
CON_COMMAND_F( open_replayinputpanel, "Open replay input panel test", FCVAR_NONE )
{
	ShowReplayInputPanel( NULL );
}

#endif
