//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowserlistitempanel.h"
#include "replaybrowsermainpanel.h"
#include "replaybrowserlistpanel.h"
#include "replaybrowserrenderdialog.h"
#include "replaybrowsermovieplayerpanel.h"
#include "vgui/IInput.h"
#include "vgui/IVGui.h"
#include "filesystem.h"
#include "replay/screenshot.h"
#include "replay/ireplaymanager.h"
#include "confirm_dialog.h"
#include "replay/ireplaymoviemanager.h"
#include "econ/econ_controls.h"

//-----------------------------------------------------------------------------

using namespace vgui;

extern IReplayMovieManager *g_pReplayMovieManager;

//-----------------------------------------------------------------------------

#define REPLAY_BORDER_WIDTH		XRES(2)
#define REPLAY_BORDER_HEIGHT	YRES(2)
#define REPLAY_BUFFER_HEIGHT	XRES(5)

//-----------------------------------------------------------------------------

CReplayScreenshotSlideshowPanel::CReplayScreenshotSlideshowPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay )
:	CSlideshowPanel( pParent, pName ),
	m_hReplay( hReplay )
{
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );

	// Add all screenshots for the given replay
	char szImage[ MAX_OSPATH ];
	for ( int i = 0; i < pReplay->GetScreenshotCount(); ++i )
	{
		const CReplayScreenshot *pScreenshot = pReplay->GetScreenshot( i );
		V_snprintf( szImage, sizeof( szImage ), "replay/thumbnails/%s.vtf", pScreenshot->m_szBaseFilename );

		// Add image to list of slideshow images
		AddImage( szImage );

		if ( i == 0 )
		{
			// Set first image
			GetImagePanel()->SetImage( szImage );
		}
	}
}

void CReplayScreenshotSlideshowPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------

CReplayBrowserThumbnail::CReplayBrowserThumbnail( Panel *pParent, const char *pName, QueryableReplayItemHandle_t hReplayItem,
												  IReplayItemManager *pReplayItemManager )
:	CReplayBasePanel( pParent, pName ),
	m_hReplayItem( hReplayItem ),
	m_pReplayItemManager( pReplayItemManager ),
	m_bMouseOver( false ),
	m_pMoviePlayer( NULL ),
	m_flLastMovieScrubTime( 0.0f ),
	m_flHoverStartTime( 0.0f ),
	m_flLastProgressChangeTime( 0.0f )
{
	SetScheme( "ClientScheme" );

	ivgui()->AddTickSignal( GetVPanel(), 10 );

	// Add the list panel as an action signal target
	// NOTE: Vile hack.
	Panel *pTarget = GetParent();
	while ( pTarget )
	{
		if ( !V_strcmp( "BasePage", pTarget->GetName() ) )
			break;
		pTarget = pTarget->GetParent();
	}
	Assert( pTarget );
	AddActionSignalTarget( pTarget );

	m_pScreenshotThumb = new CCrossfadableImagePanel( this, "ScreenshotThumbnail" );
	m_pTitle = new Label( this, "TitleLabel", "" );
	m_pDownloadLabel = new Label( this, "DownloadLabel", "" );
	m_pRecordingInProgressLabel = new Label( this, "RecordingInProgressLabel", "" );
	m_pDownloadProgress = new ProgressBar( this, "DownloadProgress" );
	m_pDownloadButton = new CExButton( this, "DownloadButton", "#Replay_Download" );
	m_pDeleteButton = new CExButton( this, "DeleteButton", "#X_Delete" );
	m_pErrorLabel = new Label( this, "ErrorLabel", "" );
	m_pDownloadOverlay = new Panel( this, "DownloadOverlay" );
	m_pBorderPanel = new EditablePanel( this, "BorderPanel" );

	m_pScreenshotThumb->InstallMouseHandler( this );

	m_pDownloadButton->AddActionSignalTarget( this );
	m_pDownloadButton->SetCommand( new KeyValues( "download" ) );

	m_pDeleteButton->AddActionSignalTarget( this );
	m_pDeleteButton->SetCommand( new KeyValues( "delete_replayitem" ) );

	SetProportional( true );

	SetReplayItem( hReplayItem );
}

CReplayBrowserThumbnail::~CReplayBrowserThumbnail()
{
	// Clear the download event handler ptr
	CGenericClassBasedReplay *pReplay = GetReplay();
	if ( pReplay )
	{
		pReplay->m_pDownloadEventHandler = NULL;
	}

	SetupReplayItemUserData( NULL );

	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CReplayBrowserThumbnail::SetReplayItem( QueryableReplayItemHandle_t hReplayItem )
{ 
	if ( m_hReplayItem != REPLAY_HANDLE_INVALID && m_hReplayItem != hReplayItem )
	{
		IQueryableReplayItem *pReplayItem = m_pReplayItemManager->GetItem( m_hReplayItem );
		if ( pReplayItem )
		{
			pReplayItem->SetUserData( NULL );

			CGenericClassBasedReplay *pTFReplay = ToGenericClassBasedReplay( pReplayItem->GetItemReplay() );
			pTFReplay->m_pDownloadEventHandler = NULL;
		}
	}

	m_hReplayItem = hReplayItem; 

	if ( hReplayItem != REPLAY_HANDLE_INVALID )
	{
		// Set this as user data
		SetupReplayItemUserData( (void *)this );
	}

	InvalidateLayout();
}

void CReplayBrowserThumbnail::SetupReplayItemUserData( void *pUserData )
{
	IQueryableReplayItem *pReplayItem = m_pReplayItemManager->GetItem( m_hReplayItem );
	if ( pReplayItem )
	{
		pReplayItem->SetUserData( pUserData );
	}
}

void CReplayBrowserThumbnail::OnTick()
{
	const CGenericClassBasedReplay *pReplay = GetReplay();
	if ( !pReplay )
		return;

	// Get out if the delete confirmation dialog is up
	if ( vgui::input()->GetAppModalSurface() )
		return;

	// Need to update layout if status has changed, since some buttons may need to be shifted around
	// TODO: Only layout when necessary
	InvalidateLayout( true, false );

	// Get mouse position and store state
	int x,y;
	vgui::input()->GetCursorPos(x, y);
	bool bOldMouseOver = m_bMouseOver;
	m_bMouseOver = m_pBorderPanel->IsWithin(x,y);

	// If we are just starting to hover over the thumbnail, mark the time
	if ( bOldMouseOver != m_bMouseOver && m_bMouseOver )
	{
		m_flHoverStartTime = gpGlobals->realtime;
	}

	const char *pBorderName = m_bMouseOver ? "ReplayHighlightBorder" : "ReplayDefaultBorder";
	IBorder *pBorder = scheme()->GetIScheme( GetScheme() )->GetBorder( pBorderName );
	m_pBorderPanel->SetBorder( pBorder );

	// Set visibility of buttons and such - a player may have saved their replay but not died yet,
	// in which case pReplay->m_bComplete will be false.
	const IQueryableReplayItem *pItem = m_pReplayItemManager->GetItem( m_hReplayItem );
	bool bIncomplete = !pReplay->m_bComplete;
	bool bDownloadPhase = !pItem->IsItemAMovie() && pReplay->m_nStatus == CReplay::REPLAYSTATUS_DOWNLOADPHASE;
	bool bErrorState = pReplay->m_nStatus == CReplay::REPLAYSTATUS_ERROR;

	m_pDownloadButton->SetVisible( false );
	m_pDeleteButton->SetVisible( bErrorState || bDownloadPhase );
	m_pDownloadOverlay->SetVisible( bErrorState || bDownloadPhase || bIncomplete );
	m_pDownloadProgress->SetVisible( bDownloadPhase );
	m_pErrorLabel->SetVisible( bErrorState );
	m_pRecordingInProgressLabel->SetVisible( bIncomplete );

	UpdateProgress( bDownloadPhase, pReplay );
}

void CReplayBrowserThumbnail::UpdateProgress( bool bDownloadPhase, const CReplay *pReplay )
{
	if ( !bDownloadPhase )
		return;

	// Get current download progress
	const float flProgress = g_pClientReplayContext->GetReplayManager()->GetDownloadProgress( pReplay );

	// Has progress changed?
	const int nNewProgress = 100 * flProgress;
	const int nOldProgress = 100 * m_pDownloadProgress->GetProgress();
	const float flCurTime = gpGlobals->realtime;
	if ( nNewProgress != nOldProgress )
	{
		m_flLastProgressChangeTime = flCurTime;
	}

	// Set download progress
	m_pDownloadProgress->SetProgress( flProgress );

	const char *pToken = "#Replay_Waiting";
	if ( ReplayUI_GetBrowserPanel() && ( flCurTime - ReplayUI_GetBrowserPanel()->GetTimeOpened() > 2.0f ) )
	{
		// Use "downloading" string if progress has changed in the last two seconds
		if ( ( flCurTime - m_flLastProgressChangeTime ) < 2.0f )
		{
			pToken = "#Replay_Downloading";
		}
	}

	const wchar_t *pLocalizedText = g_pVGuiLocalize->Find( pToken );
	if ( pLocalizedText )
	{
		// Add animating '...' to end of string
		wchar_t wszText[128] = { 0 };
		V_wcscpy_safe( wszText, pLocalizedText );
		unsigned int nNumPeriods = fmod( gpGlobals->realtime * 2.0f, 4.0f );	// Max of 3 dots
		const unsigned int nLen = wcslen( wszText );
		V_wcscat_safe( wszText, L"..." );
		wszText[ Min( nLen + nNumPeriods, (unsigned int)sizeof( wszText ) - 1 ) ] = L'\0';
		m_pDownloadLabel->SetText( wszText );
		m_pDownloadLabel->SizeToContents();
		m_pDownloadLabel->SetWide( m_pDownloadProgress->GetWide() );
		m_pDownloadLabel->SetPos( 3, (m_pDownloadProgress->GetTall() - m_pDownloadLabel->GetTall()) / 2 );
	}
}

void CReplayBrowserThumbnail::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/listthumbnail.res", "GAME" );

	// Get default from the .res file
	m_clrDefaultBg = GetBgColor();	
	m_clrHighlight = GetSchemeColor( "TanDark", Color( 255, 255, 255, 255 ), pScheme );
}

void CReplayBrowserThumbnail::PerformLayout()
{
	BaseClass::PerformLayout();

	const CGenericClassBasedReplay *pReplay = GetReplay();
	if ( !pReplay )
		return;

	AssertValidReadPtr( pReplay );

	// Get thumbnail for first screenshot
	char szImage[MAX_OSPATH] = { '\0' };
	bool bHasScreenshots = false;
	if ( pReplay->GetScreenshotCount() )
	{
		// Use first screenshot for thumbnail
		bHasScreenshots = true;
		const CReplayScreenshot *pScreenshot = pReplay->GetScreenshot( 0 );
		V_snprintf( szImage, sizeof( szImage ), "replay/thumbnails/%s.vtf", pScreenshot->m_szBaseFilename );
	}

	// See if it exists
	char szSearch[MAX_OSPATH];
	V_snprintf( szSearch, sizeof( szSearch ), "materials/vgui/%s", szImage );
	bool bShowScreenshotThumb = true;
	if ( !bHasScreenshots || !g_pFullFileSystem || !g_pFullFileSystem->FileExists( szSearch, "GAME" ) )
	{
		// Hide it
		bShowScreenshotThumb = false;
	}

	// Scale the screenshot so that it clips off the dead area of the power of 2 texture
	float flScale = pReplay->GetScreenshotCount() == 0 ? 1.0f : ( (float)m_pScreenshotThumb->GetWide() / ( .95f * pReplay->GetScreenshot( 0 )->m_nWidth ) );
	m_pScreenshotThumb->SetScaleAmount( flScale );
	m_pScreenshotThumb->SetShouldScaleImage( true );
	if ( bShowScreenshotThumb )
	{
		// We don't want to actually hide it (via SetVisible()), since we need to get mouse click events from the image panel
		m_pScreenshotThumb->SetImage( szImage );
	}

	// Setup progress bar & label
	m_pDownloadProgress->SetWide( GetWide() * .95f );
	m_pDownloadProgress->SetSegmentInfo( 0, 1 );
	m_pDownloadProgress->SetBarInset( 2 );
	m_pDownloadProgress->SetMargin( 2 );
	m_pDownloadProgress->SetPos(
		(GetWide() - m_pDownloadProgress->GetWide()) / 2,
		(m_pScreenshotThumb->GetTall() - m_pDownloadProgress->GetTall()) / 2
	);

	m_pDownloadLabel->SetParent( m_pDownloadProgress );
	m_pDownloadLabel->SetWide( m_pDownloadProgress->GetWide() );
	m_pDownloadLabel->SetPos( 3, (m_pDownloadProgress->GetTall() - m_pDownloadLabel->GetTall()) / 2 );

	m_pErrorLabel->SizeToContents();
	m_pErrorLabel->SetPos(
		(GetWide() - m_pErrorLabel->GetWide()) / 2,
		(m_pScreenshotThumb->GetTall() - m_pErrorLabel->GetTall()) / 2
	);

	// Center the title control horizontally
	int nThumbX, nThumbY, nThumbW, nThumbH;
	UpdateTitleText();
	m_pScreenshotThumb->GetBounds( nThumbX, nThumbY, nThumbW, nThumbH );

	m_pDownloadOverlay->SetBounds( 0, 0, GetWide(), GetTall() );

	if ( m_pMoviePlayer )
	{
		m_pMoviePlayer->SetBounds( nThumbX, nThumbY, nThumbW, nThumbH );
	}

	// Setup recording-in-progress
	m_pRecordingInProgressLabel->SetBounds( nThumbX, nThumbY, nThumbW, nThumbH );
	m_pRecordingInProgressLabel->InvalidateLayout( true, false );	// Need this for centerwrap to work properly

	bool bDownloadPhase = pReplay->m_nStatus == CReplay::REPLAYSTATUS_DOWNLOADPHASE;
	int nButtonWidth = 9 * GetWide() / 10;
	int nButtonX = nThumbX + ( m_pScreenshotThumb->GetWide() - nButtonWidth ) / 2;
	int nDownloadButtonY, nDeleteButtonY;
	if ( bDownloadPhase )
	{
		nDownloadButtonY = nThumbY + 12;
		nDeleteButtonY = nThumbY + m_pScreenshotThumb->GetTall() - m_pDeleteButton->GetTall() - 12;
	}
	else
	{
		nDownloadButtonY = 0;	// We don't care about this now, since it will not be visible
		nDeleteButtonY = ( m_pScreenshotThumb->GetTall() - m_pDeleteButton->GetTall() ) / 2;
	}

	// Adjust download button position
	m_pDownloadButton->SetPos( nButtonX, nDownloadButtonY );
	m_pDownloadButton->SetWide( nButtonWidth );
}

void CReplayBrowserThumbnail::OnDownloadClicked( KeyValues *pParams )
{
	// Begin the download
	OnCommand( "download" );
}

void CReplayBrowserThumbnail::OnDeleteReplay( KeyValues *pParams )
{
	OnCommand( "delete_replayitem" );
}

void CReplayBrowserThumbnail::OnCommand( const char *pCommand )
{
	const CGenericClassBasedReplay *pReplay = GetReplay();
	AssertValidReadPtr( pReplay );
	if ( !pReplay )
		return;

	if ( FStrEq( pCommand, "details" ) )	// Display replay details?
	{
		char szCmd[256];
		V_snprintf( szCmd, sizeof( szCmd ), "details%i", (int)m_hReplayItem );
		PostActionSignal( new KeyValues( "Command", "command", szCmd ) );
	}
	else if ( FStrEq( pCommand, "delete_replayitem" ) ) // Delete the replay?
	{
		ReplayUI_GetBrowserPanel()->AttemptToDeleteReplayItem( this, m_hReplayItem, m_pReplayItemManager, -1 );
	}
	else
	{
		BaseClass::OnCommand( pCommand );
	}
}

void CReplayBrowserThumbnail::OnMousePressed( MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		OnCommand( "details" );
	}
}

void CReplayBrowserThumbnail::UpdateTitleText()
{
	IQueryableReplayItem *pReplayItem = m_pReplayItemManager->GetItem( m_hReplayItem );
	m_pTitle->SetText( pReplayItem->GetItemTitle() );
}

CGenericClassBasedReplay *CReplayBrowserThumbnail::GetReplay()
{
	IQueryableReplayItem *pItem = m_pReplayItemManager->GetItem( m_hReplayItem );
	if ( !pItem )
		return NULL;

	return ToGenericClassBasedReplay( pItem->GetItemReplay() );
}

IQueryableReplayItem *CReplayBrowserThumbnail::GetReplayItem()
{
	return m_pReplayItemManager->GetItem( m_hReplayItem );
}

//-----------------------------------------------------------------------------

CReplayBrowserThumbnailRow::CReplayBrowserThumbnailRow( Panel *pParent, const char *pName, IReplayItemManager *pReplayItemManager )
:	BaseClass( pParent, pName ),
	m_pReplayItemManager( pReplayItemManager )
{
	SetProportional( true );
}

void CReplayBrowserThumbnailRow::AddReplayThumbnail( const IQueryableReplayItem *pItem )
{
	CReplayBrowserThumbnail *pThumbnail = new CReplayBrowserThumbnail( this, "ListThumbnail", pItem->GetItemHandle(), m_pReplayItemManager );
	m_vecThumbnails.AddToTail( pThumbnail );
}
void CReplayBrowserThumbnailRow::AddReplayThumbnail( QueryableReplayItemHandle_t hReplayItem )
{
	CReplayBrowserThumbnail *pThumbnail = new CReplayBrowserThumbnail( this, "ListThumbnail", hReplayItem, m_pReplayItemManager );
	m_vecThumbnails.AddToTail( pThumbnail );
}

void CReplayBrowserThumbnailRow::DeleteReplayItemThumbnail( const IQueryableReplayItem *pReplayItem )
{
	CReplayBrowserThumbnail *pThumbnail = FindThumbnail( pReplayItem );
	int nThumbnailElement = m_vecThumbnails.Find( pThumbnail );
	if ( nThumbnailElement == m_vecThumbnails.InvalidIndex() )
	{
		AssertMsg( 0, "REPLAY: Should have found replay thumbnail while attempting to delete it from the browser." );
		return;
	}

	// Delete the actual panel
	ivgui()->RemoveTickSignal( pThumbnail->GetVPanel() );
	pThumbnail->MarkForDeletion();

	// Remove from list of thumbs
	m_vecThumbnails.Remove( nThumbnailElement );
}

int CReplayBrowserThumbnailRow::GetNumVisibleReplayItems() const
{
	int iCount = 0;
	FOR_EACH_VEC( m_vecThumbnails, i )
	{
		CReplayBrowserThumbnail *pThumbnail = m_vecThumbnails[ i ];
		if ( pThumbnail->IsVisible() )
		{
			iCount++;
		}
	}
	return iCount;
}

CReplayBrowserThumbnail *CReplayBrowserThumbnailRow::FindThumbnail( const IQueryableReplayItem *pReplayItem )
{
	AssertValidReadPtr( pReplayItem );
	FOR_EACH_VEC( m_vecThumbnails, i )
	{
		CReplayBrowserThumbnail *pThumbnail = m_vecThumbnails[ i ];
		if ( pThumbnail->GetReplayItem() == pReplayItem )
			return pThumbnail;
	}
	return NULL;
}

void CReplayBrowserThumbnailRow::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/thumbnailrow.res", "GAME" );
}

void CReplayBrowserThumbnailRow::PerformLayout()
{
	for ( int i = 0; i < m_vecThumbnails.Count(); ++i )
	{
		CReplayBrowserThumbnail *pThumbnail = m_vecThumbnails[ i ];
		pThumbnail->SetPos( i * ( pThumbnail->GetWide() + 2 * REPLAY_BORDER_WIDTH ), 0 );
		bool bShouldBeVisible = pThumbnail->m_hReplayItem != REPLAY_HANDLE_INVALID;
		if ( pThumbnail->IsVisible() != bShouldBeVisible )
		{
			pThumbnail->SetVisible( bShouldBeVisible );
		}
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------

CBaseThumbnailCollection::CBaseThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager )
:	EditablePanel( pParent, pName ),
	m_pReplayItemManager( pReplayItemManager ),
	m_nStartX( XRES(15) ),
	m_pCaratLabel( NULL ),
	m_pTitleLabel( NULL ),
	m_pNoReplayItemsLabel( NULL ),
	m_pRenderAllButton( NULL )
{
	m_pParentListPanel = static_cast< CReplayListPanel * >( pParent );
	m_pShowNextButton = NULL;
	m_pShowPrevButton = NULL;
	m_iViewingPage = 0;

	m_nReplayThumbnailsPerRow = 6;
	m_nMaxRows = 2;
}

void CBaseThumbnailCollection::AddReplay( const IQueryableReplayItem *pItem )
{
	m_vecReplays.AddToTail( pItem->GetItemHandle() );
}

void CBaseThumbnailCollection::CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem )
{
	IQueryableReplayItem *pReplayItem = m_pReplayItemManager->GetItem( hReplayItem );			Assert( pReplayItem );
	if ( !pReplayItem )
		return;

	// Find the replay thumbnail
	CReplayBrowserThumbnailRow *pThumbnailRow = FindReplayItemThumbnailRow( pReplayItem );
	if ( !pThumbnailRow )
	{
		AssertMsg( 0, "REPLAY: Should have found replay thumbnail row while attempting to delete it from the browser." );
		return;
	}

	// Remove it from the replay list and refresh the page
	m_vecReplays.FindAndRemove( hReplayItem );
	UpdateViewingPage();
	InvalidateLayout();
}

int CBaseThumbnailCollection::GetRowStartY()
{
	int nVerticalBuffer = YRES( 15 );
	Panel *pLowestPanel = m_pReplayItemManager->GetItemCount() == 0 ? m_pNoReplayItemsLabel : GetLowestPanel( nVerticalBuffer );

	int x,y;
	pLowestPanel->GetPos( x,y );

	bool bMakeRoomForNextPrev = (m_pShowPrevButton && m_pShowNextButton && (m_pShowPrevButton->IsVisible() || m_pShowNextButton->IsVisible()));
	if ( bMakeRoomForNextPrev )
	{
		nVerticalBuffer += m_pShowPrevButton->GetTall();
	}
	return y + pLowestPanel->GetTall() + nVerticalBuffer;
}

CReplayBrowserThumbnailRow *CBaseThumbnailCollection::FindReplayItemThumbnailRow( const IQueryableReplayItem *pReplayItem )
{
	AssertValidReadPtr( pReplayItem );

	FOR_EACH_VEC( m_vecRows, i )
	{
		CReplayBrowserThumbnailRow *pRow = m_vecRows[ i ];

		// If the replay thumbnail exists in the given row, return it
		if ( pRow->FindThumbnail( pReplayItem ) )
			return pRow;
	}

	return NULL;
}

void CBaseThumbnailCollection::RemoveEmptyRows()
{
	// Get a pointer to the row
	CUtlVector< CReplayBrowserThumbnailRow * > vecRowsToDelete;
	FOR_EACH_VEC( m_vecRows, i )
	{
		CReplayBrowserThumbnailRow *pRow = m_vecRows[ i ];
		if ( pRow->GetNumVisibleReplayItems() == 0 )
		{
			vecRowsToDelete.AddToTail( pRow );
		}
	}

	// Delete any rows
	FOR_EACH_VEC( vecRowsToDelete, i )
	{
		// Remove it
		int nElement = m_vecRows.Find( vecRowsToDelete[ i ] );
		m_vecRows[ nElement ]->MarkForDeletion();
		m_vecRows.Remove( nElement );
	}

	// If we deleted any rows...
	if ( vecRowsToDelete.Count() )
	{
		// Reposition and repaint
		ReplayUI_GetBrowserPanel()->InvalidateLayout();
		ReplayUI_GetBrowserPanel()->Repaint();

		// If we don't have any rows left...
		if ( !m_vecRows.Count() )
		{
			m_pParentListPanel->RemoveCollection( this );
		}
	}
}

void CBaseThumbnailCollection::RemoveAll()
{
	m_vecReplays.RemoveAll();
	RemoveEmptyRows();
	UpdateViewingPage();
	InvalidateLayout();
}


void CBaseThumbnailCollection::OnUpdated()
{
	m_iViewingPage = 0;
	UpdateViewingPage();
	InvalidateLayout( true, false );
}

void CBaseThumbnailCollection::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "render_queued_replays" ) )
	{
		::ReplayUI_ShowRenderDialog( this, REPLAY_HANDLE_INVALID, false, -1 );
		return;
	}
	else if ( FStrEq( pCommand, "show_next" ) )
	{
		int iThumbnailsPerPage = (m_nReplayThumbnailsPerRow * m_nMaxRows);
		m_iViewingPage = clamp( m_iViewingPage + 1, 0, Ceil2Int((float)m_vecReplays.Count() / (float)iThumbnailsPerPage) );
		UpdateViewingPage();
		return;
	}
	else if ( FStrEq( pCommand, "show_prev" ) )
	{
		int iThumbnailsPerPage = (m_nReplayThumbnailsPerRow * m_nMaxRows);
		m_iViewingPage = clamp( m_iViewingPage - 1, 0, Ceil2Int((float)m_vecReplays.Count() / (float)iThumbnailsPerPage) );
		UpdateViewingPage();
		return;
	}

	BaseClass::OnCommand( pCommand );
}

void CBaseThumbnailCollection::UpdateViewingPage( void )
{
	int iThumbnailsPerPage = (m_nReplayThumbnailsPerRow * m_nMaxRows);
	int iFirstReplayOnPage = (m_iViewingPage * iThumbnailsPerPage);

	// If the page we're on is not the first page, and we have no replays on it, move back a page.
	while (iFirstReplayOnPage >= m_vecReplays.Count() && m_iViewingPage > 0 )
	{
		m_iViewingPage--;
		iFirstReplayOnPage = (m_iViewingPage * iThumbnailsPerPage);
	}

	for ( int i = 0; i < iThumbnailsPerPage; i++ )
	{
		int iReplayIndex = (iFirstReplayOnPage + i);
		
		int iRow = floor( i / (float)m_nReplayThumbnailsPerRow );
		int iColumn = i % m_nReplayThumbnailsPerRow;

		// Hit the max number of rows we show?
		if ( iRow >= m_nMaxRows )
			break;

		// Need a new row?
		if ( iRow >= m_vecRows.Count() )
		{
			// If we don't actually have any more replays, we don't need to make the new row.
			if ( iReplayIndex >= m_vecReplays.Count() )
				break;

			// Create a new row and add there
			CReplayBrowserThumbnailRow *pNewRow = new CReplayBrowserThumbnailRow( this, "ThumbnailRow", m_pReplayItemManager );
			m_vecRows.AddToTail( pNewRow );
			InvalidateLayout();
		}

		// Need another thumbnail in this row?
		if ( iColumn >= m_vecRows[iRow]->GetNumReplayItems() )
		{
			// Hit the max number of thumbnails in this row?
			if ( iColumn >= m_nReplayThumbnailsPerRow )
				break;

			m_vecRows[iRow]->AddReplayThumbnail( REPLAY_HANDLE_INVALID );
		}
		
		if ( iReplayIndex >= m_vecReplays.Count() )
		{
			m_vecRows[iRow]->m_vecThumbnails[iColumn]->SetReplayItem( REPLAY_HANDLE_INVALID );
			m_vecRows[iRow]->m_vecThumbnails[iColumn]->SetVisible( false );
		}
		else
		{
			m_vecRows[iRow]->m_vecThumbnails[iColumn]->SetReplayItem( m_vecReplays[iReplayIndex] );
			m_vecRows[iRow]->m_vecThumbnails[iColumn]->SetVisible( true );
		}
	}

	// Update the button counts
	wchar_t wszTemp[256];
	wchar_t wszCount[10];
	int iNextReplays = clamp( m_vecReplays.Count() - iFirstReplayOnPage - iThumbnailsPerPage, 0, iThumbnailsPerPage );
	if ( iNextReplays > 0 )
	{
		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", iNextReplays );
		g_pVGuiLocalize->ConstructString_safe( wszTemp, g_pVGuiLocalize->Find("#Replay_NextX"), 1, wszCount );
		SetDialogVariable( "nextbuttontext", wszTemp );
		m_pShowNextButton->SetVisible( true );
	}
	else
	{
		m_pShowNextButton->SetVisible( false );
	}

	int iPrevReplays = clamp( iFirstReplayOnPage, 0, iThumbnailsPerPage );
	if ( iPrevReplays > 0 )
	{
		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", iPrevReplays );
		g_pVGuiLocalize->ConstructString_safe( wszTemp, g_pVGuiLocalize->Find("#Replay_PrevX"), 1, wszCount );
		SetDialogVariable( "prevbuttontext", wszTemp );
		m_pShowPrevButton->SetVisible( true );
	}
	else
	{
		m_pShowPrevButton->SetVisible( false );
	}

	RemoveEmptyRows();

	// We may have changed our size, so we need to tell our parent that it should re-layout
	m_pParentListPanel->InvalidateLayout();
}

void CBaseThumbnailCollection::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/thumbnailcollection.res", "GAME" );

	m_pCaratLabel = dynamic_cast< CExLabel * >( FindChildByName( "CaratLabel" ) );
	m_pTitleLabel = dynamic_cast< CExLabel * >( FindChildByName( "TitleLabel" ) );
	m_pNoReplayItemsLabel = dynamic_cast< CExLabel * >( FindChildByName( "NoReplayItemsLabel" ) );
	m_pShowNextButton = dynamic_cast< CExButton * >( FindChildByName( "ShowNextButton" ) );
	if ( m_pShowNextButton )
	{
		m_pShowNextButton->AddActionSignalTarget( this );
	}
	m_pShowPrevButton = dynamic_cast< CExButton * >( FindChildByName( "ShowPrevButton" ) );
	if ( m_pShowPrevButton )
	{
		m_pShowPrevButton->AddActionSignalTarget( this );
	}

	UpdateViewingPage();
}

void CBaseThumbnailCollection::PerformLayout()
{
	int nUnconvertedBgWidth = GetWide();

	// Layout no replay items label
	m_pNoReplayItemsLabel->SetPos( ( GetWide() - m_pNoReplayItemsLabel->GetWide() ) / 2, YRES( 40 ) );
	m_pNoReplayItemsLabel->SetVisible( !m_pReplayItemManager->GetItemCount() );
	m_pNoReplayItemsLabel->SetContentAlignment( Label::a_center );

	int nStartY = YRES(5);

	// Update the title count
	wchar_t wszCount[10];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", m_vecReplays.Count() );
	wchar_t wszTemp[256];
	const char *pszLocString = IsMovieCollection() ? "#Replay_SavedMovies" : "#Replay_UnrenderedReplays";
	g_pVGuiLocalize->ConstructString_safe( wszTemp, g_pVGuiLocalize->Find(pszLocString), 1, wszCount );
	SetDialogVariable( "titleandcount", wszTemp );

	// Setup labels for unconverted replay display
	LayoutUpperPanels( nStartY, nUnconvertedBgWidth );

	int nCurrentY = GetRowStartY();

	// Position our prev button
	int nButtonX = (GetWide() - m_pShowPrevButton->GetWide()) * 0.5;
	if ( m_pShowPrevButton )
	{
		m_pShowPrevButton->SetPos( nButtonX, nCurrentY - m_pShowPrevButton->GetTall() - YRES(2) );
		nCurrentY += YRES(2);
	}

	// Setup converted row positions
	for ( int i = 0; i < m_vecRows.Count(); ++i )
	{
		CReplayBrowserThumbnailRow *pRow = m_vecRows[ i ];
		pRow->SetPos( m_nStartX, nCurrentY );
		pRow->InvalidateLayout( true, true );
		int nRowTall = pRow->GetTall();
		nCurrentY += nRowTall;
	}

	int nTotalHeight = nCurrentY;

	// Position our next button
	if ( m_pShowNextButton )
	{
		m_pShowNextButton->SetPos( nButtonX, nCurrentY );

		bool bMakeRoomForNextPrev = (m_pShowPrevButton && m_pShowNextButton && (m_pShowPrevButton->IsVisible() || m_pShowNextButton->IsVisible()));
		if ( bMakeRoomForNextPrev )
		{
			nTotalHeight += m_pShowNextButton->GetTall() + YRES(5);
		}
	}

	LayoutBackgroundPanel( nUnconvertedBgWidth, nTotalHeight );

	// TODO: Resizing can cause an InvalidateLayout() call if the new & old dimensions differ,
	// perhaps calling this here is not the best idea.
	// Adjust total height of panel
	SetTall( nTotalHeight );

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------

CReplayThumbnailCollection::CReplayThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager )
:	BaseClass( pParent, pName, pReplayItemManager ),
	m_pWarningLabel( NULL ),
	m_pUnconvertedBg( NULL )
{
	// Create additional panels for unconverted rows
	m_pLinePanel = new Panel( this, "Line" );
	m_pWarningLabel = new CExLabel( this, "WarningLabel", "#Replay_ConversionWarning" );
	m_pUnconvertedBg = new Panel( this, "UnconvertedBg" );
	m_pRenderAllButton = new CExButton( this, "RenderAllButton", "#Replay_RenderAll" );
}

bool CReplayThumbnailCollection::IsMovieCollection() const
{
	return false;
}

void CReplayThumbnailCollection::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CReplayThumbnailCollection::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

#if defined( TF_CLIENT_DLL )
	if ( m_pUnconvertedBg )
	{
		vgui::IBorder *pBorder = pScheme->GetBorder( "ButtonBorder" );
		m_pUnconvertedBg->SetBorder( pBorder );
		SetPaintBorderEnabled( true );
	}
#else
	SetPaintBorderEnabled( false );
#endif

	// Get current key binding for "save_replay", if any.
	const char *pBoundKey = engine->Key_LookupBinding( "save_replay" );
	if ( !pBoundKey || FStrEq( pBoundKey, "(null)" ) )
	{
		m_pNoReplayItemsLabel->SetText( "#Replay_NoKeyBoundNoReplays" );
	}
	else
	{
		char szKey[16];
		Q_snprintf( szKey, sizeof(szKey), "%s", pBoundKey );

		wchar_t wKey[16];
		wchar_t wLabel[256];

		g_pVGuiLocalize->ConvertANSIToUnicode( szKey, wKey, sizeof( wKey ) );
		g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find("#Replay_NoReplays" ), 1, wKey );

		m_pNoReplayItemsLabel->SetText( wLabel );
	}
}

void CReplayThumbnailCollection::LayoutUpperPanels( int nStartY, int nBgWidth )
{
	int nUnconvertedY = nStartY + 2 * REPLAY_BUFFER_HEIGHT;
	if ( !m_pTitleLabel )
		return;

	m_pTitleLabel->SizeToContents();
	m_pTitleLabel->SetBounds( m_nStartX, nUnconvertedY, GetWide(), m_pTitleLabel->GetTall() );
	m_pTitleLabel->SetVisible( true );

	int cx, cy;
	int nWarningStartY = nUnconvertedY + m_pTitleLabel->GetTall() + REPLAY_BUFFER_HEIGHT;
	m_pWarningLabel->GetContentSize( cx, cy );
	m_pWarningLabel->SetBounds( m_nStartX, nWarningStartY, 2 * GetWide() / 3, cy );	// For when "convert all" button is shown
	m_pWarningLabel->SetVisible( m_pReplayItemManager->GetItemCount() > 0 );

	// Setup carat label
	if ( m_pCaratLabel )
	{
		m_pCaratLabel->SizeToContents();
		m_pCaratLabel->SetPos( m_nStartX - m_pCaratLabel->GetWide() - XRES(3), nUnconvertedY );
		m_pCaratLabel->SetVisible( true );
	}

	// Setup line
	int nLineBuffer = m_pReplayItemManager->GetItemCount() > 0 ? YRES( 15 ) : nStartY;
	int nLineY = nWarningStartY + nLineBuffer;
	m_pLinePanel->SetBounds( 0, nLineY, nBgWidth, 1 );
	m_pLinePanel->SetVisible( true );

	int nButtonX = (GetWide() - m_pShowPrevButton->GetWide()) * 0.5;
	if ( m_pShowPrevButton )
	{
		m_pShowPrevButton->SetPos( nButtonX, nLineY );
	}
}

void CReplayThumbnailCollection::LayoutBackgroundPanel( int nWide, int nTall )
{
	// Setup bounds for dark background, if there are unconverted replays in this collection
	if ( m_pUnconvertedBg )
	{
		m_pUnconvertedBg->SetBounds(
			0,
			0,
			nWide,
			nTall
		);
		m_pUnconvertedBg->SetVisible( true );

		// Setup convert all button
		int nWarningLabelX, nWarningLabelY;
		m_pWarningLabel->GetPos( nWarningLabelX, nWarningLabelY );
		int nRenderAllX = m_pUnconvertedBg->GetWide() - m_pRenderAllButton->GetWide() - XRES( 5 );
		m_pRenderAllButton->SetPos( nRenderAllX, nWarningLabelY - m_pRenderAllButton->GetTall()/2 );
		m_pRenderAllButton->SetVisible( m_pReplayItemManager->GetItemCount() > 0 );
	}
}

Panel *CReplayThumbnailCollection::GetLowestPanel( int &nVerticalBuffer )
{
	nVerticalBuffer = YRES( 8 );
	return m_pLinePanel;
}

//-----------------------------------------------------------------------------

CMovieThumbnailCollection::CMovieThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager,
																  int nDay, int nMonth, int nYear, bool bShowSavedMoviesLabel )
:	BaseClass( pParent, pName, pReplayItemManager )
{
	Init( nDay, nMonth, nYear, bShowSavedMoviesLabel );
}

CMovieThumbnailCollection::CMovieThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager, bool bShowSavedMoviesLabel )
:	BaseClass( pParent, pName, pReplayItemManager )
{
	Init( -1, -1, -1, bShowSavedMoviesLabel );
}

void CMovieThumbnailCollection::Init( int nDay, int nMonth, int nYear, bool bShowSavedMoviesLabel )
{
	m_nDay = nDay;
	m_nMonth = nMonth;
	m_nYear = nYear;

	if ( m_nDay == OLDER_MOVIES_COLLECTION )
	{
		m_pDateLabel = new CExLabel( this, "DateLabel", "#Replay_OlderMovies" );
	}
	else
	{
		m_pDateLabel = m_nDay >= 0 ? new CExLabel( this, "DateLabel", CReplayTime::GetLocalizedDate( g_pVGuiLocalize, nDay, nMonth, nYear ) ) : NULL;
	}
	m_bShowSavedMoviesLabel = bShowSavedMoviesLabel;
}

bool CMovieThumbnailCollection::IsMovieCollection() const
{
	return true;
}

void CMovieThumbnailCollection::PerformLayout()
{
	BaseClass::PerformLayout();

	// Movies have multiple collections under a single header. So we use the total movies, not the amount in this collection.
	if ( g_pReplayMovieManager )
	{
		wchar_t wszCount[10];
		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", g_pReplayMovieManager->GetMovieCount() );
		wchar_t wszTemp[256];
		g_pVGuiLocalize->ConstructString_safe( wszTemp, g_pVGuiLocalize->Find("#Replay_SavedMovies"), 1, wszCount );
		SetDialogVariable( "titleandcount", wszTemp );
	}

	if ( m_pDateLabel )
	{
		m_pDateLabel->SetVisible( m_pReplayItemManager->GetItemCount() );
	}
}

void CMovieThumbnailCollection::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pDateLabel )
	{
		m_pDateLabel->SetVisible( true );
	}

	if ( m_pCaratLabel )
	{
		m_pCaratLabel->SetVisible( m_bShowSavedMoviesLabel );
	}
	if ( m_pTitleLabel )
	{
		m_pTitleLabel->SetVisible( m_bShowSavedMoviesLabel );
	}

	m_pNoReplayItemsLabel->SetText( "#Replay_NoMovies" );
}

void CMovieThumbnailCollection::LayoutUpperPanels( int nStartY, int nBgWidth )
{
	if ( m_pTitleLabel && m_pTitleLabel->IsVisible() )
	{
		m_pTitleLabel->SizeToContents();
		m_pTitleLabel->SetPos( m_nStartX, nStartY );

		m_pCaratLabel->SizeToContents();
		m_pCaratLabel->SetPos( m_nStartX - m_pCaratLabel->GetWide() - XRES(3), nStartY + ( m_pCaratLabel->GetTall() - m_pCaratLabel->GetTall() ) / 2 );

		nStartY += m_pTitleLabel->GetTall() + YRES( 5 );
	}

	// Date label
	if ( m_pDateLabel && m_pDateLabel->IsVisible() )
	{
		m_pDateLabel->SizeToContents();
		m_pDateLabel->SetPos( m_nStartX, nStartY );
	}
}

Panel *CMovieThumbnailCollection::GetLowestPanel( int &nVerticalBuffer )
{
	nVerticalBuffer = YRES( 8 );
	return m_pDateLabel ? m_pDateLabel : m_pTitleLabel;
}

bool CMovieThumbnailCollection::DoesDateMatch( int nDay, int nMonth, int nYear )
{
	return ( nDay == m_nDay ) && ( nMonth == m_nMonth ) && ( nYear == m_nYear );
}

#endif
