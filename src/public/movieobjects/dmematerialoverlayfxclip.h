//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMEMATERIALOVERLAYFXCLIP_H
#define DMEMATERIALOVERLAYFXCLIP_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeclip.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "datamodel/dmelementfactoryhelper.h"


//-----------------------------------------------------------------------------
// An effect clip
//-----------------------------------------------------------------------------
class CDmeMaterialOverlayFXClip : public CDmeFXClip
{
	DEFINE_ELEMENT( CDmeMaterialOverlayFXClip, CDmeFXClip );

public:
	// All effects must be able to apply their effect
	virtual void ApplyEffect( DmeTime_t time, Rect_t &currentRect, Rect_t &totalRect, ITexture *pTextures[MAX_FX_INPUT_TEXTURES] );

	// Resolves changes
	virtual void Resolve();

	// Sets the overlay material
	void SetOverlayEffect( const char *pMaterialName );
	void SetAlpha( float flAlpha );
	bool HasOpaqueOverlay();

	IMaterial *GetMaterial();
	float      GetAlpha();

private:
	CDmaString		m_Material;
	CDmaColor		m_Color;
	CDmaVar<int>	m_nLeft;
	CDmaVar<int>	m_nTop;
	CDmaVar<int>	m_nWidth;
	CDmaVar<int>	m_nHeight;
	CDmaVar<bool>	m_bFullScreen;
	CDmaVar<bool>	m_bUseSubRect;
	CDmaVar<int>	m_nSubRectLeft;
	CDmaVar<int>	m_nSubRectTop;
	CDmaVar<int>	m_nSubRectWidth;
	CDmaVar<int>	m_nSubRectHeight;
	CDmaVar<float>	m_flMovementAngle;
	CDmaVar<float>	m_flMovementSpeed;
	CMaterialReference		m_OverlayMaterial;
};


#endif // DMEMATERIALOVERLAYFXCLIP_H
