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
#include "vgui_video_player.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


VideoPlayerPanel::VideoPlayerPanel( vgui::Panel *parent, const char *panelName, int nXpos, int nYpos, int nWidth, int nHeight, const char *pVideoFile ) : 
	BaseClass( parent, panelName ),
	m_VideoMaterial( NULL ),
	m_VideoFileName( NULL ),
	m_VideoLoaded( false ),
	m_VideoPlaying( false )
{
	Assert( g_pVideo != NULL );

	// init all the video realted member vars
	ClearVideo();

	SetVisible( false );
	
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	SetProportional( false );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	
	// Set us up
	SetTall( nHeight );
	SetWide( nWidth );
	SetPos( nXpos, nYpos );

	// use defaults for scheme and control settings for now

	// SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/VideoPlayerPanelScheme.res", "VideoPlayerPanelScheme"));
	//LoadControlSettings("resource/UI/VideoPlayerPanel.res");

	// Assign video file if supplied
	SetVideo( pVideoFile );
	
	SetVisible( true );

}									

									
									

//-----------------------------------------------------------------------------
// Properly shutdown out materials
//-----------------------------------------------------------------------------
VideoPlayerPanel::~VideoPlayerPanel( void )
{
	SetParent( (vgui::Panel *) NULL );

	StopVideo();
	ClearVideo();
	
}


bool VideoPlayerPanel::SetVideo( const char *pVideoFile )
{
	ClearVideo();

	// clearing the video?
	if ( pVideoFile == NULL || pVideoFile[0] == 0x00 )
	{
		return true;
	}

	// create the material
	m_VideoMaterial = g_pVideo->CreateVideoMaterial( "VideoPlayerMaterial", pVideoFile, "GAME",
													VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
													VideoSystem::DETERMINE_FROM_FILE_EXTENSION, true );
	
	if ( m_VideoMaterial == NULL )
	{
		return false;
	}

	// save filename	
	int sLen = V_strlen( pVideoFile ) + 1;
	m_VideoFileName = new char[ sLen ];
	V_strncpy( m_VideoFileName, pVideoFile, sLen );
	
	// compute Playback dimensions
	
	int nWidth, nHeight;
	m_VideoMaterial->GetVideoImageSize( &nWidth, &nHeight );
	m_VideoMaterial->GetVideoTexCoordRange( &m_flU, &m_flV );

	float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	float flVideoRatio = ( (float) nWidth / (float) nHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
		m_letterBox = 1;
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
		m_letterBox = 2;
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
		m_letterBox = 0;
	}
	
	m_pMaterial = m_VideoMaterial->GetMaterial();
	
	m_VideoDuration = m_VideoMaterial->GetVideoDuration();
	
	m_VideoLoaded = true;
	return true;
}



void VideoPlayerPanel::ClearVideo()
{
	if ( m_VideoPlaying )
	{
		StopVideo();
	}

	// Shut down this video, destroy the video material
	if ( g_pVideo != NULL && m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}
	
	if ( m_VideoFileName != NULL )
	{
		delete[] m_VideoFileName;
		m_VideoFileName = NULL;
	}

	m_pMaterial = NULL;
	
	m_VideoLoaded = false;
	m_VideoPlaying = false;
	m_VideoPaused = false;
	m_nPlaybackHeight = 0;
	m_nPlaybackWidth = 0;
	m_letterBox = 0;
	
	m_flU = 0.0f;
	m_flV = 0.0f;
	
	m_VideoDuration = 0.0f;
	
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPlayerPanel::Activate( void )
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
void VideoPlayerPanel::OnClose( void )
{
	StopVideo();

//	enginesound->NotifyEndMoviePlayback();

	vgui::surface()->RestrictPaintToSinglePanel( NULL );

	SetVisible( false );
	MarkForDeletion();
}


//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void VideoPlayerPanel::Paint( void )
{
	BaseClass::Paint();

	// Get our dimensions
	int xpos = 0;
	int ypos = 0;
	vgui::ipanel()->GetAbsPos( GetVPanel(), xpos, ypos );
//	GetPanelPos( xpos, ypos );
	int width = GetWide();
	int height = GetTall();

	
	// Are we playing the video?  Do we even have a video?
	if ( !m_VideoLoaded || !m_VideoPlaying )
	{
		vgui::surface()->DrawSetColor(  0, 0, 0, 255 );
		vgui::surface()->DrawFilledRect( 0, 0, width, height );
		return;		
	}
	
	if ( m_VideoMaterial == NULL )
		return;

	if ( m_VideoMaterial->Update() == false )
	{
		StopVideo();
		return;
	}

	// Black out the letterbox ares if we have them
	if ( m_letterBox != 0 )
	{
		vgui::surface()->DrawSetColor(  0, 0, 0, 255 );
		
		if ( m_letterBox == 1 )		// bars on top, bottom
		{
			int excess = ( height - m_nPlaybackHeight );
			int top = excess /2;
			int bot = excess - top;
		
			vgui::surface()->DrawFilledRect( 0, 0, width, top );
			vgui::surface()->DrawFilledRect( 0, height - bot, width, height );
		}
		
		if ( m_letterBox == 2 )		// bars on left, right
		{
			int excess = ( width - m_nPlaybackWidth );
			int left = excess /2;
			int right = excess - left;
		
			vgui::surface()->DrawFilledRect( 0, 0, left, height );
			vgui::surface()->DrawFilledRect( width-right, 0, width, height );
		}
	}

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext( materials );
	
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
	float flRightX = xpos + ( m_nPlaybackWidth-1 );

	float flTopY = ypos;
	float flBottomY = ypos + ( m_nPlaybackHeight-1 );

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

	float alpha = ((float)GetFgColor()[3]/255.0f);

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

bool VideoPlayerPanel::StartVideo()
{
	if ( !m_VideoLoaded )
		return false;

	// already playing?		
	if ( m_VideoPlaying )
	{
		// paused?
		if ( m_VideoPaused )
		{
			return UnpauseVideo();
		}
		return true;	
	}

	m_VideoPlaying = m_VideoMaterial->StartVideo();

	return m_VideoPlaying;

}

bool VideoPlayerPanel::StopVideo()
{
	if ( !m_VideoLoaded || !m_VideoPlaying )
		return false;

	m_VideoMaterial->StopVideo();
	m_VideoPlaying = false;
	
	return true;	
}

bool VideoPlayerPanel::PauseVideo()
{
	if ( !m_VideoLoaded || !m_VideoPlaying )
		return false;

	if ( !m_VideoPaused )
	{
		m_VideoMaterial->SetPaused( true );
		m_VideoPaused = true;
	}
	
	return true;
}


bool VideoPlayerPanel::UnpauseVideo()
{
	if ( !m_VideoLoaded || !m_VideoPlaying )
		return false;

	if ( m_VideoPaused )
	{
		m_VideoMaterial->SetPaused( false );
		m_VideoPaused = false;
	}
	
	return true;
}

float VideoPlayerPanel::GetCurrentPlaybackTime()
{
	if ( !m_VideoLoaded )
	{
		return 0.0f;
	}
	
	return m_VideoMaterial->GetCurrentVideoTime();
}



bool VideoPlayerPanel::SetCurrentPlaybackTime( float newTime )
{
	if ( !m_VideoLoaded )
		return false;
		
	if ( newTime < 0.0f || newTime > m_VideoDuration )
		return false;
		
	return m_VideoMaterial->SetTime( newTime );
}


bool VideoPlayerPanel::HasAudio()
{
	if ( !m_VideoLoaded )
		return false;

	return m_VideoMaterial->HasAudio();
}

bool VideoPlayerPanel::IsMuted()
{
	if ( !m_VideoLoaded )
		return false;
	
	return m_VideoMaterial->IsMuted();
		
}


bool VideoPlayerPanel::SetMute( bool muted )
{
	if ( !m_VideoLoaded )
		return false;

	m_VideoMaterial->SetMuted( muted );
	return true;
}

float VideoPlayerPanel::GetVolume()
{
	if ( !m_VideoLoaded )
		return 0.0f;

	return m_VideoMaterial->GetVolume();
}


bool VideoPlayerPanel::SetVolume( float newVolume )
{
	if ( !m_VideoLoaded )
		return false;

	return m_VideoMaterial->SetVolume( newVolume );
}
