//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowsermovieplayerpanel.h"
#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "engine/IEngineSound.h"
#include "iclientmode.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

CMoviePlayerPanel::CMoviePlayerPanel( Panel *pParent, const char *pName, const char *pMovieFilename )
:	CReplayBasePanel( pParent, pName ),
	m_flCurFrame( 0.0f ),
	m_flLastTime( 0.0f ),
	m_nLastMouseXPos( 0 ),
	m_bPlaying( false ),
	m_bLooping( false ),
	m_bFullscreen( false ),
	m_bMouseOverScrub( false ),
	m_pOldParent( NULL ),
	m_pVideoMaterial( NULL )
{
	if ( g_pVideo )
	{
		m_pVideoMaterial = g_pVideo->CreateVideoMaterial( pMovieFilename, pMovieFilename, "GAME" );
		if ( m_pVideoMaterial )
		{
			m_pMaterial = m_pVideoMaterial->GetMaterial();
			m_pMaterial->AddRef();
			m_nNumFrames = m_pVideoMaterial->GetFrameCount();
		
		}
	}

	ivgui()->AddTickSignal( GetVPanel(), 0 );
}

CMoviePlayerPanel::~CMoviePlayerPanel()
{
	FreeMaterial();

	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CMoviePlayerPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	GetPosRelativeToAncestor( NULL, m_nGlobalPos[0], m_nGlobalPos[1] );

	if ( m_bFullscreen )
	{
		// Cache parent
		m_pOldParent = GetParent();
		GetBounds( m_aOldBounds[0], m_aOldBounds[1], m_aOldBounds[2], m_aOldBounds[3] );

		// Adjust parent for fullscreen mode
		SetParent( g_pClientMode->GetViewport() );
		
		// Adjust bounds for fullscreen
		SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
	}
	else if ( m_pOldParent )
	{
		// Restore old parent/bounds
		SetParent( m_pOldParent );
		SetBounds( m_aOldBounds[0], m_aOldBounds[1], m_aOldBounds[2], m_aOldBounds[3] );
	}
}

void CMoviePlayerPanel::OnMousePressed( MouseCode code )
{
//	ToggleFullscreen();
}

void CMoviePlayerPanel::SetScrubOnMouseOverMode( bool bOn )
{
	if ( bOn )
	{
		m_bPlaying = false;
	}

	m_bMouseOverScrub = bOn;
}

void CMoviePlayerPanel::Play()
{
	m_bPlaying = true;
	m_flLastTime = gpGlobals->realtime;

	enginesound->NotifyBeginMoviePlayback();
}

void CMoviePlayerPanel::FreeMaterial()
{
	if ( m_pVideoMaterial )
	{
		if ( g_pVideo )
		{
			g_pVideo->DestroyVideoMaterial( m_pVideoMaterial );
		}

		m_pVideoMaterial = NULL;
	}

	if ( m_pMaterial )
	{
		m_pMaterial->Release();
		m_pMaterial = NULL;
	}
}

void CMoviePlayerPanel::OnTick()
{
	if ( !IsEnabled() )
		return;

	if ( m_bMouseOverScrub )
	{
		int nMouseX, nMouseY;
		input()->GetCursorPos( nMouseX, nMouseY );
		if ( IsWithin( nMouseX, nMouseY ) &&
			 nMouseX != m_nLastMouseXPos )
		{
			float flPercent = (float)( nMouseX - m_nGlobalPos[0] ) / GetWide();
			m_flCurFrame = flPercent * ( m_nNumFrames - 1 );
			m_nLastMouseXPos = nMouseX;
		}
	}
	else if ( m_bPlaying )
	{
		float flElapsed = gpGlobals->realtime - m_flLastTime;
		m_flLastTime = gpGlobals->realtime;

		m_flCurFrame += flElapsed * m_pVideoMaterial->GetVideoFrameRate().GetFPS();
		// Loop if necessary
		if ( m_flCurFrame >= m_nNumFrames )
		{
			if ( m_bLooping )
			{
				m_flCurFrame = m_flCurFrame - m_nNumFrames;
			}
			else
			{
				// Don't go past last frame
				m_flCurFrame = m_nNumFrames - 1;
			}
		}
	}

	m_pVideoMaterial->SetFrame( m_flCurFrame );

//	Msg( "frame: %f / %i\n", m_flCurFrame, m_nNumFrames );
}

void CMoviePlayerPanel::Paint()
{
	if ( m_pVideoMaterial == NULL )
		return;

	// Get panel position/dimensions
	int x,y;
	int w,h;
	GetPosRelativeToAncestor( NULL, x, y );
	GetSize( w,h );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pMaterial );

	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	float flMinU = 0.0f, flMinV = 0.0f;
	float flMaxU, flMaxV;
	m_pVideoMaterial->GetVideoTexCoordRange( &flMaxU, &flMaxV );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flMinU, flMinV );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + w, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flMaxU, flMinV );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + w, y + h, 0.0f );
	meshBuilder.TexCoord2f( 0, flMaxU, flMaxV );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + h, 0.0f );
	meshBuilder.TexCoord2f( 0, flMinU, flMaxV );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CMoviePlayerPanel::ToggleFullscreen()
{
	m_bFullscreen = !m_bFullscreen;

	InvalidateLayout( false, false );
}

#endif