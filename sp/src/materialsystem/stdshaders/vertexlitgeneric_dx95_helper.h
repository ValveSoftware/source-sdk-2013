//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VERTEXLITGENERIC_DX95_HELPER_H
#define VERTEXLITGENERIC_DX95_HELPER_H

#include <string.h>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;


//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct VertexLitGeneric_DX95_Vars_t
{
	VertexLitGeneric_DX95_Vars_t() { memset( this, 0xFF, sizeof(VertexLitGeneric_DX95_Vars_t) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTexture2;
	int m_nBaseTextureFrame2;
	int m_nBaseTexture3;
	int m_nBaseTextureFrame3;
	int m_nBaseTextureTransform;
	int m_nAlbedo;
	int m_nSelfIllumTint;
	int m_nDetail;
	int m_nDetailFrame;
	int m_nDetailScale;
	int m_nEnvmap;
	int m_nEnvmapFrame;
	int m_nEnvmapMask;
	int m_nEnvmapMaskFrame;
	int m_nEnvmapMaskTransform;
	int m_nEnvmapTint;
	int m_nBumpmap;
	int m_nBumpFrame;
	int m_nBumpmap2;
	int m_nBumpFrame2;
	int m_nBumpMask;
	int m_nBumpmap3;
	int m_nBumpFrame3;
	int m_nBumpTransform;
	int m_nEnvmapContrast;
	int m_nEnvmapSaturation;
	int m_nAlphaTestReference;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
	int m_nSelfIllumEnvMapMask_Alpha;

};

void InitParamsVertexLitGeneric_DX95( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, bool bVertexLitGeneric, VertexLitGeneric_DX95_Vars_t &info );
void InitVertexLitGeneric_DX95( CBaseVSShader *pShader, IMaterialVar** params, bool bVertexLitGeneric, VertexLitGeneric_DX95_Vars_t &info );
void DrawVertexLitGeneric_DX95( CBaseVSShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool bVertexLitGeneric, VertexLitGeneric_DX95_Vars_t &info );


#endif // VERTEXLITGENERIC_DX95_HELPER_H
