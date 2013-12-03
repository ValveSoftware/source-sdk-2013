//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowserlistpanel.h"
#include "ienginevgui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/IVGui.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "replaybrowserlistitempanel.h"
#include "replaybrowserpreviewpanel.h"
#include "replaybrowserbasepage.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplaymanager.h"
#include "replaybrowsermainpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IClientReplayContext *g_pClientReplayContext;
extern IReplayMovieManager *g_pReplayMovieManager;
extern const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------

DECLARE_BUILD_FACTORY( CReplayListPanel );

//-----------------------------------------------------------------------------

#define MAX_MOVIE_THUMBNAILS	12		// The remaining movies will be put into a single collection

//-----------------------------------------------------------------------------

CReplayListPanel::CReplayListPanel( Panel *pParent, const char *pName )
:	BaseClass( pParent, pName ),
	m_pPrevHoverPanel( NULL ),
	m_pPreviewPanel( NULL )
{
	ivgui()->AddTickSignal( GetVPanel(), 10 );

	m_pBorderArrowImg = new ImagePanel( this, "ArrowImage" );
	
	// Add replays and movies collections, which will contain all replays & movies.
	m_pReplaysCollection = new CReplayThumbnailCollection( this, "ReplayThumbnailCollection", GetReplayItemManager() );
	m_pMoviesCollection = new CMovieThumbnailCollection( this, "MovieThumbnailCollection", GetReplayMovieItemManager(), true );

	m_vecCollections.AddToTail( m_pReplaysCollection );
	m_vecCollections.AddToTail( m_pMoviesCollection );

	m_wszFilter[0] = L' ';
	m_wszFilter[1] = NULL;
}

CReplayListPanel::~CReplayListPanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CReplayListPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/replaylistpanel.res", "GAME" );

#if !defined( TF_CLIENT_DLL )
	SetPaintBorderEnabled( false );
#endif

	MoveScrollBarToTop();

	vgui::ScrollBar *pScrollBar = dynamic_cast< vgui::ScrollBar * >( FindChildByName( "PanelListPanelVScroll" ) );
	pScrollBar->SetScrollbarButtonsVisible( false );
	Color clrButtonColor = GetSchemeColor( "Yellow", Color( 255, 255, 255, 255 ), pScheme );
	Color clrBgColor = GetSchemeColor( "TanDark", Color( 255, 255, 255, 255 ), pScheme );
	const int nWidth = XRES( 5 );
	pScrollBar->SetSize( nWidth, GetTall() );
	pScrollBar->GetSlider()->SetSize( nWidth, GetTall() );
}

void CReplayListPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CReplayListPanel::OnMouseWheeled(int delta)
{
	if ( !GetScrollbar()->IsVisible() )
		return;

	BaseClass::OnMouseWheeled( delta );
}

void CReplayListPanel::SetupBorderArrow( bool bLeft )
{
	m_pBorderArrowImg->SetVisible( true );

	m_pBorderArrowImg->SetImage( bLeft ? "replay/replay_balloon_arrow_left" : "replay/replay_balloon_arrow_right" );
	m_pBorderArrowImg->SetZPos( 1000 );

	m_pBorderArrowImg->GetImage()->GetContentSize( m_aBorderArrowDims[0], m_aBorderArrowDims[1] );
	m_pBorderArrowImg->SetSize( m_aBorderArrowDims[0], m_aBorderArrowDims[1] );
}

void CReplayListPanel::ClearPreviewPanel()
{
	if ( m_pPreviewPanel )
	{
		m_pPreviewPanel->MarkForDeletion();
		m_pPreviewPanel = NULL;
	}
}

void CReplayListPanel::ApplyFilter( const wchar_t *pFilterText )
{
	Q_wcsncpy( m_wszFilter, pFilterText, sizeof( m_wszFilter ) );
	V_wcslower( m_wszFilter );

	m_pReplaysCollection->RemoveAll();
	m_pMoviesCollection->RemoveAll();
	m_pPrevHoverPanel = NULL;
	ClearPreviewPanel();

	RemoveAll();
	AddReplaysToList();

	FOR_EACH_VEC( m_vecCollections, i )
	{
		m_vecCollections[i]->OnUpdated();
	}

	InvalidateLayout();
}

void CReplayListPanel::OnTick()
{
	if ( !enginevgui->IsGameUIVisible() )
		return;

	CReplayBrowserPanel *pReplayBrowser = ReplayUI_GetBrowserPanel();
	if ( !pReplayBrowser || !pReplayBrowser->IsVisible() )
		return;

	int x,y;
	vgui::input()->GetCursorPos(x, y);

	// If the deletion confirmation dialog is up
	if ( vgui::input()->GetAppModalSurface() )
	{
		ClearPreviewPanel();

		// Hide the preview arrow
		m_pBorderArrowImg->SetVisible( false );

		return;
	}

	CReplayBrowserThumbnail *pOverPanel = FindThumbnailAtCursor( x, y );
	if ( m_pPrevHoverPanel != pOverPanel )
	{
		if ( m_pPrevHoverPanel )
		{
			OnItemPanelExited( m_pPrevHoverPanel );
		}

		m_pPrevHoverPanel = pOverPanel;

		if ( m_pPrevHoverPanel )
		{
			OnItemPanelEntered( m_pPrevHoverPanel );
		}
	}
}

void CReplayListPanel::OnItemPanelEntered( vgui::Panel *pPanel )
{
	CReplayBrowserThumbnail *pThumbnail = dynamic_cast< CReplayBrowserThumbnail * >( pPanel );

	if ( IsVisible() && pThumbnail && pThumbnail->IsVisible() )
	{
		ClearPreviewPanel();

		// Determine which type of preview panel to display
		IReplayItemManager *pItemManager;
		IQueryableReplayItem *pReplayItem = FindReplayItem( pThumbnail->m_hReplayItem, &pItemManager );

		AssertMsg( pReplayItem, "Why is this happening?" );
		if ( !pReplayItem )
			return;

		if ( pReplayItem->IsItemAMovie() )
		{
			m_pPreviewPanel = new CReplayPreviewPanelBase( this, pReplayItem->GetItemHandle(), pItemManager );
		}
		else
		{
			m_pPreviewPanel = new CReplayPreviewPanelSlideshow( this, pReplayItem->GetItemHandle(), pItemManager );
		}

		m_pPreviewPanel->InvalidateLayout( true, true );

		int x,y;
		pThumbnail->GetPosRelativeToAncestor( this, x, y );

		int nXPos, nYPos;
		int nOffset = XRES( 1 );
		nXPos = ( x > GetWide()/2 ) ? ( x - m_pPreviewPanel->GetWide() - nOffset ) : ( x + pThumbnail->GetWide() + nOffset );
		nYPos = y + ( pThumbnail->GetTall() - m_pPreviewPanel->GetTall() ) / 2;

		// Make sure the popup stays onscreen.
		if ( nXPos < 0 )
		{
			nXPos = 0;
		}
		else if ( (nXPos + m_pPreviewPanel->GetWide()) > GetWide() )
		{
			nXPos = GetWide() - m_pPreviewPanel->GetWide();
		}

		if ( nYPos < 0 )
		{
			nYPos = 0;
		}
		else if ( (nYPos + m_pPreviewPanel->GetTall()) > GetTall() )
		{
			// Move it up as much as we can without it going below the bottom
			nYPos = GetTall() - m_pPreviewPanel->GetTall();
		}

		// Setup the balloon's arrow
		bool bLeftArrow = x < (GetWide() / 2);
		SetupBorderArrow( bLeftArrow );		// Sets proper image and caches image dims in m_aBorderArrowDims
		int nArrowXPos, nArrowYPos;
		const int nPreviewBorderWidth = 2;	// Should be just big enough to cover the preview's border width
		if ( bLeftArrow )
		{
			// Setup the arrow along the left-hand side
			nArrowXPos = nXPos - m_aBorderArrowDims[0] + nPreviewBorderWidth;
		}
		else
		{
			nArrowXPos = nXPos + m_pPreviewPanel->GetWide() - nPreviewBorderWidth;
		}
		nArrowYPos = MIN( nYPos + m_pPreviewPanel->GetTall() - m_pBorderArrowImg->GetTall() * 2, y + ( pThumbnail->m_pScreenshotThumb->GetTall() - m_aBorderArrowDims[1] ) / 2 );
		m_pBorderArrowImg->SetPos( nArrowXPos, nArrowYPos );

		m_pPreviewPanel->SetPos( nXPos, nYPos );
		m_pPreviewPanel->SetVisible( true );

		surface()->PlaySound( "replay\\replaypreviewpopup.wav" );
	}
}

void CReplayListPanel::OnItemPanelExited( vgui::Panel *pPanel )
{
	CReplayBrowserThumbnail *pThumbnail = dynamic_cast < CReplayBrowserThumbnail * > ( pPanel );

	if ( pThumbnail && IsVisible() && m_pPreviewPanel )
	{
		m_pBorderArrowImg->SetVisible( false );
		ClearPreviewPanel();
	}
}

CBaseThumbnailCollection *CReplayListPanel::FindOrAddReplayThumbnailCollection( const IQueryableReplayItem *pItem, IReplayItemManager *pItemManager )
{
	Assert( pItem );

	if ( pItem->IsItemAMovie() )
	{
		return m_pMoviesCollection;
	}
	
	return m_pReplaysCollection;
}

void CReplayListPanel::AddReplaysToList()
{
	// Cache off list item pointers into a temp list for processing
	CUtlLinkedList< IQueryableReplayItem *, int > lstMovies;
	CUtlLinkedList< IQueryableReplayItem *, int > lstReplays;

	// Add all replays to a replays list
	g_pReplayManager->GetReplaysAsQueryableItems( lstReplays );

	// Add all movies to a movies list
	g_pReplayMovieManager->GetMoviesAsQueryableItems( lstMovies );

	// Go through all movies, and add them to the proper collection, based on date
	FOR_EACH_LL( lstMovies, i )
	{
		if ( PassesFilter( lstMovies[ i ] ) )
		{
			AddReplayItem( lstMovies[ i ]->GetItemHandle() );
		}
	}

	// Add any replays to the "temporary replays" collection
	FOR_EACH_LL( lstReplays, i )
	{
		if ( PassesFilter( lstReplays[ i ] ) )
		{
			m_pReplaysCollection->AddReplay( lstReplays[ i ] );
		}
	}

	// Add all collection panels to the list panel
	FOR_EACH_VEC( m_vecCollections, i )
	{
		AddItem( NULL, m_vecCollections[ i ] );
	}
}

void CReplayListPanel::RemoveCollection( CBaseThumbnailCollection *pCollection )
{
	// Never remove our two base collections. If they have no entries, they display messages instead.
	if ( pCollection == m_pMoviesCollection || pCollection == m_pReplaysCollection )
		return;

	// Find the item and remove it
	int i = FirstItem();
	while ( i != InvalidItemID() )
	{
		if ( GetItemPanel( i ) == pCollection )
		{
			int nNextI = NextItem( i );
			RemoveItem( i );
			i = nNextI;
		}
		else
		{
			i = NextItem( i );
		}
	}

	// Remove our own cached ptr
	i = m_vecCollections.Find( pCollection );
	if ( i != m_vecCollections.InvalidIndex() )
	{
		m_vecCollections.Remove( i );
	}
}

CReplayBrowserThumbnail *CReplayListPanel::FindThumbnailAtCursor( int x, int y )
{
	// Is the cursor hovering over any of the thumbnails?
	FOR_EACH_VEC( m_vecCollections, i )
	{
		CBaseThumbnailCollection *pCollection = m_vecCollections[ i ];
		if ( pCollection->IsWithin( x, y ) )
		{
			FOR_EACH_VEC( pCollection->m_vecRows, j )
			{
				CReplayBrowserThumbnailRow *pRow = pCollection->m_vecRows[ j ];
				if ( pRow->IsWithin( x, y ) )
				{
					FOR_EACH_VEC( pRow->m_vecThumbnails, k )
					{
						CReplayBrowserThumbnail *pThumbnail = pRow->m_vecThumbnails[ k ];
						if ( pThumbnail->IsWithin( x, y ) )
						{
							return pThumbnail;
						}
					}
				}
			}
		}
	}

	return NULL;
}

#if defined( WIN32 )
	#define Q_wcstok( text, delimiters, context ) wcstok( text, delimiters ); context;
#elif defined( OSX ) || defined( LINUX )
	#define Q_wcstok( text, delimiters, context ) wcstok( text, delimiters, context )
#endif

bool CReplayListPanel::PassesFilter( IQueryableReplayItem *pItem )
{
	CGenericClassBasedReplay *pReplay = ToGenericClassBasedReplay( pItem->GetItemReplay() );
	if ( !pReplay )
		return false;

	wchar_t wszSearchableText[1024] = L"";
	wchar_t wszTemp[256];
	// title
	const wchar_t *pTitle = pItem->GetItemTitle();
	V_wcscat_safe( wszSearchableText, pTitle );
	V_wcscat_safe( wszSearchableText, L" " );
	// map
	const char *pMapName = GetMapDisplayName( pReplay->m_szMapName );
	g_pVGuiLocalize->ConvertANSIToUnicode( pMapName, wszTemp, sizeof( wszTemp ) );
	V_wcscat_safe( wszSearchableText, wszTemp );
	V_wcscat_safe( wszSearchableText, L" " );
	// player class
	g_pVGuiLocalize->ConvertANSIToUnicode( pReplay->GetPlayerClass(), wszTemp, sizeof( wszTemp ) );
	V_wcscat_safe( wszSearchableText, wszTemp );
	V_wcscat_safe( wszSearchableText, L" " );
	// killer name
	if ( pReplay->WasKilled() )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pReplay->GetKillerName(), wszTemp, sizeof( wszTemp ) );
		V_wcscat_safe( wszSearchableText, wszTemp );
		V_wcscat_safe( wszSearchableText, L" " );
	}
	// lower case
	V_wcslower( wszSearchableText );

	wchar_t wszFilter[256];
	Q_wcsncpy( wszFilter, m_wszFilter, sizeof( wszFilter ) );
	
	bool bPasses = true;
	wchar_t seps[] = L" ";
	wchar_t *last = NULL;
	wchar_t *token = Q_wcstok( wszFilter, seps, &last );
	while ( token && bPasses )
	{
		bPasses &= wcsstr( wszSearchableText, token ) != NULL;
		token = Q_wcstok( NULL, seps, &last );
	}

	return bPasses;
}

void CReplayListPanel::AddReplayItem( ReplayItemHandle_t hItem )
{
	IReplayItemManager *pItemManager;
	const IQueryableReplayItem *pItem = FindReplayItem( hItem, &pItemManager );
	if ( !pItem )
		return;

	// Find or add the collection
	CBaseThumbnailCollection *pCollection = FindOrAddReplayThumbnailCollection( pItem, pItemManager );

	// Add the replay
	pCollection->AddReplay( pItem );
}

void CReplayListPanel::CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem )
{
	IReplayItemManager *pItemManager;
	const IQueryableReplayItem *pReplayItem = FindReplayItem( hReplayItem, &pItemManager );		AssertValidReadPtr( pReplayItem );

	CBaseThumbnailCollection *pCollection = NULL;

	FOR_EACH_VEC( m_vecCollections, i )
	{
		CBaseThumbnailCollection *pCurCollection = m_vecCollections[ i ];
		if ( pCurCollection->FindReplayItemThumbnailRow( pReplayItem ) )
		{
			pCollection = pCurCollection;
			break;
		}
	}

	// Find the collection associated with the given replay - NOTE: we pass false here for the "bAddIfNotFound" param
	if ( !pCollection )
	{
		AssertMsg( 0, "REPLAY: Should have found collection while attempting to delete a replay from the browser." );
		return;
	}

	// Clear the previous hover pointer to avoid a potential crash where we've got a stale ptr
	m_pPrevHoverPanel = NULL;

	// Clear out the preview panel if it exists and is for the given replay necessary
	if ( m_pPreviewPanel && m_pPreviewPanel->GetReplayHandle() == pReplayItem->GetItemReplayHandle() )
	{
		ClearPreviewPanel();
		m_pBorderArrowImg->SetVisible( false );
	}

	pCollection->CleanupUIForReplayItem( hReplayItem );
}

#endif