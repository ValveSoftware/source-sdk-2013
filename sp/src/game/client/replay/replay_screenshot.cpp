//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "replay/iclientreplaycontext.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay_screenshot.h"
#include "view.h"
#include "filesystem.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "fasttimer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

extern IClientReplayContext *g_pClientReplayContext;
ITexture *CReplayScreenshotTaker::m_pScreenshotTarget;


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CReplayScreenshotTaker::CReplayScreenshotTaker( IViewRender *pViewRender, CViewSetup &view )
:	m_pViewRender( pViewRender ),
	m_View( view )
{
	m_pUnpaddedPixels = NULL;
	m_pPaddedPixels = NULL;
	m_pVTFPixels = NULL;

	m_pVTFTexture = NULL;

	m_pBuffer = NULL;

	if ( !m_pScreenshotTarget )
		return;

	m_aPaddedDims[ 0 ] = m_pScreenshotTarget->GetActualWidth();
	m_aPaddedDims[ 1 ] = m_pScreenshotTarget->GetActualHeight();

	g_pClientReplayContext->GetScreenshotManager()->GetUnpaddedScreenshotSize( m_aUnpaddedDims[ 0 ], m_aUnpaddedDims[ 1 ] );

	// Calculate sizes
	int nUnpaddedSize = 3 * m_aUnpaddedDims[ 0 ] * m_aUnpaddedDims[ 1 ];
	int nPaddedSize = 3 * m_aPaddedDims[ 0 ] * m_aPaddedDims[ 1 ];
	
	// Allocate for padded & unpadded pixel data
	m_pUnpaddedPixels = new uint8[ nUnpaddedSize ];
	m_pPaddedPixels = new uint8[ nPaddedSize ];

	// White out the entire padded image
	V_memset( m_pPaddedPixels, 255, nPaddedSize );

	// Create the VTF
#ifndef _X360
	IVTFTexture *pVTFTexture = CreateVTFTexture();
	const int nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_SRGB;
	if ( !pVTFTexture->Init( m_aPaddedDims[ 0 ], m_aPaddedDims[ 1 ], 1, IMAGE_FORMAT_RGB888, nFlags, 1, 1 ) )
		return;

	m_pVTFTexture = pVTFTexture;
#else
	m_pVTFTexture = NULL;
#endif // _X360

	// Allocate pixels for the output buffer
	int nVTFSize = 1024 + ( 3 * m_aPaddedDims[ 0 ] * m_aPaddedDims[ 1 ] );
	m_pVTFPixels = new uint8[ nVTFSize ];
	m_pBuffer = new CUtlBuffer( m_pVTFPixels, nVTFSize );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CReplayScreenshotTaker::~CReplayScreenshotTaker()
{
	delete [] m_pUnpaddedPixels;
	delete [] m_pPaddedPixels;
	delete [] m_pVTFPixels;

#ifndef _X360
	DestroyVTFTexture( m_pVTFTexture );
#endif // _X360

	delete m_pBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot for the replay system
//-----------------------------------------------------------------------------
void CReplayScreenshotTaker::TakeScreenshot( WriteReplayScreenshotParams_t &params )
{
	if ( !m_pViewRender )
		return;

	CFastTimer timer;
	ConVarRef replay_debug( "replay_debug" );
	bool bDbg = replay_debug.IsValid() && replay_debug.GetBool();

	int width = params.m_nWidth;
	int height = params.m_nHeight;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();

	extern bool g_bRenderingScreenshot;
	g_bRenderingScreenshot = true;

	// Push back buffer on the stack with small viewport
	pRenderContext->PushRenderTargetAndViewport( m_pScreenshotTarget, 0, 0, width, height );

	// setup the view to render
	CViewSetup viewSetup = m_View;
	viewSetup.x = 0;
	viewSetup.y = 0;
	viewSetup.width = width;
	viewSetup.height = height;
	viewSetup.fov = ScaleFOVByWidthRatio( m_View.fov, ( (float)width / (float)height ) / ( 4.0f / 3.0f ) );
	viewSetup.m_bRenderToSubrectOfLargerScreen = true;

	// Setup view origin/angles
	if ( params.m_pOrigin )
	{
		viewSetup.origin = *params.m_pOrigin;
	}
	if ( params.m_pAngles )
	{
		viewSetup.angles = *params.m_pAngles;
	}

	timer.Start();
	
		// draw out the scene - don't draw the HUD or the viewmodel
		m_pViewRender->RenderView( viewSetup, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, 0 );

	timer.End();
	if ( bDbg ) Warning( "Screenshot RenderView(): %.4f s\n", timer.GetDuration().GetSeconds() );

	timer.Start();

		// Get Bits from the material system
		pRenderContext->ReadPixels( 0, 0, width, height, m_pUnpaddedPixels, IMAGE_FORMAT_RGB888 );

	timer.End();
	if ( bDbg ) Warning( "Screenshot ReadPixels(): %.4f s\n", timer.GetDuration().GetSeconds() );

	// Some stuff to be setup dependent on padded vs. not padded
	int nSrcWidth, nSrcHeight;
	unsigned char *pSrcImage;

	// Setup dimensions as needed
	int nPaddedWidth = m_aPaddedDims[0];
	int nPaddedHeight = m_aPaddedDims[1];

	// Allocate
	unsigned char *pUnpaddedImage = m_pUnpaddedPixels;
	unsigned char *pPaddedImage = m_pPaddedPixels;
	
	timer.Start();
		// Copy over each row individually
		for ( int nRow = 0; nRow < height; ++nRow )
		{
			unsigned char *pDst = pPaddedImage + 3 * ( nRow * nPaddedWidth );
			const unsigned char *pSrc = pUnpaddedImage + 3 * ( nRow * width );
			V_memcpy( pDst, pSrc, 3 * width );
		}
	timer.End();
	if ( bDbg ) Warning( "Screenshot copy image: %.4f s\n", timer.GetDuration().GetSeconds() );

	// Setup source data
	nSrcWidth = nPaddedWidth;
	nSrcHeight = nPaddedHeight;
	pSrcImage = pPaddedImage;

	if ( !m_pVTFTexture )
		return;

	// Copy the image data over to the VTF
	unsigned char *pDestBits = m_pVTFTexture->ImageData();
	int nDstSize = nSrcWidth * nSrcHeight * 3;
	V_memcpy( pDestBits, pSrcImage, nDstSize );

	bool bWriteResult = true;

	// Reset put
	m_pBuffer->SeekPut( CUtlBuffer::SEEK_HEAD, 0 );

	timer.Start();
		// Serialize to the buffer
		bWriteResult = m_pVTFTexture->Serialize( *m_pBuffer );
	timer.End();
	if ( bDbg ) Warning( "Screenshot VTF->Serialize(): %.4f s\n", timer.GetDuration().GetSeconds() );
	
	if ( !bWriteResult )
	{
		Warning( "Couldn't write Replay screenshot.\n" );
		bWriteResult = false;

		return;
	}

	// async write to disk (this will take ownership of the memory)
	char szPathedFileName[_MAX_PATH];
	Q_snprintf( szPathedFileName, sizeof(szPathedFileName), "//MOD/%s", params.m_pFilename );

	timer.Start();
		filesystem->AsyncWrite( szPathedFileName, m_pBuffer->Base(), m_pBuffer->TellPut(), false );
	timer.End();
	if ( bDbg ) Warning( "Screenshot AsyncWrite(): %.4f s\n", timer.GetDuration().GetSeconds() );

	// restore our previous state
	pRenderContext->PopRenderTargetAndViewport();
	
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	g_bRenderingScreenshot = false;
}


void CReplayScreenshotTaker::CreateRenderTarget( IMaterialSystem *pMaterialSystem )
{
	m_pScreenshotTarget = pMaterialSystem->CreateNamedRenderTargetTextureEx2( "rt_ReplayScreenshot", 0, 0, RT_SIZE_REPLAY_SCREENSHOT, IMAGE_FORMAT_RGB888, MATERIAL_RT_DEPTH_SEPARATE );
	m_pScreenshotTarget->AddRef();  // we will leak this ref, but only at shutdown of the app, which will be cleaned up then
}


#endif