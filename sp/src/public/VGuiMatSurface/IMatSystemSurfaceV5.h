//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IMATSYSTEMSURFACEV5_H
#define IMATSYSTEMSURFACEV5_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui/VGUI.h>
#include "vgui/isurfacev30.h"


namespace MatSystemSurfaceV5
{
	#define MAT_SYSTEM_SURFACE_INTERFACE_VERSION_5 "MatSystemSurface005"
	
	
	class IMatSystemSurface : public SurfaceV30::ISurface
	{
	public:
		// Hook needed to get input to work.
		// If the app drives the input (like the engine needs to do for VCR mode), 
		// it can set bLetAppDriveInput to true and call HandleWindowMessage for the Windows messages.
		virtual void AttachToWindow( void *hwnd, bool bLetAppDriveInput=false ) = 0;

		// If you specified true for bLetAppDriveInput, then call this for each window message that comes in.
		virtual void HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam ) = 0;

		// Tells the surface to ignore windows messages
		virtual void EnableWindowsMessages( bool bEnable ) = 0;

		// Starts, ends 3D painting
		// NOTE: These methods should only be called from within the paint()
		// method of a panel.
		virtual void Begin3DPaint( int iLeft, int iTop, int iRight, int iBottom ) = 0;
		virtual void End3DPaint() = 0;

		// NOTE: This also should only be called from within the paint()
		// method of a panel. Use it to disable clipping for the rendering
		// of this panel.
		virtual void DisableClipping( bool bDisable ) = 0;

		// Prevents vgui from changing the cursor
		virtual bool IsCursorLocked() const = 0;

		// Sets the mouse get + set callbacks
		virtual void SetMouseCallbacks( GetMouseCallback_t getFunc, SetMouseCallback_t setFunc ) = 0;

		// Installs a function to play sounds
		virtual void InstallPlaySoundFunc( PlaySoundFunc_t soundFunc ) = 0;

		// Some drawing methods that cannot be accomplished under Win32
		virtual void DrawColoredCircle( int centerx, int centery, float radius, int r, int g, int b, int a ) = 0;
		virtual int DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, PRINTF_FORMAT_STRING char *fmt, ... ) = 0;

		// Draws text with current font at position and wordwrapped to the rect using color values specified
		virtual void DrawColoredTextRect( vgui::HFont font, int x, int y, int w, int h, int r, int g, int b, int a, PRINTF_FORMAT_STRING char *fmt, ... ) = 0;
		virtual void DrawTextHeight( vgui::HFont font, int w, int& h, PRINTF_FORMAT_STRING char *fmt, ... ) = 0;

		// Returns the length of the text string in pixels
		virtual int	DrawTextLen( vgui::HFont font, PRINTF_FORMAT_STRING char *fmt, ... ) = 0;

		// Draws a panel in 3D space. Assumes view + projection are already set up
		// Also assumes the (x,y) coordinates of the panels are defined in 640xN coords
		// (N isn't necessary 480 because the panel may not be 4x3)
		// The width + height specified are the size of the panel in world coordinates
		virtual void DrawPanelIn3DSpace( vgui::VPANEL pRootPanel, const VMatrix &panelCenterToWorld, int nPixelWidth, int nPixelHeight, float flWorldWidth, float flWorldHeight ) = 0; 

		// Binds a material to a surface texture ID
		virtual void DrawSetTextureMaterial( int id, IMaterial *pMaterial ) = 0;
	};

}

//-----------------------------------------------------------------------------
// FIXME: This works around using scoped interfaces w/ EXPOSE_SINGLE_INTERFACE
//-----------------------------------------------------------------------------
class IMatSystemSurfaceV5 : public MatSystemSurfaceV5::IMatSystemSurface
{
public:
};

#endif // IMATSYSTEMSURFACEV5_H
