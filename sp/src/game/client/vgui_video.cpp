//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#include "cbase.h"
#include <vgui/IVGui.h>
#include "vgui/IInput.h"
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui_video.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static CUtlVector< VideoPanel * > g_vecVideoPanels;

// Thiis is a hack due to the fact that the user can type quit with the video panel up, but it's parented to the GameUI dll root panel, which is already gone so
//  we would crash in the destructor
void VGui_ClearVideoPanels()
{
	for ( int i = g_vecVideoPanels.Count() - 1; i >= 0; --i )
	{
		if ( g_vecVideoPanels[ i ] )
		{
			delete g_vecVideoPanels[ i ];
		}
	}
	g_vecVideoPanels.RemoveAll();
}

struct VideoPanelParms_t
{
	VideoPanelParms_t( bool _interrupt = true, bool _loop = false, bool _mute = false )
	{
		bAllowInterrupt = _interrupt;
		bLoop = _loop;
		bMute = _mute;
	}

	bool bAllowInterrupt;
	bool bLoop;
	bool bMute;

	//float flFadeIn;
	//float flFadeOut;
};

VideoPanel::VideoPanel( unsigned int nXPos, unsigned int nYPos, unsigned int nHeight, unsigned int nWidth, bool allowAlternateMedia ) : 
	BaseClass( NULL, "VideoPanel" ),
	m_VideoMaterial( NULL ),
	m_nPlaybackWidth( 0 ),
	m_nPlaybackHeight( 0 ),
	m_nShutdownCount( 0 ),
	m_bLooping( false ),
	m_bStopAllSounds( true ),
	m_bAllowInterruption( true ),
	m_bAllowAlternateMedia( allowAlternateMedia ),
	m_bStarted( false )
{
#ifdef MAPBASE
	vgui::VPANEL pParent = enginevgui->GetPanel( PANEL_ROOT );
#else
	vgui::VPANEL pParent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
#endif

	SetParent( pParent );
	SetVisible( false );
	
	// Must be passed in, off by default
	m_szExitCommand[0] = '\0';

	m_bBlackBackground = true;

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( false );

	SetProportional( false );
	SetVisible( true );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	
	// Set us up
	SetTall( nHeight );
	SetWide( nWidth );
	SetPos( nXPos, nYPos );

	SetScheme(vgui::scheme()->LoadSchemeFromFile( "resource/VideoPanelScheme.res", "VideoPanelScheme"));
	LoadControlSettings("resource/UI/VideoPanel.res");

	// Let us update
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	g_vecVideoPanels.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Properly shutdown out materials
//-----------------------------------------------------------------------------
VideoPanel::~VideoPanel( void )
{
	g_vecVideoPanels.FindAndRemove( this );

	SetParent( (vgui::Panel *) NULL );

	// Shut down this video, destroy the video material
	if ( g_pVideo != NULL && m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keeps a tab on when the movie is ending and allows a frame to pass to prevent threading issues
//-----------------------------------------------------------------------------
void VideoPanel::OnTick( void ) 
{ 
	if ( m_nShutdownCount > 0 )
	{
		m_nShutdownCount++;

		if ( m_nShutdownCount > 10 )
		{
			OnClose();
			m_nShutdownCount = 0;
		}
	}

	BaseClass::OnTick(); 
}

void VideoPanel::OnVideoOver()
{
	StopPlayback();
}

//-----------------------------------------------------------------------------
// Purpose: Begins playback of a movie
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel::BeginPlayback( const char *pFilename )
{
	if ( !pFilename || pFilename[ 0 ] == '\0' )
		return false;

#ifdef _X360
	XVIDEO_MODE videoMode;
	XGetVideoMode( &videoMode );

	// for 50Hz PAL, load a 25Hz version of the ep1_recap movie.
	if( ( videoMode.RefreshRate < 59.0f ) && ( Q_stricmp( pFilename, "media/ep1_recap.bik" ) == 0 ) )
	{
		pFilename = "media/ep1_recap_25fps.bik";
	}
#endif

	// need working video services
	if ( g_pVideo == NULL )
		return false;

	// Create a new video material
	if ( m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}

	m_VideoMaterial = g_pVideo->CreateVideoMaterial( "VideoMaterial", pFilename, "GAME",
													VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
													VideoSystem::DETERMINE_FROM_FILE_EXTENSION, m_bAllowAlternateMedia );
	
	if ( m_VideoMaterial == NULL )
		return false;

	if ( m_bLooping )
	{
		m_VideoMaterial->SetLooping( true );
	}

#ifdef MAPBASE
	if ( m_bMuted )
	{
		m_VideoMaterial->SetMuted( true );
	}
#endif

	m_bStarted = true;

	// We want to be the sole audio source
	if ( m_bStopAllSounds )
	{
		enginesound->NotifyBeginMoviePlayback();
	}

	int nWidth, nHeight;
	m_VideoMaterial->GetVideoImageSize( &nWidth, &nHeight );
	m_VideoMaterial->GetVideoTexCoordRange( &m_flU, &m_flV );
	m_pMaterial = m_VideoMaterial->GetMaterial();


	float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	float flVideoRatio = ( (float) nWidth / (float) nHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::Activate( void )
{
	MoveToFront();
	RequestFocus();
	SetVisible( true );
	SetEnabled( true );

	vgui::surface()->SetMinimized( GetVPanel(), false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::DoModal( void )
{
	MakePopup();
	Activate();

	vgui::input()->SetAppModalSurface( GetVPanel() );
	vgui::surface()->RestrictPaintToSinglePanel( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	bool bInterruptKeyPressed = ( code == KEY_ESCAPE );
	if ( m_bAllowInterruption && bInterruptKeyPressed )
	{
		StopPlayback();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle keys that should cause us to close
//-----------------------------------------------------------------------------
void VideoPanel::OnKeyCodePressed( vgui::KeyCode keycode )
{
	vgui::KeyCode code = GetBaseButtonCode( keycode );

	// All these keys will interrupt playback
	bool bInterruptKeyPressed =	  ( code == KEY_ESCAPE || 
									code == KEY_BACKQUOTE || 
									code == KEY_SPACE || 
									code == KEY_ENTER ||
									code == KEY_XBUTTON_A || 
									code == KEY_XBUTTON_B ||
									code == KEY_XBUTTON_X || 
									code == KEY_XBUTTON_Y || 
									code == KEY_XBUTTON_START || 
									code == KEY_XBUTTON_BACK );
	
	// These keys cause the panel to shutdown
	if ( m_bAllowInterruption && bInterruptKeyPressed )
	{
		StopPlayback();
	}
	else
	{
		BaseClass::OnKeyCodePressed( keycode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::StopPlayback( void )
{
	SetVisible( false );

	// Start the deferred shutdown process
	m_nShutdownCount = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::OnClose( void )
{
	if ( m_bStopAllSounds )
	{
		enginesound->NotifyEndMoviePlayback();
	}

	BaseClass::OnClose();

	if ( vgui::input()->GetAppModalSurface() == GetVPanel() )
	{
		vgui::input()->ReleaseAppModalSurface();
	}

	vgui::surface()->RestrictPaintToSinglePanel( NULL );

	// Fire an exit command if we're asked to do so
	if ( m_szExitCommand[0] )
	{
		engine->ClientCmd( m_szExitCommand );
	}

	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::GetPanelPos( int &xpos, int &ypos )
{
	xpos = ( (float) ( GetWide() - m_nPlaybackWidth ) / 2 );
	ypos = ( (float) ( GetTall() - m_nPlaybackHeight ) / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void VideoPanel::Paint( void )
{
	BaseClass::Paint();

	if ( m_VideoMaterial == NULL )
		return;

	float alpha = ((float)GetFgColor()[3]/255.0f);
#ifdef MAPBASE
	if (m_flFadeIn != 0.0f || m_flFadeOut != 0.0f)
	{
		// GetCurrentVideoTime() and GetVideoDuration() are borked
		float flFrameCount = m_VideoMaterial->GetFrameCount();
		float flEnd = flFrameCount / m_VideoMaterial->GetVideoFrameRate().GetFPS();
		float flTime = ((float)(m_VideoMaterial->GetCurrentFrame()) / flFrameCount) * flEnd;
		float flFadeOutDelta = (flEnd - m_flFadeOut);

		if (flTime <= m_flFadeIn)
		{
			alpha = (flTime / m_flFadeIn);
		}
		else if (flTime >= flFadeOutDelta)
		{
			alpha = (1.0f - ((flTime - flFadeOutDelta) / m_flFadeOut));
		}
	}
#endif

	if ( m_VideoMaterial->Update() == false )
	{
		// Issue a close command
		OnVideoOver();
		//OnClose();
	}

	// Sit in the "center"
	int xpos, ypos;
	GetPanelPos( xpos, ypos );
	LocalToScreen( xpos, ypos );

	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	if ( m_bBlackBackground )
	{
		vgui::surface()->DrawSetColor(  0, 0, 0, alpha * 255.0f );
		vgui::surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	}

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext( materials );

#ifdef MAPBASE
	pRenderContext->ClearColor4ub( 255, 255, 255, alpha * 255.0f );
#endif
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( m_pMaterial, NULL );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float flLeftX = xpos;
	float flRightX = xpos + (m_nPlaybackWidth-1);

	float flTopY = ypos;
	float flBottomY = ypos + (m_nPlaybackHeight-1);

	// Map our UVs to cut out just the portion of the video we're interested in
	float flLeftU = 0.0f;
	float flTopV = 0.0f;

	// We need to subtract off a pixel to make sure we don't bleed
	float flRightU = m_flU - ( 1.0f / (float) m_nPlaybackWidth );
	float flBottomV = m_flV - ( 1.0f / (float) m_nPlaybackHeight );

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	for ( int corner=0; corner<4; corner++ )
	{
		bool bLeft = (corner==0) || (corner==3);
		meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f );
		meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
		meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
		meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
		meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
		meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
		meshBuilder.AdvanceVertex();
	}
	
	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: Create and playback a video in a panel
// Input  : nWidth - Width of panel (in pixels) 
//			nHeight - Height of panel
//			*pVideoFilename - Name of the file to play
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel_Create( unsigned int nXPos, unsigned int nYPos, 
						unsigned int nWidth, unsigned int nHeight, 
						const char *pVideoFilename, 
						const char *pExitCommand /*= NULL*/,
						const VideoPanelParms_t &parms )
{
	// Create the base video panel
	VideoPanel *pVideoPanel = new VideoPanel( nXPos, nYPos, nHeight, nWidth );
	if ( pVideoPanel == NULL )
		return false;

	// Toggle if we want the panel to allow interruption
	pVideoPanel->SetAllowInterrupt( parms.bAllowInterrupt );

	// Set the command we'll call (if any) when the video is interrupted or completes
	pVideoPanel->SetExitCommand( pExitCommand );

#ifdef MAPBASE
	// Toggle if we want the panel to loop (inspired by Portal 2)
	pVideoPanel->SetLooping( parms.bLoop );

	// Toggle if we want the panel to be muted
	pVideoPanel->SetMuted( parms.bMute );

	// TODO: Unique "Stop All Sounds" parameter
	if (parms.bMute)
	{
		pVideoPanel->SetStopAllSounds( false );
	}

	// Fade parameters (unfinished)
	//pVideoPanel->SetFade( parms.flFadeIn, parms.flFadeOut );
#endif

	// Start it going
	if ( pVideoPanel->BeginPlayback( pVideoFilename ) == false )
	{
		delete pVideoPanel;
		return false;
	}

	// Take control
	pVideoPanel->DoModal();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Create a video panel with the supplied commands
//-----------------------------------------------------------------------------
void CreateVideoPanel( const char *lpszFilename, const char *lpszExitCommand, int nWidth, int nHeight, VideoPanelParms_t &parms )
{
	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( lpszFilename, strFilename, MAX_PATH );
	Q_strncat( strFullpath, lpszFilename, MAX_PATH );

	// Use the full screen size if they haven't specified an override
	unsigned int nScreenWidth = ( nWidth != 0 ) ? nWidth : ScreenWidth();
	unsigned int nScreenHeight = ( nHeight != 0 ) ? nHeight : ScreenHeight();

	// Create the panel and go!
	if ( VideoPanel_Create( 0, 0, nScreenWidth, nScreenHeight, strFullpath, lpszExitCommand, parms ) == false )
	{
		Warning( "Unable to play video: %s\n", strFullpath );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback
//-----------------------------------------------------------------------------

CON_COMMAND( playvideo, "Plays a video: <filename> [width height]" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = Q_atoi( args[2] );
	unsigned int nScreenHeight = Q_atoi( args[3] );

	// New struct; functionally identical
	VideoPanelParms_t parms;
	
	CreateVideoPanel( args[1], NULL, nScreenWidth, nScreenHeight, parms );
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback
//-----------------------------------------------------------------------------

CON_COMMAND( playvideo_nointerrupt, "Plays a video without ability to skip: <filename> [width height]" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = Q_atoi( args[2] );
	unsigned int nScreenHeight = Q_atoi( args[3] );

	// New struct; functionally identical
	VideoPanelParms_t parms( false );

	CreateVideoPanel( args[1], NULL, nScreenWidth, nScreenHeight, parms );
}


//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback and fire a command on completion
//-----------------------------------------------------------------------------

CON_COMMAND( playvideo_exitcommand, "Plays a video and fires and exit command when it is stopped or finishes: <filename> <exit command>" )
{
	if ( args.ArgC() < 2 )
		return;

	// Pull out the exit command we want to use
	char *pExitCommand = Q_strstr( args.GetCommandString(), args[2] );

	// New struct; functionally identical
	VideoPanelParms_t parms;

	CreateVideoPanel( args[1], pExitCommand, 0, 0, parms );
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback and fire a command on completion
//-----------------------------------------------------------------------------

CON_COMMAND( playvideo_exitcommand_nointerrupt, "Plays a video (without interruption) and fires and exit command when it is stopped or finishes: <filename> <exit command>" )
{
	if ( args.ArgC() < 2 )
		return;

	// Pull out the exit command we want to use
	char *pExitCommand = Q_strstr( args.GetCommandString(), args[2] );

	// New struct; functionally identical
	VideoPanelParms_t parms( false );

	CreateVideoPanel( args[1], pExitCommand, 0, 0, parms );
}

//-----------------------------------------------------------------------------
// Purpose: Cause all playback to stop
//-----------------------------------------------------------------------------

CON_COMMAND( stopvideos, "Stops all videos playing to the screen" )
{
	FOR_EACH_VEC( g_vecVideoPanels, itr )
	{
		g_vecVideoPanels[itr]->StopPlayback();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback and fire a command on completion
//-----------------------------------------------------------------------------

CON_COMMAND( playvideo_complex, "Plays a video with various parameters to simplify logic_playmovie: <filename> <exit command> <no interrupt> <looping> <fade in> <fade out>" )
{
	if ( args.ArgC() < 2 )
		return;

	// Pull out the exit command we want to use
	char *pExitCommand = Q_strstr( args.GetCommandString(), args[2] );

	// Parameters
	VideoPanelParms_t parms;

	if (args.ArgC() >= 3)
		parms.bAllowInterrupt = atoi( args[3] ) != 0;
	if (args.ArgC() >= 4)
		parms.bLoop = atoi( args[4] ) != 0;
	if (args.ArgC() >= 5)
		parms.bMute = atoi( args[5] ) != 0;

	//if (args.ArgC() >= 5)
	//	parms.flFadeIn = atof( args[5] );
	//if (args.ArgC() >= 6)
	//	parms.flFadeOut = atof( args[6] );

	// Stop a softlock
	if (parms.bAllowInterrupt == false && parms.bLoop)
	{
		Warning( "WARNING: Tried to play video set to be uninterruptible and looping. This would cause a softlock because the video loops forever and there's no way to stop it.\n" );
		return;
	}

	CreateVideoPanel( args[1], pExitCommand, 0, 0, parms );
}
