//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#if !defined( REPLAY_SCREENSHOT_H )
#define REPLAY_SCREENSHOT_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "replay/screenshot.h"

//-----------------------------------------------------------------------------

class IViewRender;

//-----------------------------------------------------------------------------

//
// Takes screenshots as VTF's for the replay system - allocates enough
// memory up-front as opposed to every frame.  Should be destroyed and recreated
// any time the cvar "replay_screenshotresolution" changes OR the actual screen
// resolution.
//
class CReplayScreenshotTaker
{
public:
	CReplayScreenshotTaker( IViewRender *pViewRender, CViewSetup &view );
	~CReplayScreenshotTaker();

	void		TakeScreenshot( WriteReplayScreenshotParams_t &params );

	static void CreateRenderTarget( IMaterialSystem *pMaterialSystem );

private:
	IViewRender	*m_pViewRender;
	CViewSetup	&m_View;
	uint8		*m_pUnpaddedPixels;
	uint8		*m_pPaddedPixels;
	IVTFTexture *m_pVTFTexture;
	uint8		*m_pVTFPixels;

	int			m_aUnpaddedDims[2];		// Width & height of m_pUnpaddedPixels
	int			m_aPaddedDims[2];		// Width & height of m_pPaddedPixels

	CUtlBuffer	*m_pBuffer;


	static ITexture		*m_pScreenshotTarget;
};

#endif // REPLAY_SCREENSHOT_H
