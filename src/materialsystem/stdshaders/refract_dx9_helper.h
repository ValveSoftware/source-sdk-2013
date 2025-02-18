//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef REFRACT_DX9_HELPER_H
#define REFRACT_DX9_HELPER_H
#ifdef _WIN32
#pragma once
#endif

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
struct Refract_DX9_Vars_t
{
	Refract_DX9_Vars_t() { memset( this, 0xFF, sizeof( *this ) ); }

	int m_nBaseTexture;
	int m_nFrame;
	int m_nRefractAmount;
	int m_nRefractTint;
	int m_nNormalMap;
	int m_nNormalMap2;
	int m_nBumpFrame;
	int m_nBumpFrame2;
	int m_nBumpTransform;
	int m_nBumpTransform2;
	int m_nBlurAmount;
	int m_nFadeOutOnSilhouette;
	int m_nEnvmap;
	int m_nEnvmapFrame;
	int m_nEnvmapTint;
	int m_nEnvmapContrast;
	int m_nEnvmapSaturation;
	int m_nRefractTintTexture;
	int m_nRefractTintTextureFrame;
	int m_nFresnelReflection;
	int m_nNoWriteZ;
	int m_nMasked;
	int m_nVertexColorModulate;
	int m_nForceAlphaWrite;
};

void InitParamsRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, 
						   Refract_DX9_Vars_t &info );
void InitRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, Refract_DX9_Vars_t &info );
void DrawRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					 IShaderShadow* pShaderShadow, Refract_DX9_Vars_t &info, VertexCompressionType_t vertexCompression );

#endif // REFRACT_DX9_HELPER_H
