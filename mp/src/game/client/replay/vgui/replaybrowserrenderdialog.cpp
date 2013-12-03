//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowserrenderdialog.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "replay/genericclassbased_replay.h"
#include "ienginevgui.h"
#include "replayrenderoverlay.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplaymoviemanager.h"
#include "video/ivideoservices.h"
#include "confirm_dialog.h"
#include "replay/replayrenderer.h"

#include "replay/performance.h"
#include "replay/replayvideo.h"
#include "replay_gamestats_shared.h"

#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;

//-----------------------------------------------------------------------------

ConVar replay_rendersetting_quitwhendone( "replay_rendersetting_quitwhendone", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "Quit after rendering is completed.", true, 0.0f, true, 1.0f );
ConVar replay_rendersetting_exportraw( "replay_rendersetting_exportraw", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Export raw TGA frames and a .wav file, instead of encoding a movie file.", true, 0.0f, true, 1.0f );
ConVar replay_rendersetting_motionblurquality( "replay_rendersetting_motionblurquality", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "Motion blur quality.", true, 0, true, MAX_MOTION_BLUR_QUALITY );
ConVar replay_rendersetting_motionblurenabled( "replay_rendersetting_motionblurenabled", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "Motion blur enabled/disabled.", true, 0.0f, true, 1.0f );
ConVar replay_rendersetting_encodingquality( "replay_rendersetting_encodingquality", "100", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "Render quality: the higher the quality, the larger the resulting movie file size.", true, 0, true, 100 );
ConVar replay_rendersetting_motionblur_can_toggle( "replay_rendersetting_motionblur_can_toggle", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "" );
ConVar replay_rendersetting_renderglow( "replay_rendersetting_renderglow", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Glow effect enabled/disabled.", true, 0.0f, true, 1.0f );

//-----------------------------------------------------------------------------

CReplayRenderDialog::CReplayRenderDialog( Panel *pParent, ReplayHandle_t hReplay, bool bSetQuit, int iPerformance )
:	BaseClass( pParent, "RenderDialog" ),
	m_bShowAdvancedOptions( false ),
	m_hReplay( hReplay ),
	m_bSetQuit( bSetQuit ),
	m_iPerformance( iPerformance ),
	m_pVideoModesCombo( NULL ),
	m_pCodecCombo( NULL ),
	m_pPlayVoiceCheck( NULL ),
	m_pShowAdvancedOptionsCheck( NULL ),
	m_pQuitWhenDoneCheck( NULL ),
	m_pExportRawCheck( NULL ),
	m_pTitleText( NULL ),
	m_pResolutionNoteLabel( NULL ),
	m_pEnterANameLabel( NULL ),
	m_pVideoModeLabel( NULL ),
	m_pCodecLabel( NULL ),
	m_pMotionBlurLabel( NULL ),
	m_pMotionBlurSlider( NULL ),
	m_pQualityLabel( NULL ),
	m_pQualitySlider( NULL ),
	m_pTitleLabel( NULL ),
	m_pCancelButton( NULL ),
	m_pRenderButton( NULL ),
	m_pBgPanel( NULL ),
	m_pMotionBlurCheck( NULL ),
	m_pQualityPresetLabel( NULL ),
	m_pQualityPresetCombo( NULL ),
	m_pSeparator( NULL ),
	m_pGlowEnabledCheck( NULL )
{
	m_iQualityPreset = ReplayVideo_GetDefaultQualityPreset();
}

void CReplayRenderDialog::UpdateControlsValues()
{
	ConVarRef replay_voice_during_playback( "replay_voice_during_playback" );

	m_pQuitWhenDoneCheck->SetSelected( replay_rendersetting_quitwhendone.GetBool() );
	m_pExportRawCheck->SetSelected( replay_rendersetting_exportraw.GetBool() );
	m_pShowAdvancedOptionsCheck->SetSelected( m_bShowAdvancedOptions );
	m_pMotionBlurSlider->SetValue( replay_rendersetting_motionblurquality.GetInt() );
	m_pMotionBlurCheck->SetSelected( replay_rendersetting_motionblurenabled.GetBool() );
	m_pQualitySlider->SetValue( replay_rendersetting_encodingquality.GetInt() / ReplayVideo_GetQualityInterval() );

	if ( m_pGlowEnabledCheck )
	{
		m_pGlowEnabledCheck->SetSelected( replay_rendersetting_renderglow.GetBool() );
	}

	if ( replay_voice_during_playback.IsValid() )
	{
		m_pPlayVoiceCheck->SetSelected( replay_voice_during_playback.GetBool() );
	}
	else
	{
		m_pPlayVoiceCheck->SetEnabled( false );
	}
}

void CReplayRenderDialog::AddControlToAutoLayout( Panel *pPanel, bool bAdvanced )
{
	LayoutInfo_t *pNewLayoutInfo = new LayoutInfo_t;
	pNewLayoutInfo->pPanel = pPanel;

	// Use the positions from the .res file as relative positions for auto-layout
	pPanel->GetPos( pNewLayoutInfo->nOffsetX, pNewLayoutInfo->nOffsetY );
	
	pNewLayoutInfo->bAdvanced = bAdvanced;

	// Add to the list
	m_lstControls.AddToTail( pNewLayoutInfo );
}

void CReplayRenderDialog::SetValuesFromQualityPreset()
{
	const ReplayQualityPreset_t &preset = ReplayVideo_GetQualityPreset( m_iQualityPreset );
	replay_rendersetting_motionblurquality.SetValue( preset.m_iMotionBlurQuality );
	replay_rendersetting_motionblurenabled.SetValue( (int)preset.m_bMotionBlurEnabled );
	replay_rendersetting_encodingquality.SetValue( preset.m_iQuality );
	for ( int i = 0; i < ReplayVideo_GetCodecCount(); ++i )
	{
		const ReplayCodec_t &CurCodec = ReplayVideo_GetCodec( i );
		if ( CurCodec.m_nCodecId == preset.m_nCodecId )
		{
			m_pCodecCombo->ActivateItem( m_pCodecCombo->GetItemIDFromRow( i ) );
			break;
		}
	}
	UpdateControlsValues();
	InvalidateLayout();
}

void CReplayRenderDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	int i;

	// Link in TF scheme
	extern IEngineVGui *enginevgui;
	vgui::HScheme pTFScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( pTFScheme );
	SetProportional( true );

	BaseClass::ApplySchemeSettings( vgui::scheme()->GetIScheme( pTFScheme ) );

	LoadControlSettings( "Resource/UI/replaybrowser/renderdialog.res", "GAME" );

	// retrieve controls
	m_pPlayVoiceCheck = dynamic_cast< CheckButton * >( FindChildByName( "PlayVoice" ) );
	m_pShowAdvancedOptionsCheck = dynamic_cast< CheckButton * >( FindChildByName( "ShowAdvancedOptions" ) );
	m_pQuitWhenDoneCheck = dynamic_cast< CheckButton * >( FindChildByName( "QuitWhenDone" ) );
	m_pExportRawCheck = dynamic_cast< CheckButton * >( FindChildByName( "ExportRaw" ) );
	m_pTitleText = dynamic_cast< TextEntry * >( FindChildByName( "TitleInput" ) );
	m_pResolutionNoteLabel = dynamic_cast< CExLabel * >( FindChildByName( "ResolutionNoteLabel" ) );
	m_pEnterANameLabel = dynamic_cast< CExLabel * >( FindChildByName( "EnterANameLabel" ) );
	m_pVideoModeLabel = dynamic_cast< CExLabel * >( FindChildByName( "VideoModeLabel" ) );
	m_pCodecLabel = dynamic_cast< CExLabel * >( FindChildByName( "CodecLabel" ) );
	m_pMotionBlurLabel = dynamic_cast< CExLabel * >( FindChildByName( "MotionBlurLabel" ) );
	m_pMotionBlurSlider = dynamic_cast< Slider * >( FindChildByName( "MotionBlurSlider" ) );
	m_pQualityLabel = dynamic_cast< CExLabel * >( FindChildByName( "QualityLabel" ) );
	m_pQualitySlider = dynamic_cast< Slider * >( FindChildByName( "QualitySlider" ) );
	m_pTitleLabel = dynamic_cast< CExLabel * >( FindChildByName( "TitleLabel" ) );
	m_pRenderButton = dynamic_cast< CExButton * >( FindChildByName( "RenderButton" ) );
	m_pCancelButton = dynamic_cast< CExButton * >( FindChildByName( "CancelButton" ) );
	m_pBgPanel = dynamic_cast< EditablePanel * >( FindChildByName( "BGPanel" ) );
	m_pMotionBlurCheck = dynamic_cast< CheckButton * >( FindChildByName( "MotionBlurEnabled" ) );
	m_pQualityPresetLabel = dynamic_cast< CExLabel * >( FindChildByName( "QualityPresetLabel" ) );
	m_pQualityPresetCombo = dynamic_cast< vgui::ComboBox * >( FindChildByName( "QualityPresetCombo" ) );
	m_pCodecCombo = dynamic_cast< vgui::ComboBox * >( FindChildByName( "CodecCombo" ) );
	m_pVideoModesCombo = dynamic_cast< vgui::ComboBox * >( FindChildByName( "VideoModeCombo" ) );
	m_pEstimateTimeLabel = dynamic_cast< CExLabel * >( FindChildByName( "EstimateTimeLabel" ) );
	m_pEstimateFileLabel = dynamic_cast< CExLabel * >( FindChildByName( "EstimateFileLabel" ) );
	m_pSeparator = FindChildByName( "SeparatorLine" );
	m_pGlowEnabledCheck = dynamic_cast< CheckButton * >( FindChildByName( "GlowEnabled" ) );
	m_pLockWarningLabel = dynamic_cast< CExLabel * >( FindChildByName( "LockWarningLabel" ) );

#if defined( TF_CLIENT_DLL )
	if ( m_pBgPanel )
	{
		m_pBgPanel->SetPaintBackgroundType( 2 );	// Rounded.
	}
#endif

	AddControlToAutoLayout( m_pTitleLabel, false );

	// The replay may be REPLAY_HANDLE_INVALID in the case that we are about to render all unrendered replays
	if ( m_hReplay != REPLAY_HANDLE_INVALID )
	{
		CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
		m_pTitleText->SetText( pReplay->m_wszTitle );
		m_pTitleText->SetVisible( true );
		m_pTitleLabel->SetText( "#Replay_RenderReplay" );
		m_pEnterANameLabel->SetVisible( true );

		AddControlToAutoLayout( m_pEnterANameLabel, false );
	}
	else
	{
		m_pTitleLabel->SetText( "#Replay_RenderReplays" );
	}

	m_pTitleText->SelectAllOnFocusAlways( true );

	AddControlToAutoLayout( m_pTitleText, false );

	// Update controls based on preset
	SetValuesFromQualityPreset();

	// Set quit button if necessary
	if ( m_bSetQuit )
	{
		m_pQuitWhenDoneCheck->SetSelected( true );
	}

	m_pPlayVoiceCheck->SetProportional( false );
	m_pQuitWhenDoneCheck->SetProportional( false );
	m_pShowAdvancedOptionsCheck->SetProportional( false );
	m_pMotionBlurCheck->SetProportional( false );
	m_pMotionBlurSlider->InvalidateLayout( false, true );	// Without this, the range labels show up with "..." because of an invalid font in TextImage::ApplySchemeSettings().
	m_pExportRawCheck->SetProportional( false );
	m_pQualitySlider->InvalidateLayout( false, true );	// Without this, the range labels show up with "..." because of an invalid font in TextImage::ApplySchemeSettings().

	if ( m_pGlowEnabledCheck )
	{
		m_pGlowEnabledCheck->SetProportional( false );
	}

	// Fill in combo box with preset quality levels
	const int nQualityPresetCount = ReplayVideo_GetQualityPresetCount();
	m_pQualityPresetCombo->SetNumberOfEditLines( nQualityPresetCount );
	for ( i = 0; i < nQualityPresetCount; ++i )
	{
		const ReplayQualityPreset_t &CurQualityPreset = ReplayVideo_GetQualityPreset( i );
		m_pQualityPresetCombo->AddItem( CurQualityPreset.m_pName, NULL );
		m_pQualityPresetCombo->SetItemEnabled( i, true );
	}
	m_pQualityPresetCombo->ActivateItem( m_pQualityPresetCombo->GetItemIDFromRow( m_iQualityPreset ) );

	// Fill in combo box with video modes
	int nScreenW = ScreenWidth();
	int nScreenH = ScreenHeight();
	const int nVidModeCount = ReplayVideo_GetVideoModeCount();
	m_pVideoModesCombo->SetNumberOfEditLines( nVidModeCount );
	bool bAtLeastOneVideoModeAdded = false;
	bool bEnable = false;
	bool bSkipped = false;
	
	for ( i = 0; i < nVidModeCount; ++i )
	{
		// Only offer display modes less than the current window size
		const ReplayVideoMode_t	&CurVideoMode = ReplayVideo_GetVideoMode( i );

		int nMw = CurVideoMode.m_nWidth;
		int nMh = CurVideoMode.m_nHeight;

		// Only display modes that fit in the current window
		bEnable =  ( nMw <= nScreenW && nMh <= nScreenH );
		if (!bEnable)
			bSkipped = true;
		
		m_pVideoModesCombo->AddItem( CurVideoMode.m_pName, NULL );
		m_pVideoModesCombo->SetItemEnabled( i, bEnable );

		if (bEnable)
			bAtLeastOneVideoModeAdded = true;
	}
	if ( bAtLeastOneVideoModeAdded )
	{
		m_pVideoModesCombo->ActivateItem( m_pVideoModesCombo->GetItemIDFromRow( 0 ) );
	}

	// fill in the combo box with codecs
	const int nNumCodecs = ReplayVideo_GetCodecCount();
	m_pCodecCombo->SetNumberOfEditLines( nNumCodecs );
	for ( i = 0; i < nNumCodecs; ++i )
	{
		const ReplayCodec_t &CurCodec = ReplayVideo_GetCodec( i );
		m_pCodecCombo->AddItem( CurCodec.m_pName, NULL );
		m_pCodecCombo->SetItemEnabled( i, true );
	}
	m_pCodecCombo->ActivateItem( m_pCodecCombo->GetItemIDFromRow( 0 ) );

	// now layout

	// simplified options
	AddControlToAutoLayout( m_pVideoModeLabel, false );
	AddControlToAutoLayout( m_pVideoModesCombo, false );
	// Show the note about "not all resolutions are available?"
	if ( bSkipped && m_pResolutionNoteLabel )
	{
		m_pResolutionNoteLabel->SetVisible( true );
		AddControlToAutoLayout( m_pResolutionNoteLabel, false );
	}
	// other simplified options
	AddControlToAutoLayout( m_pQualityPresetLabel, false );
	AddControlToAutoLayout( m_pQualityPresetCombo, false );
	AddControlToAutoLayout( m_pEstimateTimeLabel, false );
	AddControlToAutoLayout( m_pEstimateFileLabel, false );
	AddControlToAutoLayout( m_pPlayVoiceCheck, false );
	AddControlToAutoLayout( m_pShowAdvancedOptionsCheck, false );
	AddControlToAutoLayout( m_pQuitWhenDoneCheck, false );

	AddControlToAutoLayout( m_pLockWarningLabel, false );

	// now advanced options
	AddControlToAutoLayout( m_pSeparator, true );

	AddControlToAutoLayout( m_pCodecLabel, true );
	AddControlToAutoLayout( m_pCodecCombo, true );

	if ( replay_rendersetting_motionblur_can_toggle.GetBool() )
	{
		AddControlToAutoLayout( m_pMotionBlurCheck, true );
	}
	else
	{
		m_pMotionBlurCheck->SetVisible( false );
	}
	AddControlToAutoLayout( m_pMotionBlurLabel, true );
	AddControlToAutoLayout( m_pMotionBlurSlider, true );

	AddControlToAutoLayout( m_pQualityLabel, true );
	AddControlToAutoLayout( m_pQualitySlider, true );

	AddControlToAutoLayout( m_pExportRawCheck, true );

	if ( m_pGlowEnabledCheck )
	{
		AddControlToAutoLayout( m_pGlowEnabledCheck, true );
	}

	// these buttons always show up
	AddControlToAutoLayout( m_pRenderButton, false );
	AddControlToAutoLayout( m_pCancelButton, false );
}

void CReplayRenderDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pResolutionNoteLabel->SizeToContents();	// Get the proper height

	int nY = m_nStartY;
	Panel *pPrevPanel = NULL;
	int nLastCtrlHeight = 0;

	FOR_EACH_LL( m_lstControls, i )
	{
		LayoutInfo_t *pLayoutInfo = m_lstControls[ i ];
		Panel *pPanel = pLayoutInfo->pPanel;

		// should an advanced option be shown?
		if ( pLayoutInfo->bAdvanced )
		{
			if ( pPanel->IsVisible() != m_bShowAdvancedOptions )
			{
				pPanel->SetVisible( m_bShowAdvancedOptions );
			}
		}

		if ( !pPanel->IsVisible() )
			continue;

		if ( pPrevPanel && pLayoutInfo->nOffsetY >= 0 )
		{
			nY += pPrevPanel->GetTall() + pLayoutInfo->nOffsetY + m_nVerticalBuffer;
		}

		pPanel->SetPos( pLayoutInfo->nOffsetX ? pLayoutInfo->nOffsetX : m_nDefaultX, nY );

		pPrevPanel = pPanel;
		nLastCtrlHeight = pPanel->GetTall();
	}

	m_pBgPanel->SetTall( nY + nLastCtrlHeight + 2 * m_nVerticalBuffer );
}

void CReplayRenderDialog::Close()
{
	SetVisible( false );
	MarkForDeletion();
	TFModalStack()->PopModal( this );
}

void CReplayRenderDialog::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "cancel" ) )
	{
		Close();
	}
	else if ( FStrEq( pCommand, "render" ) )
	{
		Close();
		Render();
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( pCommand ) );
	}

	BaseClass::OnCommand( pCommand );
}

void CReplayRenderDialog::Render()
{
	// Only complain about QuickTime if we aren't exporting raw TGA's/WAV
	if ( !m_pExportRawCheck->IsSelected() )
	{
#ifndef USE_WEBM_FOR_REPLAY
		if ( !g_pVideo || !g_pVideo->IsVideoSystemAvailable( VideoSystem::QUICKTIME ) )
		{
			ShowMessageBox( "#Replay_QuicktimeTitle", "#Replay_NeedQuicktime", "#GameUI_OK" );
			return;
		}
		
		if ( g_pVideo->GetVideoSystemStatus( VideoSystem::QUICKTIME ) != VideoSystemStatus::OK )
		{
			if ( g_pVideo->GetVideoSystemStatus( VideoSystem::QUICKTIME ) == VideoSystemStatus::NOT_CURRENT_VERSION )
			{
				ShowMessageBox( "#Replay_QuicktimeTitle", "#Replay_NeedQuicktimeNewer", "#GameUI_OK" );
				return;	
			}
		
			ShowMessageBox( "#Replay_QuicktimeTitle", "#Replay_Err_QT_FailedToLoad", "#GameUI_OK" );
			return;
		}
#endif
	}
	
	// Update convars from settings
	const int nMotionBlurQuality = clamp( m_pMotionBlurSlider->GetValue(), 0, MAX_MOTION_BLUR_QUALITY );
	replay_rendersetting_quitwhendone.SetValue( (int)m_pQuitWhenDoneCheck->IsSelected() );
	replay_rendersetting_exportraw.SetValue( (int)m_pExportRawCheck->IsSelected() );
	replay_rendersetting_motionblurquality.SetValue( nMotionBlurQuality );
	replay_rendersetting_motionblurenabled.SetValue( replay_rendersetting_motionblur_can_toggle.GetBool() ? (int)m_pMotionBlurCheck->IsSelected() : 1 );
	replay_rendersetting_encodingquality.SetValue( clamp( m_pQualitySlider->GetValue() * ReplayVideo_GetQualityInterval(), 0, 100 ) );

	if ( m_pGlowEnabledCheck )
	{
		replay_rendersetting_renderglow.SetValue( m_pGlowEnabledCheck->IsSelected() );
	}

	ConVarRef replay_voice_during_playback( "replay_voice_during_playback" );
	if ( replay_voice_during_playback.IsValid() && m_pPlayVoiceCheck->IsEnabled() )
	{
		replay_voice_during_playback.SetValue( (int)m_pPlayVoiceCheck->IsSelected() );
	}

	// Setup parameters for render
	RenderMovieParams_t params;
	params.m_hReplay = m_hReplay;
	params.m_iPerformance = m_iPerformance;	// Use performance passed in from details panel
	params.m_bQuitWhenFinished = m_pQuitWhenDoneCheck->IsSelected();
	params.m_bExportRaw = m_pExportRawCheck->IsSelected();
	m_pTitleText->GetText( params.m_wszTitle, sizeof( params.m_wszTitle ) );
#ifdef USE_WEBM_FOR_REPLAY
	V_strcpy( params.m_szExtension, ".webm" );	// Use .webm
#else
	V_strcpy( params.m_szExtension, ".mov" );	// Use .mov for Quicktime
#endif

	const int iRes = m_pVideoModesCombo->GetActiveItem();
	const ReplayVideoMode_t &VideoMode = ReplayVideo_GetVideoMode( iRes );

	params.m_Settings.m_bMotionBlurEnabled = replay_rendersetting_motionblurenabled.GetBool();
	params.m_Settings.m_bAAEnabled = replay_rendersetting_motionblurenabled.GetBool();
	params.m_Settings.m_nMotionBlurQuality = nMotionBlurQuality;
	params.m_Settings.m_nWidth = VideoMode.m_nWidth;
	params.m_Settings.m_nHeight = VideoMode.m_nHeight;
	params.m_Settings.m_FPS.SetFPS( VideoMode.m_nBaseFPS, VideoMode.m_bNTSCRate );
	params.m_Settings.m_Codec = ReplayVideo_GetCodec( m_pCodecCombo->GetActiveItem() ).m_nCodecId;
	params.m_Settings.m_nEncodingQuality = replay_rendersetting_encodingquality.GetInt();
	params.m_Settings.m_bRaw = m_pExportRawCheck->IsSelected();

	// Calculate the framerate for the engine - for each engine frame, we need the # of motion blur timesteps,
	// x 2, since the shutter is open for nNumMotionBlurTimeSteps and closed for nNumMotionBlurTimeSteps,
	// with the engine frame centered in the shutter open state (ie when we're half way through the motion blur
	// timesteps).  Antialiasing does not factor in here because it doesn't require extra frames - the AA jitter
	// is interwoven in with the motion sub-frames.
	const int nNumMotionBlurTimeSteps = ( params.m_Settings.m_bMotionBlurEnabled ) ? CReplayRenderer::GetNumMotionBlurTimeSteps( params.m_Settings.m_nMotionBlurQuality ) : 1;
	
	if ( params.m_Settings.m_bMotionBlurEnabled )
	{
	 	params.m_flEngineFps = 2 * nNumMotionBlurTimeSteps * params.m_Settings.m_FPS.GetFPS();
	}
	else
	{
		Assert( nNumMotionBlurTimeSteps == 1 );
		params.m_flEngineFps = params.m_Settings.m_FPS.GetFPS();
	}

	// Close the browser
	extern void ReplayUI_CloseReplayBrowser();
	ReplayUI_CloseReplayBrowser();

	// Hide the console
	engine->ExecuteClientCmd( "hideconsole" );

	// Stats tracking.
	GetReplayGameStatsHelper().SW_ReplayStats_WriteRenderDataStart( params, this );

	// Render the movie
	g_pReplayMovieManager->RenderMovie( params );
}

void CReplayRenderDialog::OnKeyCodeTyped( vgui::KeyCode code )
{
	if( code == KEY_ENTER )
	{
		OnCommand( "render" );
	}
	else if ( code == KEY_ESCAPE )
	{
		MarkForDeletion();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

void CReplayRenderDialog::OnThink()
{
	if ( m_pEstimateTimeLabel == NULL || m_pEstimateFileLabel == NULL )
		return;

	// The replay may be NULL if this dialog is created by 'save all' from the quit confirmation dialog.  In this
	// case, we don't want to a replay-specific time estimate anyway, so we can just early out here.
	CGenericClassBasedReplay *pReplay = ToGenericClassBasedReplay( g_pReplayManager->GetReplay( m_hReplay) );
	if ( !pReplay )
		return;

	const int nMotionBlurQuality = clamp( m_pMotionBlurSlider->GetValue(), 0, MAX_MOTION_BLUR_QUALITY );
	const int nCodecQuality = clamp( m_pQualitySlider->GetValue(), 0, ReplayVideo_GetQualityRange() );
	VideoEncodeCodec::EVideoEncodeCodec_t eCodec = ReplayVideo_GetCodec( m_pCodecCombo->GetActiveItem() ).m_nCodecId;

	// fFrameSize is the scale factor based on the size of the rendered frame.  
	const int iRes = m_pVideoModesCombo->GetActiveItem();
	const ReplayVideoMode_t &VideoMode = ReplayVideo_GetVideoMode( iRes );
	float fFrameSize = (float)(VideoMode.m_nWidth * VideoMode.m_nHeight)/(float)(640*480);
	

	float flEstimatedFileSize = 0;
	float flEstimatedRenderTime_Min = 0;

	static float mjpegToMotionBlurMultiplierTable[] = { 2.0f, 3.0f, 5.5f, 12.0f };
	static float h264ToMotionBlurMultiplierTable[] = { 2.8f, 4.2f, 6.4f, 13.0f };
	static float webmToMotionBlurMultiplierTable[] = { 2.8f, 4.2f, 6.4f, 13.0f };

	static float mjpegToQualityMultiplierTable[] = { 620.0f, 736.0f, 1284.0f, 2115.0f, 3028.0f };
	static float h264ToQualityMultiplierTable[] = { 276.0f, 384.0f, 595.0f, 1026.0f, 1873.0f };
	static float webmToQualityMultiplierTable[] = { 125.0f, 250.0f, 312.0f, 673.0f, 1048.0f };

	switch ( eCodec )
	{
	case VideoEncodeCodec::WEBM_CODEC:
		flEstimatedFileSize			= pReplay->m_flLength * webmToQualityMultiplierTable[nCodecQuality]*fFrameSize;
		flEstimatedRenderTime_Min	= pReplay->m_flLength * webmToMotionBlurMultiplierTable[nMotionBlurQuality];
		break;
	case VideoEncodeCodec::H264_CODEC:
		flEstimatedFileSize			= pReplay->m_flLength * h264ToQualityMultiplierTable[nCodecQuality];
		flEstimatedRenderTime_Min	= pReplay->m_flLength * h264ToMotionBlurMultiplierTable[nMotionBlurQuality];
		break;
	case VideoEncodeCodec::MJPEG_A_CODEC:
		flEstimatedFileSize			= pReplay->m_flLength * mjpegToQualityMultiplierTable[nCodecQuality];
		flEstimatedRenderTime_Min	= pReplay->m_flLength * mjpegToMotionBlurMultiplierTable[nMotionBlurQuality];
		break;
	}

	float flEstimatedRenderTime_Max = flEstimatedRenderTime_Min * 3.0f;

	// @todo Tom Bui: if this goes into hours, we are in trouble...
	wchar_t wzFileSize[64];
	_snwprintf( wzFileSize, ARRAYSIZE( wzFileSize ), L"%d", (int)flEstimatedFileSize );
	wchar_t wzTimeMin[64];
	wchar_t wzTimeMax[64];
	g_pVGuiLocalize->ConvertANSIToUnicode( CReplayTime::FormatTimeString( flEstimatedRenderTime_Min ), wzTimeMin, sizeof( wzTimeMin ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( CReplayTime::FormatTimeString( flEstimatedRenderTime_Max ), wzTimeMax, sizeof( wzTimeMax ) );

	wchar_t wzText[256] = L"";

	g_pVGuiLocalize->ConstructString( wzText, sizeof( wzText ), g_pVGuiLocalize->Find( "#Replay_RenderEstimate_File" ), 1, 
									  wzFileSize,
									  wzTimeMin,
									  wzTimeMax );
	m_pEstimateFileLabel->SetText( wzText );

	g_pVGuiLocalize->ConstructString( wzText, sizeof( wzText ), g_pVGuiLocalize->Find( "#Replay_RenderEstimate_Time" ), 2, 
									  wzTimeMin,
									  wzTimeMax );
	m_pEstimateTimeLabel->SetText( wzText );
}

void CReplayRenderDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );

	if ( pComboBox == m_pQualityPresetCombo )
	{
		m_iQualityPreset = m_pQualityPresetCombo->GetActiveItem();
		SetValuesFromQualityPreset();
	}
}

void CReplayRenderDialog::OnCheckButtonChecked( vgui::Panel *panel )
{
	if ( panel == m_pShowAdvancedOptionsCheck )
	{
		m_bShowAdvancedOptions = m_pShowAdvancedOptionsCheck->IsSelected();
		InvalidateLayout( true, false );
	}
}

void CReplayRenderDialog::OnSetFocus()
{
	m_pTitleText->RequestFocus();
}

void ReplayUI_ShowRenderDialog( Panel* pParent, ReplayHandle_t hReplay, bool bSetQuit, int iPerformance )
{
	CReplayRenderDialog *pRenderDialog = vgui::SETUP_PANEL( new CReplayRenderDialog( pParent, hReplay, bSetQuit, iPerformance ) );

	pRenderDialog->SetVisible( true );
	pRenderDialog->MakePopup();
	pRenderDialog->MoveToFront();
	pRenderDialog->SetKeyBoardInputEnabled( true );
	pRenderDialog->SetMouseInputEnabled( true );
	TFModalStack()->PushModal( pRenderDialog );
}

#endif
