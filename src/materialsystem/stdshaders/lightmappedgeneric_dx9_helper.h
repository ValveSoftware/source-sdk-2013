//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LIGHTMAPPEDGENERIC_DX9_HELPER_H
#define LIGHTMAPPEDGENERIC_DX9_HELPER_H

#include <string.h>
#include "BaseVSShader.h"


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
struct LightmappedGeneric_DX9_Vars_t
{
	LightmappedGeneric_DX9_Vars_t() { memset( this, 0xFF, sizeof(LightmappedGeneric_DX9_Vars_t) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlbedo;
	int m_nSelfIllumTint;

	int m_nAlpha2; // Hack for DoD srgb blend issues on overlays

	int m_nDetail;
	int m_nDetailFrame;
	int m_nDetailScale;
	int m_nDetailTextureCombineMode;
	int m_nDetailTextureBlendFactor;
	int m_nDetailTint;

	int m_nEnvmap;
	int m_nEnvmapFrame;
	int m_nEnvmapMask;
	int m_nEnvmapMaskFrame;
	int m_nEnvmapMaskTransform;
	int m_nEnvmapTint;
	int m_nBumpmap;
	int m_nBumpFrame;
	int m_nBumpTransform;
	int m_nEnvmapContrast;
	int m_nEnvmapSaturation;
	int m_nFresnelReflection;
	int m_nNoDiffuseBumpLighting;
	int m_nBumpmap2;
	int m_nBumpFrame2;
	int m_nBumpTransform2;
	int m_nBumpMask;
	int m_nBaseTexture2;
	int m_nBaseTexture2Frame;
	int m_nBaseTextureNoEnvmap;
	int m_nBaseTexture2NoEnvmap;
	int m_nDetailAlphaMaskBaseTexture;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
	int m_nLightWarpTexture;
	int m_nBlendModulateTexture;
	int m_nMaskedBlending;
	int m_nBlendMaskTransform;
	int m_nSelfShadowedBumpFlag;
	int m_nSeamlessMappingScale;
	int m_nAlphaTestReference;

	int m_nSoftEdges;
	int m_nEdgeSoftnessStart;
	int m_nEdgeSoftnessEnd;

	int m_nOutline;
	int m_nOutlineColor;
	int m_nOutlineAlpha;
	int m_nOutlineStart0;
	int m_nOutlineStart1;
	int m_nOutlineEnd0;
	int m_nOutlineEnd1;

};

void InitParamsLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, LightmappedGeneric_DX9_Vars_t &info );
void InitLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, LightmappedGeneric_DX9_Vars_t &info );
void DrawLightmappedGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
								 IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, 
								 LightmappedGeneric_DX9_Vars_t &info, CBasePerMaterialContextData **pContextDataPtr	 );


#endif // LIGHTMAPPEDGENERIC_DX9_HELPER_H
