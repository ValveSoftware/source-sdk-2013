//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VIEW_SHAREDV1_H
#define VIEW_SHAREDV1_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Renderer setup data.  
//-----------------------------------------------------------------------------
class CViewSetupV1
{
public:
	CViewSetupV1()
	{
		m_bForceAspectRatio1To1 = false;
		m_bRenderToSubrectOfLargerScreen = false;
		bForceClearWholeRenderTarget = false;
		m_bUseRenderTargetAspectRatio = false;
	}

// shared by 2D & 3D views

	// User specified context
	int			context;			

	// left side of view window
	int			x;					
	// top side of view window
	int			y;					
	// width of view window
	int			width;				
	// height of view window
	int			height;				

	// clear the color buffer before rendering this view?
	bool		clearColor;			
	// clear the Depth buffer before rendering this view?
	bool		clearDepth;			
	// NOTE: This is for a workaround on ATI with building cubemaps.  Clearing just the viewport doesn't seem to work properly.
	bool		bForceClearWholeRenderTarget;

// the rest are only used by 3D views

	// Orthographic projection?
	bool		m_bOrtho;			
	// View-space rectangle for ortho projection.
	float		m_OrthoLeft;		
	float		m_OrthoTop;
	float		m_OrthoRight;
	float		m_OrthoBottom;

	// horizontal FOV in degrees
	float		fov;				
	// horizontal FOV in degrees for in-view model
	float		fovViewmodel;		

	// 3D origin of camera
	Vector		origin;					
	// Origin gets reflected on the water surface, but things like
	// displacement LOD need to be calculated from the viewer's 
	// real position.		
	Vector		m_vUnreflectedOrigin;																			
	
	// heading of camera (pitch, yaw, roll)
	QAngle		angles;				
	// local Z coordinate of near plane of camera
	float		zNear;			
		// local Z coordinate of far plane of camera
	float		zFar;			

	// local Z coordinate of near plane of camera ( when rendering view model )
	float		zNearViewmodel;		
	// local Z coordinate of far plane of camera ( when rendering view model )
	float		zFarViewmodel;		

	bool		m_bForceAspectRatio1To1;

	// set to true if this is to draw into a subrect of the larger screen
	// this really is a hack, but no more than the rest of the way this class is used
	bool		m_bRenderToSubrectOfLargerScreen;

	// Use this for situations like water where you want to render the aspect ratio of the
	// back buffer into a square (or otherwise) render target.
	bool		m_bUseRenderTargetAspectRatio;
};

#endif // VIEW_SHAREDV1_H
