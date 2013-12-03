//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowsermainpanel.h"
#include "replaybrowserbasepage.h"
#include "confirm_delete_dialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/TextImage.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "ienginevgui.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplaymoviemanager.h"
#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Replay deletion confirmation dialog
//-----------------------------------------------------------------------------
class CConfirmDeleteReplayDialog : public CConfirmDeleteDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDeleteReplayDialog, CConfirmDeleteDialog );
public:
	CConfirmDeleteReplayDialog( Panel *pParent, IReplayItemManager *pItemManager, int iPerformance )
	:	BaseClass( pParent )
	{
		m_pTextId = iPerformance >= 0 ? "#Replay_DeleteEditConfirm" : pItemManager->AreItemsMovies() ? "#Replay_DeleteMovieConfirm" : "#Replay_DeleteReplayConfirm";
	}

	const wchar_t *GetText()
	{
		return g_pVGuiLocalize->Find( m_pTextId );
	}

	const char *m_pTextId;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CReplayBrowserPanel::CReplayBrowserPanel( Panel *parent )
:	PropertyDialog(parent, "ReplayBrowser"),
	m_pConfirmDeleteDialog( NULL )
{
	// Clear out delete info
	V_memset( &m_DeleteInfo, 0, sizeof( m_DeleteInfo ) );

	// Replay browser is parented to the game UI panel
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	// Setup page
	m_pReplaysPage = new CReplayBrowserBasePage( this );
	m_pReplaysPage->AddActionSignalTarget( this );

	AddPage( m_pReplaysPage, "#Replay_MyReplays" );

	m_pReplaysPage->SetVisible( true );

	ListenForGameEvent( "gameui_hidden" );

	// Create this now, so that it can be the default button (if created in .res file, it fights with PropertyDialog's OkButton & generates asserts)
	CExButton *pCloseButton = new CExButton( this, "BackButton", "" );
	GetFocusNavGroup().SetDefaultButton(pCloseButton);

	m_flTimeOpened = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CReplayBrowserPanel::~CReplayBrowserPanel()
{
	if ( m_pConfirmDeleteDialog )
	{
		m_pConfirmDeleteDialog->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/mainpanel.res", "GAME" );

	SetOKButtonVisible(false);
	SetCancelButtonVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::ShowPanel(bool bShow, ReplayHandle_t hReplayDetails/*=REPLAY_HANDLE_INVALID*/,
									int iPerformance/*=-1*/ )
{
	if ( bShow )
	{
		GetPropertySheet()->SetActivePage( m_pReplaysPage );
		InvalidateLayout( false, true );
		Activate();

		m_flTimeOpened = gpGlobals->realtime;
	}
	else
	{
		PostMessage( m_pReplaysPage, new KeyValues("CancelSelection") );
	}

	SetVisible( bShow );
	m_pReplaysPage->SetVisible( bShow );

	if ( hReplayDetails != REPLAY_HANDLE_INVALID )
	{
		char szDetails[32];
		V_snprintf( szDetails, sizeof( szDetails ), "details%i_%i", (int)hReplayDetails, iPerformance );
		m_pReplaysPage->OnCommand( szDetails );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		ShowPanel( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "back" ) )
	{
		if ( m_pReplaysPage->IsDetailsViewOpen() )
		{
			m_pReplaysPage->DeleteDetailsPanelAndShowReplayList();
		}
		else
		{
			// Close the main panel
			ShowPanel( false );

			// TODO: Properly manage the browser so that we don't have to recreate it ever time its opened
			MarkForDeletion();

			// If we're connected to a game server, we also close the game UI.
			if ( engine->IsInGame() )
			{
				engine->ClientCmd_Unrestricted( "gameui_hide" );
			}
		}
	}

	BaseClass::OnCommand( command );
}

void CReplayBrowserPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		ShowPanel( false );
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	if ( GetBaseButtonCode( code ) == KEY_XBUTTON_B )
	{
		ShowPanel( false );
	}
	else if ( code == KEY_ENTER )
	{
		// do nothing, the default is to close the panel!
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::ShowDeleteReplayDenialDlg()
{
	ShowMessageBox( "#Replay_DeleteDenialTitle", "#Replay_DeleteDenialText", "#GameUI_OK" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::AttemptToDeleteReplayItem( Panel *pHandler, ReplayItemHandle_t hReplayItem,
													 IReplayItemManager *pItemManager, int iPerformance )
{
	IQueryableReplayItem *pItem = pItemManager->GetItem( hReplayItem );
	CGenericClassBasedReplay *pReplay = ToGenericClassBasedReplay( pItem->GetItemReplay() );

	// If this is an actual replay the user is trying to delete, only allow it
	// if the replay says it's OK.  Don't execute this code for performances.
	if ( !pItemManager->AreItemsMovies() && iPerformance < 0 && !pReplay->ShouldAllowDelete() )
	{
		ShowDeleteReplayDenialDlg();
		return;
	}

	// Otherwise, show the confirm delete dlg
	vgui::surface()->PlaySound( "replay\\replaydialog_warn.wav" );
	ConfirmReplayItemDelete( pHandler, hReplayItem, pItemManager, iPerformance );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::ConfirmReplayItemDelete( Panel *pHandler, ReplayItemHandle_t hReplayItem,
												   IReplayItemManager *pItemManager, int iPerformance )
{
	CConfirmDeleteReplayDialog *pConfirm = vgui::SETUP_PANEL( new CConfirmDeleteReplayDialog( this, pItemManager, iPerformance ) );
	if ( pConfirm )
	{
		// Cache replay and handler for later
		m_DeleteInfo.m_hReplayItem = hReplayItem;
		m_DeleteInfo.m_pItemManager = pItemManager;
		m_DeleteInfo.m_hHandler = pHandler->GetVPanel();
		m_DeleteInfo.m_iPerformance = iPerformance;

		// Display the panel!
		pConfirm->Show();

		// Cache confirm dialog ptr
		m_pConfirmDeleteDialog = pConfirm;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::OnConfirmDelete( KeyValues *data )
{
	// Clear confirm ptr
	m_pConfirmDeleteDialog = NULL;

	// User confirmed delete?
	int nConfirmed = data->GetInt( "confirmed", 0 );
	if ( !nConfirmed )
		return;

	// Get the replay from the dialog
	ReplayItemHandle_t hReplayItem = m_DeleteInfo.m_hReplayItem;

	// Post actions signal to the handler
	KeyValues *pMsg = new KeyValues( "ReplayItemDeleted" );
	pMsg->SetInt( "replayitem", (int)hReplayItem );
	pMsg->SetInt( "perf", m_DeleteInfo.m_iPerformance );
	PostMessage( m_DeleteInfo.m_hHandler, pMsg );

	// Delete actual replay item
	if ( m_DeleteInfo.m_iPerformance < 0 )
	{
		// Cleanup UI related to the replay/movie
		CleanupUIForReplayItem( hReplayItem );

		// Delete the replay/movie
		m_DeleteInfo.m_pItemManager->DeleteItem( GetActivePage(), hReplayItem, false );
	}

	vgui::surface()->PlaySound( "replay\\deleted_take.wav" );

	// Clear delete info
	V_memset( &m_DeleteInfo, 0, sizeof( m_DeleteInfo ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::OnSaveReplay( ReplayHandle_t hNewReplay )
{
	// Verify that the handle is valid
	Assert( g_pReplayManager->GetReplay( hNewReplay ) );

	m_pReplaysPage->AddReplay( hNewReplay );
	m_pReplaysPage->Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::OnDeleteReplay( ReplayHandle_t hDeletedReplay )
{
	// Verify that the handle is valid
	Assert( g_pReplayManager->GetReplay( hDeletedReplay ) );

	DeleteReplay( hDeletedReplay );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::DeleteReplay( ReplayHandle_t hReplay )
{
	m_pReplaysPage->DeleteReplay( hReplay );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayBrowserPanel::CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem )
{
	if ( GetActivePage() == m_pReplaysPage )
	{
		m_pReplaysPage->CleanupUIForReplayItem( hReplayItem );
	}
}

static vgui::DHANDLE<CReplayBrowserPanel> g_ReplayBrowserPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CReplayBrowserPanel *ReplayUI_OpenReplayBrowserPanel( ReplayHandle_t hReplayDetails,
													  int iPerformance )
{
	if ( !g_ReplayBrowserPanel.Get() )
	{
		g_ReplayBrowserPanel = vgui::SETUP_PANEL( new CReplayBrowserPanel( NULL ) );
		g_ReplayBrowserPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_ReplayBrowserPanel->ShowPanel( true, hReplayDetails, iPerformance );

	extern IReplayMovieManager *g_pReplayMovieManager;
	if ( g_pReplayMovieManager->GetMovieCount() > 0 )
	{
		// Fire a message the game DLL can intercept (for achievements, etc).
		IGameEvent *event = gameeventmanager->CreateEvent( "browse_replays" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	return g_ReplayBrowserPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CReplayBrowserPanel *ReplayUI_GetBrowserPanel( void )
{
	return g_ReplayBrowserPanel.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ReplayUI_CloseReplayBrowser()
{
	if ( g_ReplayBrowserPanel )
	{
		g_ReplayBrowserPanel->MarkForDeletion();
		g_ReplayBrowserPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ReplayUI_ReloadBrowser( ReplayHandle_t hReplay/*=REPLAY_HANDLE_INVALID*/,
							 int iPerformance/*=-1*/ )
{
	delete g_ReplayBrowserPanel.Get();
	g_ReplayBrowserPanel = NULL;
	ReplayUI_OpenReplayBrowserPanel( hReplay, iPerformance );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CON_COMMAND_F( open_replaybrowser, "Open the replay browser.", FCVAR_CLIENTDLL )
{
	ReplayUI_OpenReplayBrowserPanel( REPLAY_HANDLE_INVALID, -1 );
	g_ReplayBrowserPanel->InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CON_COMMAND_F( replay_reloadbrowser, "Reloads replay data and display replay browser", FCVAR_CLIENTDLL | FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	ReplayUI_ReloadBrowser();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CON_COMMAND_F( replay_hidebrowser, "Hides replay browser", FCVAR_CLIENTDLL )
{
	ReplayUI_CloseReplayBrowser();
}

#endif