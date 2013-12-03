//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui_controls/TextEntry.h"

#include "replaybrowserbasepage.h"
#include "replaybrowserdetailspanel.h"
#include "replaybrowsermainpanel.h"
#include "replaybrowserlistpanel.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplaymanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;

//-----------------------------------------------------------------------------

CReplayBrowserBasePage::CReplayBrowserBasePage( Panel *pParent )
:	BaseClass( pParent, "BasePage" )
{
	m_pReplayList = new CReplayListPanel( this, "ReplayList" );
	m_pReplayList->SetFirstColumnWidth( 0 );

	m_pSearchTextEntry = new vgui::TextEntry( this, "SearchTextEntry" );
	m_pSearchTextEntry->SelectAllOnFocusAlways( true );
	m_pSearchTextEntry->AddActionSignalTarget( this );
	m_pSearchTextEntry->SetCatchEnterKey( true );

	InvalidateLayout( true, true );

	m_pReplayList->AddReplaysToList();

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CReplayBrowserBasePage::~CReplayBrowserBasePage()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CReplayBrowserBasePage::OnTick()
{
	if ( !IsVisible() )
		return;

	int nCursorX, nCursorY;
	input()->GetCursorPos( nCursorX, nCursorY );

	if ( input()->IsMouseDown( MOUSE_LEFT ) &&
		 !m_pSearchTextEntry->IsWithin( nCursorX, nCursorY ) &&
		 m_pSearchTextEntry->HasFocus() )
	{
		RequestFocus();
	}
}

void CReplayBrowserBasePage::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/basepage.res", "GAME" );

	m_pSearchTextEntry->SetText( "#Replay_SearchText" );
}

void CReplayBrowserBasePage::OnPageShow()
{
	BaseClass::OnPageShow();
	m_pSearchTextEntry->SetText( "#Replay_SearchText" );
}

void CReplayBrowserBasePage::OnSelectionStarted()
{
	PostActionSignal( new KeyValues("SelectionUpdate", "open", 1 ) );
}

void CReplayBrowserBasePage::OnSelectionEnded()
{
	PostActionSignal( new KeyValues("SelectionUpdate", "open", 0 ) );
}

void CReplayBrowserBasePage::CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem )
{
	m_pReplayList->CleanupUIForReplayItem( hReplayItem );
}

void CReplayBrowserBasePage::AddReplay( ReplayHandle_t hReplay )
{
	m_pReplayList->AddReplayItem( hReplay );
}

void CReplayBrowserBasePage::DeleteReplay( ReplayHandle_t hReplayItem )
{
	IReplayItemManager *pItemManager;
	if ( FindReplayItem( hReplayItem, &pItemManager ) )
	{
		ReplayUI_GetBrowserPanel()->AttemptToDeleteReplayItem( this, hReplayItem, pItemManager, -1 );
	}
}

void CReplayBrowserBasePage::OnCancelSelection()
{
}

void CReplayBrowserBasePage::GoBack()
{
	DeleteDetailsPanelAndShowReplayList();
}

void CReplayBrowserBasePage::OnReplayItemDeleted( KeyValues *pParams )
{
	GoBack();
}

void CReplayBrowserBasePage::OnTextChanged( KeyValues *data )
{
	wchar_t wszText[256];
	m_pSearchTextEntry->GetText( wszText, ARRAYSIZE( wszText ) );
	m_pReplayList->ApplyFilter( wszText );
	InvalidateLayout();
}

void CReplayBrowserBasePage::OnCommand( const char *pCommand )
{
	// User wants details on a replay?
	if ( !V_strnicmp( pCommand, "details", 7 ) )
	{
		// Get rid of preview panel
		m_pReplayList->ClearPreviewPanel();

		QueryableReplayItemHandle_t hReplayItem = (QueryableReplayItemHandle_t)atoi( pCommand + 7 );
		IReplayItemManager *pItemManager;
		IQueryableReplayItem *pReplayItem = FindReplayItem( hReplayItem, &pItemManager );		Assert( pReplayItem );
		if ( pReplayItem )
		{
			// Get performance
			int iPerformance = -1;
			const char *pPerformanceStr = V_strstr( pCommand + 8, "_" );
			if ( pPerformanceStr )
			{
				iPerformance = atoi( pPerformanceStr + 1 );
			}

			m_hReplayDetailsPanel = vgui::SETUP_PANEL( new CReplayDetailsPanel( this, hReplayItem, iPerformance, pItemManager ) );
			m_hReplayDetailsPanel->SetVisible( true );
			m_hReplayDetailsPanel->MoveToFront();

			m_pReplayList->SetVisible( false );

			surface()->PlaySound( "replay\\showdetails.wav" );
		}
	}

	// "back" button was hit in details panel?
	else if ( FStrEq( pCommand, "back" ) )
	{
		GoBack();
	}

	BaseClass::OnCommand( pCommand );
}

void CReplayBrowserBasePage::DeleteDetailsPanelAndShowReplayList()
{
	// Delete the panel
	if ( m_hReplayDetailsPanel )
	{
		m_hReplayDetailsPanel->MarkForDeletion();
		m_hReplayDetailsPanel = NULL;
	}

	m_pReplayList->SetVisible( true );
}

void CReplayBrowserBasePage::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pSearchTextEntry )
	{
		const bool bHasReplays = g_pReplayManager && g_pReplayManager->GetReplayCount();
		const bool bHasMovies = g_pReplayMovieManager && g_pReplayMovieManager->GetMovieCount();
		int aListPos[2];
		m_pReplayList->GetPos( aListPos[0], aListPos[1] );
		m_pSearchTextEntry->SetPos( aListPos[0] + m_pReplayList->GetWide() - m_pSearchTextEntry->GetWide(), YRES( 5 ) );
		m_pSearchTextEntry->SetVisible( bHasReplays || bHasMovies );
	}

	// Invalidate the list too, because we might be laying out due to a replay being removed from the list.
	m_pReplayList->InvalidateLayout();
}

void CReplayBrowserBasePage::FreeDetailsPanelMovieLock()
{
	m_hReplayDetailsPanel->FreeMovieFileLock();
}

bool CReplayBrowserBasePage::IsDetailsViewOpen()
{
	return m_hReplayDetailsPanel.Get() != NULL && m_hReplayDetailsPanel->IsVisible();
}

#endif