//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef EYES_DX8_DX9_HELPER_H
#define EYES_DX8_DX9_HELPER_H
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
struct Eyes_DX8_DX9_Vars_t
{
	Eyes_DX8_DX9_Vars_t() { memset( this, 0xFF, sizeof(Eyes_DX8_DX9_Vars_t) ); }

	int m_nBaseTexture;
	int m_nFrame;
	int m_nIris;
	int m_nIrisFrame;
	int m_nGlint;
	int m_nEyeOrigin;
	int m_nEyeUp;
	int m_nIrisU;
	int m_nIrisV;
	int m_nGlintU;
	int m_nGlintV;
	int m_nDilation;
	int m_nIntro;
	int m_nEntityOrigin;
	int m_nWarpParam;
};

void InitParamsEyes_DX8_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, Eyes_DX8_DX9_Vars_t &info );
void InitEyes_DX8_DX9( CBaseVSShader *pShader, IMaterialVar** params, Eyes_DX8_DX9_Vars_t &info );
void DrawEyes_DX8_DX9( bool bDX9, CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					  IShaderShadow* pShaderShadow, Eyes_DX8_DX9_Vars_t &info, VertexCompressionType_t vertexCompression );

#endif // EYES_DX8_DX9_HELPER_H
