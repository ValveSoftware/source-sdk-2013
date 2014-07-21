//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef VMTPANEL_H
#define VMTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "matsys_controls/potterywheelpanel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class CMeshBuilder;
class Vector;

namespace vgui
{
	class ScrollBar;
	class IScheme;
}


//-----------------------------------------------------------------------------
// Material Viewer Panel
//-----------------------------------------------------------------------------
class CVMTPanel : public CPotteryWheelPanel
{
	DECLARE_CLASS_SIMPLE( CVMTPanel, CPotteryWheelPanel );

public:
	// constructor, destructor
	CVMTPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CVMTPanel();

	// Set the material to draw
	void SetMaterial( IMaterial *pMaterial );

	// Set rendering mode (stretch to full screen, or use actual size)
	void RenderUsingActualSize( bool bEnable );

	// performs the layout
	virtual void PerformLayout();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	// paint it stretched to the window size
	void DrawStretchedToPanel( CMeshBuilder &meshBuilder );

	// paint it actual size
	void DrawActualSize( CMeshBuilder &meshBuilder );

	// Draw it on a sphere
	void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi );

	// paint it!
	virtual void OnPaint3D();

private:
	// The material to draw
	IMaterial *m_pMaterial;

	// A texture to use for a lightmap
	CTextureReference m_pLightmapTexture;

	// The default env_cubemap
	CTextureReference m_DefaultEnvCubemap;

	// Are we using actual size or not?
	bool m_bUseActualSize;

	// Scroll bars
	vgui::ScrollBar *m_pHorizontalBar;
	vgui::ScrollBar *m_pVerticalBar;

	// The viewable size
	int	m_iViewableWidth;
	int m_iViewableHeight;
};

#endif // VMTPANEL_H
	    