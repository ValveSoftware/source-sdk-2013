//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayperformancesavedlg.h"
#include "replay/performance.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplayperformancecontroller.h"
#include "replay/replay.h"
#include "econ/confirm_dialog.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/TextImage.h"
#include "vgui/ISurface.h"
#include "replay/replaycamera.h"
#include "replayperformanceeditor.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Player input dialog for a replay
//-----------------------------------------------------------------------------
class CReplayPerformanceSaveDlg	: public EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CReplayPerformanceSaveDlg, EditablePanel );

public:
	CReplayPerformanceSaveDlg( Panel *pParent, const char *pName,
		OnConfirmSaveCallback pfnCallback, void *pContext, CReplay *pReplay, bool bExitEditorWhenDone );
	~CReplayPerformanceSaveDlg();

	static void Show( OnConfirmSaveCallback pfnCallback, void *pContext, CReplay *pReplay,
		bool bExitEditorWhenDone );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( KeyCode code );
	virtual void OnKeyCodeTyped( KeyCode code );

	bool		ConfirmOverwriteOrSaveNow();
	void		CloseWindow();

	static void	OnConfirmOverwrite( bool bConfirm, void *pContext );

	MESSAGE_FUNC( OnSetFocus, "SetFocus" );

	static vgui::DHANDLE< CReplayPerformanceSaveDlg > ms_hDlg;

private:
	OnConfirmSaveCallback	m_pfnCallback;
	void					*m_pContext;
	Panel					*m_pDlg;
	CReplay					*m_pReplay;
	TextEntry				*m_pTitleEntry;
	bool					m_bExitEditorWhenDone;
	wchar_t					m_wszTitle[ MAX_TAKE_TITLE_LENGTH ];
};

vgui::DHANDLE< CReplayPerformanceSaveDlg > CReplayPerformanceSaveDlg::ms_hDlg;

//-----------------------------------------------------------------------------
// Purpose: CReplayPerformanceSaveDlg implementation
//-----------------------------------------------------------------------------
CReplayPerformanceSaveDlg::CReplayPerformanceSaveDlg( Panel *pParent, const char *pName,
													  OnConfirmSaveCallback pfnCallback, void *pContext,
													  CReplay *pReplay, bool bExitEditorWhenDone )
:	BaseClass( pParent, pName ),
	m_pfnCallback( pfnCallback ),
	m_pContext( pContext ),
	m_pReplay( pReplay ),
	m_bExitEditorWhenDone( bExitEditorWhenDone ),
	m_pDlg( NULL ),
	m_pTitleEntry( NULL )
{
	Assert( m_pContext );

	SetScheme( "ClientScheme" );
	SetProportional( true );
}

CReplayPerformanceSaveDlg::~CReplayPerformanceSaveDlg()
{
	ms_hDlg = NULL;
}

/*static*/ void CReplayPerformanceSaveDlg::Show( OnConfirmSaveCallback pfnCallback, void *pContext, CReplay *pReplay,
												 bool bExitEditorWhenDone )
{
	Assert( !ms_hDlg.Get() );

	ms_hDlg = vgui::SETUP_PANEL( new CReplayPerformanceSaveDlg( NULL, "ReplayInputPanel", pfnCallback, pContext, pReplay, bExitEditorWhenDone ) );
	ms_hDlg->SetVisible( true );
	ms_hDlg->MakePopup();
	ms_hDlg->MoveToFront();
	ms_hDlg->SetKeyBoardInputEnabled(true);
	ms_hDlg->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( ms_hDlg );
	engine->ClientCmd_Unrestricted( "gameui_hide" );

	ReplayCamera()->EnableInput( false );
}

void CReplayPerformanceSaveDlg::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replayperformanceeditor/savedlg.res", "GAME" );

	// Cache off the dlg pointer
	m_pDlg = FindChildByName( "Dlg" );

	CExButton *pDiscardButton;
	pDiscardButton = dynamic_cast< CExButton * >( m_pDlg->FindChildByName( "DiscardButton" ) );
	SetXToRed( pDiscardButton );

	// Setup some action sigs
	m_pDlg->FindChildByName( "SaveButton" )->AddActionSignalTarget( this );
	m_pDlg->FindChildByName( "CancelButton" )->AddActionSignalTarget( this );
	pDiscardButton->AddActionSignalTarget( this );

	m_pTitleEntry = static_cast< TextEntry * >( m_pDlg->FindChildByName( "TitleInput" ) );
	m_pTitleEntry->SelectAllOnFocusAlways( true );
	m_pTitleEntry->SetSelectionBgColor( GetSchemeColor( "Yellow", Color( 255, 255, 255, 255), pScheme ) );
	m_pTitleEntry->SetSelectionTextColor( Color( 255, 255, 255, 255 ) );
	m_pTitleEntry->SetText( L"" );
}

void CReplayPerformanceSaveDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	SetWide( ScreenWidth() );
	SetTall( ScreenHeight() );

	// Center
	m_pDlg->SetPos( ( ScreenWidth() - m_pDlg->GetWide() ) / 2, ( ScreenHeight() - m_pDlg->GetTall() ) / 2 );
}

void CReplayPerformanceSaveDlg::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		surface()->PlaySound( "replay\\record_fail.wav" );
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CReplayPerformanceSaveDlg::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_ENTER )
	{
		OnCommand( "save" );
	}

	BaseClass::OnKeyCodePressed( code );
}

void CReplayPerformanceSaveDlg::OnSetFocus()
{
	m_pTitleEntry->RequestFocus();
}

/*static*/ void CReplayPerformanceSaveDlg::OnConfirmOverwrite( bool bConfirm, void *pContext )
{
	CReplayPerformanceSaveDlg *pThis = (CReplayPerformanceSaveDlg *)pContext;
	pThis->m_pfnCallback( bConfirm, pThis->m_wszTitle, pThis->m_pContext );
	pThis->CloseWindow();
}

bool CReplayPerformanceSaveDlg::ConfirmOverwriteOrSaveNow()
{
	// Using the same title as an existing performance?
	CReplayPerformance *pExistingPerformance = m_pReplay->GetPerformanceWithTitle( m_wszTitle );
	if ( pExistingPerformance )
	{
		ShowConfirmDialog( "#Replay_OverwriteDlgTitle", "#Replay_OverwriteDlgText",
			"#Replay_ConfirmOverwrite", "#Replay_Cancel", OnConfirmOverwrite, NULL, this );
		return false;
	}

	m_pfnCallback( true, m_wszTitle, m_pContext );

	return true;
}

void CReplayPerformanceSaveDlg::OnCommand( const char *command )
{
	bool bCloseWindow = false;

	extern IReplayPerformanceController *g_pReplayPerformanceController;

	if ( !Q_strnicmp( command, "save", 4 ) )
	{
		// Get the text and save the replay/performance immediately
		m_pTitleEntry->GetText( m_wszTitle, MAX_TAKE_TITLE_LENGTH );

		// If we aren't overwriting an existing performance, this func will return true.
		bCloseWindow = ConfirmOverwriteOrSaveNow();
	}
	else if ( !Q_strnicmp( command, "cancel", 6 ) )
	{
		bCloseWindow = true;
	}

	// Close the window?
	if ( bCloseWindow )
	{
		CloseWindow();
	}

	BaseClass::OnCommand( command );
}

void CReplayPerformanceSaveDlg::CloseWindow()
{
	SetVisible( false );
	MarkForDeletion();
	TFModalStack()->PopModal( ms_hDlg.Get() );
	ReplayCamera()->EnableInput( true );

	CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
	if ( m_bExitEditorWhenDone && pEditor )
	{
		pEditor->Exit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ReplayUI_ShowPerformanceSaveDlg( OnConfirmSaveCallback pfnCallback,
									  void *pContext, CReplay *pReplay,
									  bool bExitEditorWhenDone )
{
	CReplayPerformanceSaveDlg::Show( pfnCallback, pContext, pReplay, bExitEditorWhenDone );
}

bool ReplayUI_IsPerformanceSaveDlgOpen()
{
	return CReplayPerformanceSaveDlg::ms_hDlg.Get() != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Test the replay input dialog
//-----------------------------------------------------------------------------
CON_COMMAND_F( replay_test_take_save_dlg, "Open replay save take dlg", FCVAR_NONE )
{
	ReplayUI_ShowPerformanceSaveDlg( NULL, NULL, NULL, false );
}

#endif
