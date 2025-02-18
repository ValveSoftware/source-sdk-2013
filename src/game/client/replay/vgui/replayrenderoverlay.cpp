//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayrenderoverlay.h"
#include "vgui_controls/TextImage.h"
#include "replay/genericclassbased_replay.h"
#include "iclientmode.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "ienginevgui.h"
#include "vgui/IVGui.h"
#include "econ/confirm_dialog.h"
#include "replay/ireplaymanager.h"
#include "replay/irecordingsessionmanager.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/replayrenderer.h"
#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

#define TMP_ENCODED_AUDIO			".tmp.aac"
#ifdef USE_WEBM_FOR_REPLAY
#define TMP_ENCODED_VIDEO			".tmp.webm"
#else
#define TMP_ENCODED_VIDEO			".tmp.mov"
#endif

//-----------------------------------------------------------------------------

ConVar replay_enablerenderpreview( "replay_enablerenderpreview", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Enable preview during replay render." );

//-----------------------------------------------------------------------------

void OnRenderCancelDialogButtonPressed( bool bConfirm, void *pContext )
{
	if ( bConfirm )
	{
		g_pReplayMovieManager->CancelRender();
	}
}

//-----------------------------------------------------------------------------

CReplayRenderOverlay::CReplayRenderOverlay( Panel *pParent ) 
:	BaseClass( pParent, "ReplayRenderOverlay" ),
	m_pBottom( NULL ),
	m_pCancelButton( NULL ),
	m_pTitleLabel( NULL ),
	m_pProgressLabel( NULL ),
	m_pFilenameLabel( NULL ),
	m_pRenderProgress( NULL ),
	m_pRenderer( NULL ),
	m_pPreviewCheckButton( NULL ),
	m_unNumFrames( 0 ),
	m_flStartTime( 0.0f ),
	m_flPreviousTimeLeft( 0.0f )
{
	if ( pParent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );
	}

	ivgui()->AddTickSignal( GetVPanel(), 10 );

	m_pRenderer = new CReplayRenderer( this );
}

CReplayRenderOverlay::~CReplayRenderOverlay()
{
	ivgui()->RemoveTickSignal( GetVPanel() );

	delete m_pRenderer;
}

void CReplayRenderOverlay::Show()
{
	// Setup panel
	SetVisible( true );
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	MakePopup( true );
	MoveToFront();
	TFModalStack()->PushModal( this );

	// Make sure game UI is hidden
	engine->ClientCmd_Unrestricted( "gameui_hide" );

	InvalidateLayout( false, true );
}

void CReplayRenderOverlay::Hide()
{
	SetVisible( false );
	TFModalStack()->PopModal( this );
	MarkForDeletion();
}

void CReplayRenderOverlay::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Load controls
	LoadControlSettings( "Resource/UI/replayrenderoverlay.res", "GAME" );

	// Layout bottom
	m_pBottom = dynamic_cast< EditablePanel * >( FindChildByName( "BottomPanel" ) );
	if ( !m_pBottom )
		return;

	// Find some controls
	m_pTitleLabel = dynamic_cast< CExLabel * >( FindChildByName( "TitleLabel" ) );
	m_pProgressLabel = dynamic_cast< CExLabel * >( FindChildByName( "ProgressLabel" ) );
	m_pRenderProgress = dynamic_cast< ProgressBar * >( FindChildByName( "RenderProgress" ) );
	m_pCancelButton = dynamic_cast< CExButton * >( FindChildByName( "CancelButton" ) );
	m_pFilenameLabel = dynamic_cast< CExLabel * >( FindChildByName( "FilenameLabel" ) );
	m_pPreviewCheckButton = dynamic_cast< CheckButton * >( FindChildByName( "PreviewCheckButton" ) );

	m_pPreviewCheckButton->SetProportional( false );
	m_pPreviewCheckButton->SetSelected( replay_enablerenderpreview.GetBool() );
	m_pPreviewCheckButton->AddActionSignalTarget( this );

	const char *pMovieFilename = m_pRenderer->GetMovieFilename();
	if ( m_pFilenameLabel && pMovieFilename )
	{
		const char *pFilename = V_UnqualifiedFileName( pMovieFilename );
		m_pFilenameLabel->SetText( pFilename );
	}
}

void CReplayRenderOverlay::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( !m_pBottom )
		return;

	int sw, sh;
	vgui::surface()->GetScreenSize( sw, sh );
	SetBounds( 0, 0, sw, sh );

	int nBottomPanelHeight = sh * .13f;
	int nBottomPanelStartY = sh - nBottomPanelHeight;
	m_pBottom->SetBounds( 0, nBottomPanelStartY, sw, nBottomPanelHeight );

	int nBottomW = sw;
	int nBottomH = nBottomPanelHeight;

	// Setup progress bar
	if ( !m_pRenderProgress )
		return;

	int nProgHeight = YRES(20);
	int nMargin = nBottomW/5;
	int nProgX = nMargin;
	int nProgY = nBottomPanelStartY + ( nBottomH - nProgHeight ) / 2;
	int nProgW = nBottomW - 2*nMargin;

	// Only show progress bar if replay is valid and length of render is non-zero, and the record start tick exists
	CReplay *pReplay = g_pReplayManager->GetPlayingReplay();
	if ( pReplay )
	{
		const float flTotalTime = pReplay->m_flLength;
		const int nServerRecordStartTick = g_pClientReplayContext->GetRecordingSessionManager()->GetServerStartTickForSession( pReplay->m_hSession );	// NOTE: Returns -1 on fail
		if ( flTotalTime > 0.0f && nServerRecordStartTick >= 0 )
		{
			m_pRenderProgress->SetVisible( true );
			m_pRenderProgress->SetBounds( nProgX, nProgY, nProgW, nProgHeight );
			m_pRenderProgress->SetSegmentInfo( XRES(1), XRES(8) );
		}
	}

	// Layout title label
	const int nTitleLabelY = nBottomPanelStartY + ( m_pBottom->GetTall() - m_pTitleLabel->GetTall() ) / 2;
	if ( m_pTitleLabel )
	{
		m_pTitleLabel->SizeToContents();
		m_pTitleLabel->SetPos( ( nProgX - m_pTitleLabel->GetWide() ) / 2, nTitleLabelY );
	}

	// Layout preview check button
	if ( m_pPreviewCheckButton )
	{
		m_pPreviewCheckButton->SizeToContents();
		m_pPreviewCheckButton->SetPos( ( nProgX - m_pPreviewCheckButton->GetWide() ) / 2, nTitleLabelY + m_pTitleLabel->GetTall() + YRES(3) );
	}

	// Layout filename label
	if ( m_pFilenameLabel )
	{
		int nProgBottomY = nProgY + nProgHeight;
		m_pFilenameLabel->SizeToContents();
		m_pFilenameLabel->SetPos( nProgX, nProgBottomY + ( sh - nProgBottomY - m_pFilenameLabel->GetTall() ) / 2 );
	}

	// Layout progress label
	if ( m_pProgressLabel )
	{
		int nProgBottomY = nProgY + nProgHeight;
		m_pProgressLabel->SizeToContents();
		m_pProgressLabel->SetPos( nProgX, nProgBottomY + ( sh - nProgBottomY - m_pProgressLabel->GetTall() ) / 2 );
		m_pProgressLabel->SetWide( nProgW );
	}

	// Layout cancel button
	if ( !m_pCancelButton )
		return;

	// Put cancel button half way in between progress bar and screen right
	int nProgRightX = nProgX + nProgW;
	m_pCancelButton->SetPos(
		nProgRightX + ( m_pBottom->GetWide() - nProgRightX - m_pCancelButton->GetWide() ) / 2,
		nBottomPanelStartY + ( m_pBottom->GetTall() - m_pCancelButton->GetTall() ) / 2
	);

	SetXToRed( m_pCancelButton );

	m_pCancelButton->RequestFocus();
}

void CReplayRenderOverlay::OnTick()
{
#if _DEBUG
	if ( m_bReloadScheme )
	{
		InvalidateLayout( true, true );
		m_bReloadScheme = false;
	}
#endif

	// Update progress
	if ( m_pRenderProgress )
	{
		CReplay *pReplay = g_pReplayManager->GetPlayingReplay();
		if ( pReplay && m_pRenderProgress->IsVisible() )
		{
			float flCurTime, flTotalTime;
			g_pClientReplayContext->GetPlaybackTimes( flCurTime, flTotalTime, pReplay, m_pRenderer->GetPerformance() );
			const float flProgress = ( flTotalTime == 0.0f ) ? 1.0f : ( flCurTime / flTotalTime );

			Assert( flTotalTime > 0.0f );	// NOTE: Progress bar will always be invisible if total time is 0, but check anyway to be safe.
			m_pRenderProgress->SetProgress( MAX( m_pRenderProgress->GetProgress(), flProgress ) ); // The MAX() here keeps the progress bar from thrashing

			if ( m_pProgressLabel )
			{
				// @note Tom Bui: this is a horribly ugly hack, but the first couple of frames take a really freaking long time, so that
				// really blows out the estimate
				float flTimePassed = 0.0f;
				++m_unNumFrames;				
				const uint32 kNumFramesToWait = 10;
				if ( m_unNumFrames < kNumFramesToWait )
				{
					m_flStartTime = gpGlobals->realtime;
				}
				else if ( m_unNumFrames > kNumFramesToWait )
				{
					flTimePassed = gpGlobals->realtime - m_flStartTime;
					float flEstimatedTimeLeft = flProgress > 0.0f ? ( flTimePassed / flProgress ) - flTimePassed : 0.0f;
					// exponential moving average FIR filter
					// S(t) =  smoothing_factor * Y(t) + (1 - smoothing_factor)* Y(t-1)
					// previous value is essentially 90% of the current value
					const float kSmoothingFactor = 0.1f;
					if ( m_flPreviousTimeLeft == 0.0f )
					{
						m_flPreviousTimeLeft = flEstimatedTimeLeft;
					}
					else
					{
						m_flPreviousTimeLeft = kSmoothingFactor * flEstimatedTimeLeft + ( 1 - kSmoothingFactor ) * m_flPreviousTimeLeft;
					}
				}
				
				wchar_t wszTimeLeft[256];
				wchar_t wszTime[256];
				{
					const char *pRenderTime = CReplayTime::FormatTimeString( RoundFloatToInt( m_flPreviousTimeLeft ) );
					g_pVGuiLocalize->ConvertANSIToUnicode( pRenderTime, wszTimeLeft, sizeof( wszTimeLeft ) );
				}
				{
					const char *pRenderTime = CReplayTime::FormatTimeString( RoundFloatToInt( flTimePassed ) );
					g_pVGuiLocalize->ConvertANSIToUnicode( pRenderTime, wszTime, sizeof( wszTime ) );
				}
				wchar_t wszText[256];
				g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "#Replay_RenderOverlay_TimeLeft" ), 2, wszTime, wszTimeLeft );
				m_pProgressLabel->SetText( wszText );
			}
		}
	}
}

void CReplayRenderOverlay::OnMousePressed( MouseCode nCode )
{
#if _DEBUG
	m_bReloadScheme = true;
#endif
	BaseClass::OnMousePressed( nCode );
}

void CReplayRenderOverlay::OnKeyCodeTyped( vgui::KeyCode nCode )
{
	if ( nCode == KEY_ESCAPE )
	{
		if ( TFModalStack()->Top() == GetVPanel() )
		{
			OnCommand( "confirmcancel" );
			return;
		}
	}

	BaseClass::OnKeyCodeTyped( nCode );
}

void CReplayRenderOverlay::OnCommand( const char *pCommand )
{
	if ( !V_stricmp( pCommand, "confirmcancel" ) )
	{
		ShowConfirmDialog( "#Replay_CancelRenderTitle", "#Replay_ConfirmCancelRender", "#Replay_YesCancel", "#Replay_No", OnRenderCancelDialogButtonPressed, this, NULL, "replay\\replaydialog_warn.wav" );
		return;
	}

	BaseClass::OnCommand( pCommand );
}

void CReplayRenderOverlay::OnCheckButtonChecked( Panel *pPanel )
{
	replay_enablerenderpreview.SetValue( (int)m_pPreviewCheckButton->IsSelected() );
}

//-----------------------------------------------------------------------------

static CReplayRenderOverlay *s_pRenderOverlay = NULL;

void ReplayUI_OpenReplayRenderOverlay()
{
	if ( !g_pReplayMovieManager->IsRendering() )
		return;

	// Delete any existing panel
	if ( s_pRenderOverlay )
	{
		s_pRenderOverlay->MarkForDeletion();
	}

	// Create the panel - get the render resolution from the settings
	s_pRenderOverlay = SETUP_PANEL( new CReplayRenderOverlay( NULL ) );	// Parenting to NULL allows us to turn off world rendering in engine/view.cpp (V_RenderView())

	// Set the panel as the movie renderer, so it can receive begin/end render calls from the engine
	g_pClientReplayContext->SetMovieRenderer( s_pRenderOverlay->m_pRenderer );
}

void ReplayUI_HideRenderOverlay()
{
	if ( s_pRenderOverlay )
	{
		s_pRenderOverlay->MarkForDeletion();
		s_pRenderOverlay = NULL;
	}

	g_pClientReplayContext->SetMovieRenderer( NULL );
}

//-----------------------------------------------------------------------------

#endif
