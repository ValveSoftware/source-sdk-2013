//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"


#if defined( REPLAY_ENABLED )

#include "replaybrowserpreviewpanel.h"
#include "replaybrowsermainpanel.h"
#include "replaybrowsermovieplayerpanel.h"
#include "replaybrowserlistitempanel.h"
#include "replay/ireplaymovie.h"
#include "replay/screenshot.h"
#include "vgui/ISurface.h"
#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

CReplayPreviewPanelBase::CReplayPreviewPanelBase( Panel *pParent, QueryableReplayItemHandle_t hItem, IReplayItemManager *pItemManager )
:	EditablePanel( pParent, "PreviewPanel" ),
	m_hItem( hItem ),
	m_pItemManager( pItemManager )
{
	CGenericClassBasedReplay *pReplay = GetReplay();
	IQueryableReplayItem *pItem = pItemManager->GetItem( hItem );

	// Setup class image
	char szImage[MAX_OSPATH];
	m_pClassImage = new ImagePanel( this, "ClassImage" );
	V_snprintf( szImage, sizeof( szImage ), "class_sel_sm_%s_%s", pReplay->GetMaterialFriendlyPlayerClass(), pReplay->GetPlayerTeam() );	// Cause default image to display
	m_pClassImage->SetImage( szImage );

	m_pInfoPanel = new vgui::EditablePanel( this, "InfoPanel" );

	// Setup map label
	const char *pMapName = pReplay->m_szMapName;
	const char *pUnderscore = V_strstr( pMapName, "_" );
	if ( pUnderscore )
	{
		pMapName = pUnderscore + 1;
	}
	m_pMapLabel = new CExLabel( m_pInfoPanel, "MapLabel", pMapName );

	// Setup record date/time
	const CReplayTime &RecordTime = pItem->GetItemDate();
	int nDay, nMonth, nYear;
	RecordTime.GetDate( nDay, nMonth, nYear );
	int nHour, nMin, nSec;
	RecordTime.GetTime( nHour, nMin, nSec );
	const wchar_t *pDateAndTime = CReplayTime::GetLocalizedDate( g_pVGuiLocalize, nDay, nMonth, nYear, &nHour, &nMin, &nSec );

	// Setup date / time label
	m_pDateTimeLabel = new CExLabel( m_pInfoPanel, "DateTimeLabel", pDateAndTime );

	// Setup info labels
	for ( int i = 0; i < NUM_INFO_LABELS; ++i )
	{
		for ( int j = 0; j < 2; ++j )
		{
			m_pReplayInfoLabels[i][j] = new CExLabel( m_pInfoPanel, VarArgs("Label%d_%d", i, j), "" );
		}
	}
	m_pReplayInfoLabels[ LABEL_PLAYED_AS   ][1]->SetText( pReplay->GetPlayerClass() );
	m_pReplayInfoLabels[ LABEL_KILLED_BY   ][1]->SetText( pReplay->WasKilled() ? pReplay->GetKillerName() : "#Replay_NoKiller" );
	m_pReplayInfoLabels[ LABEL_LIFE_LENGTH ][1]->SetText( CReplayTime::FormatTimeString( (int)pItem->GetItemLength() ) );
}

CReplayPreviewPanelBase::~CReplayPreviewPanelBase()
{
}

void CReplayPreviewPanelBase::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/previewpanel.res", "GAME" );

#if !defined( TF_CLIENT_DLL )
	m_pClassImage->SetVisible( false );
#endif
}

void CReplayPreviewPanelBase::PerformLayout()
{
	BaseClass::PerformLayout();

	CGenericClassBasedReplay *pReplay = GetReplay();
	if ( !pReplay )
		return;

	int nWide = XRES(18);
	int nTall = YRES(18);
	int nScreenshotH = 0;	// Represents the height of the screenshot OR the "no screenshot" label
	LayoutView( nWide, nTall, nScreenshotH );
	int iInfoHeight = m_pInfoPanel->GetTall();
	nTall += iInfoHeight;

	if ( m_pClassImage )
	{
		int w, h;
		m_pClassImage->GetImage()->GetContentSize( w, h );
		float s = ShoudlUseLargeClassImage() ? 1.25f : 1.0f;
		int nClassTall = s*h;
		m_pClassImage->SetSize( s*w, nClassTall );
		m_pClassImage->SetShouldScaleImage( true );
		m_pClassImage->SetScaleAmount( s );

		// The panel should be at least as tall as the height of the screenshot
		// (or "no screenshot" label) and the class image.
		if ( nTall < nClassTall )
		{
			nTall = nClassTall;
		}
		m_pClassImage->SetPos( XRES(9), nTall - nClassTall );
	}

	const int nLabelX = m_pClassImage->GetWide() + XRES( 18 );
	int iInfoWidth = nWide - nLabelX;
	m_pMapLabel->SetSize( iInfoWidth, m_pMapLabel->GetTall() );
	m_pDateTimeLabel->SetSize( iInfoWidth, m_pMapLabel->GetTall() );
	m_pInfoPanel->SetBounds( nLabelX, nTall - iInfoHeight, iInfoWidth, iInfoHeight );

	nTall += YRES(9);

	SetSize( nWide, nTall );
}

void CReplayPreviewPanelBase::LayoutView( int &nWide, int &nTall, int &nCurY )
{
	nWide = XRES( 188 );
	nTall = YRES(9);
	nCurY = nTall;
}

CGenericClassBasedReplay *CReplayPreviewPanelBase::GetReplay()
{
	return ToGenericClassBasedReplay( m_pItemManager->GetItem( m_hItem )->GetItemReplay() );
}

ReplayHandle_t CReplayPreviewPanelBase::GetReplayHandle()
{
	return GetReplay()->GetHandle();
}

//-----------------------------------------------------------------------------

CReplayPreviewPanelSlideshow::CReplayPreviewPanelSlideshow( Panel *pParent, QueryableReplayItemHandle_t hReplay, IReplayItemManager *pItemManager )
:	BaseClass( pParent, hReplay, pItemManager ),
	m_pScreenshotPanel( NULL )
{
	// Setup screenshot slideshow panel
	CGenericClassBasedReplay *pReplay = GetReplay();
	const int nScreenshotCount = pReplay->GetScreenshotCount();
	if ( nScreenshotCount )
	{
		m_pScreenshotPanel = new CReplayScreenshotSlideshowPanel( this, "ScreenshotSlideshowPanel", hReplay );
		
		// Set pretty quick transition times based on the screenshot count
		m_pScreenshotPanel->SetInterval( ( nScreenshotCount == 2 ) ? 3.0f : 2.0f );
		m_pScreenshotPanel->SetTransitionTime( 0.5f );
	}

	// Setup the no screenshot label
	m_pNoScreenshotLabel = new CExLabel( this, "NoScreenshotLabel", "#Replay_NoScreenshot" );
	m_pNoScreenshotLabel->SetVisible( false );
}

void CReplayPreviewPanelSlideshow::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pNoScreenshotLabel->SizeToContents();
	m_pNoScreenshotLabel->SetWide( GetWide() );
}

void CReplayPreviewPanelSlideshow::LayoutView( int &nWide, int &nTall, int &nCurY )
{
	if ( m_pScreenshotPanel )
	{
		// Use the dimensions from the first screenshot to figure out the scale, even though the dimensions
		// may vary if the user changed resolutions during gameplay
		CGenericClassBasedReplay *pReplay = GetReplay();
		const CReplayScreenshot *pScreenshot = pReplay->GetScreenshot( 0 );
		int nScreenshotW = pScreenshot->m_nWidth;
		int nScreenshotH = pScreenshot->m_nHeight;

		// Scale the screenshot if it's too big for the current resolution
		float flScreenshotScale = 1.0f;
		int nMaxScreenshotWidth = ScreenWidth() / 3;
		if ( nScreenshotW > nMaxScreenshotWidth )
		{
			flScreenshotScale = (float)nMaxScreenshotWidth / pScreenshot->m_nWidth;
			nScreenshotW = nMaxScreenshotWidth;
		}
		nCurY = nScreenshotH * flScreenshotScale;

		m_pScreenshotPanel->GetImagePanel()->SetShouldScaleImage( true );
		m_pScreenshotPanel->GetImagePanel()->SetScaleAmount( flScreenshotScale );

		nWide += nScreenshotW;
		nTall += nCurY;

		m_pScreenshotPanel->SetBounds( (nWide - nScreenshotW) * 0.5, YRES(9), nScreenshotW, nCurY ); 
	}
	else
	{
		int w, h;
		m_pNoScreenshotLabel->SetContentAlignment( Label::a_center );
		m_pNoScreenshotLabel->GetContentSize( w, h );
		nTall += YRES( 20 );
		m_pNoScreenshotLabel->SetBounds( 0, nTall, w, h );
		nTall += YRES( 20 );
		m_pNoScreenshotLabel->SetVisible( true );

		nWide += XRES( 213 );	// Default width (maps to 640 on 1920x1200)
		nTall += h;
		nCurY = nTall;
	}
}

//-----------------------------------------------------------------------------

CReplayPreviewPanelMovie::CReplayPreviewPanelMovie( Panel *pParent, QueryableReplayItemHandle_t hItem, IReplayItemManager *pItemManager )
:	BaseClass( pParent, hItem, pItemManager ),
	m_pMoviePlayerPanel( NULL )
{
	m_flCreateTime = gpGlobals->realtime;

	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CReplayPreviewPanelMovie::~CReplayPreviewPanelMovie()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CReplayPreviewPanelMovie::OnTick()
{
	if ( gpGlobals->realtime >= m_flCreateTime + 0.5f )
	{
		if ( !m_pMoviePlayerPanel )
		{
			m_pMoviePlayerPanel = new CMoviePlayerPanel( this, "MoviePlayer", GetReplayMovie()->GetMovieFilename() );
			InvalidateLayout( true, false );
		}

		if ( !m_pMoviePlayerPanel->IsPlaying() )
		{
			m_pMoviePlayerPanel->SetLooping( true );
			m_pMoviePlayerPanel->Play();
		}
	}
}

IReplayMovie* CReplayPreviewPanelMovie::GetReplayMovie()
{
	return static_cast< IReplayMovie * >( m_pItemManager->GetItem( m_hItem ) );
}

void CReplayPreviewPanelMovie::LayoutView( int &nWide, int &nTall, int &nCurY )
{
	// Get frame dimensions
	int nFrameWidth, nFrameHeight;
	IReplayMovie* pReplayMovie = GetReplayMovie();
	pReplayMovie->GetFrameDimensions( nFrameWidth, nFrameHeight );

	int nScaledWidth = nFrameWidth;
	int nScaledHeight = nFrameHeight;

	// Scale the screenshot if it's too big for the current resolution
	float flScale = 1.0f;
	int nMaxWidth = ScreenWidth() / 3;
	if ( nFrameWidth > nMaxWidth )
	{
		flScale = (float)nMaxWidth / nFrameWidth;
		nScaledWidth = nMaxWidth;
		nScaledHeight = nFrameHeight * flScale;
	}

	nWide += nScaledWidth;
	nTall += nScaledHeight;
	nCurY = nTall;

	// Layout movie player panel if it's ready
	if ( m_pMoviePlayerPanel )
	{
		m_pMoviePlayerPanel->SetBounds( 9, 9, nScaledWidth, nScaledHeight );
		m_pMoviePlayerPanel->SetEnabled( true );
		m_pMoviePlayerPanel->SetVisible( true );
		m_pMoviePlayerPanel->SetZPos( 101 );
	}
}

#endif
