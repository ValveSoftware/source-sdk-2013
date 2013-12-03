//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VMTPREVIEWPANEL_H
#define VMTPREVIEWPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "vgui_controls/Panel.h"
#include "tier1/utlstring.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "mathlib/vector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// VMT Preview panel
//
//-----------------------------------------------------------------------------
class CVMTPreviewPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CVMTPreviewPanel, vgui::Panel );

public:
	// constructor
	CVMTPreviewPanel( vgui::Panel *pParent, const char *pName );
	void SetVMT( const char *pMaterialName );
	const char *GetVMT() const;

	// Paints the texture
	virtual void Paint( void );

	// View it in 3D or 2D mode
	void DrawIn3DMode( bool b3DMode );

private:
	// Two different preview methods
	void DrawSphere( void );
	void DrawRectangle( void );

	// Set up a projection matrix for a 90 degree fov
	void SetupProjectionMatrix( int nWidth, int nHeight );
	void SetupOrthoMatrix( int nWidth, int nHeight );

	// Sets the camera to look at the the thing we're spinning around
	void LookAt( const Vector &vecLookAt, float flRadius );

	// Sets up lighting state
	void SetupLightingState();

	// Draw a sphere
	void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi );

	// Draw sprite-card based materials
	void RenderSpriteCard( const Vector &vCenter, float flRadius );

	CUtlString m_VMTName;
	CMaterialReference m_Material;
	CTextureReference m_pLightmapTexture;
	CTextureReference m_DefaultEnvCubemap;
	Vector m_LightDirection;
	Color m_LightColor;
	float m_flLightIntensity;
	Vector m_vecCameraDirection;
	float m_flLastRotationTime;
	bool m_bDrawIn3DMode;
};


#endif // VMTPREVIEWPANEL_H