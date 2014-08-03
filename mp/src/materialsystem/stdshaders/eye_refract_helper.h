//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef EYE_REFRACT_HELPER_H
#define EYE_REFRACT_HELPER_H
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
struct Eye_Refract_Vars_t
{
	Eye_Refract_Vars_t() { memset( this, 0xFF, sizeof(Eye_Refract_Vars_t) ); }

	int m_nFrame;
	int m_nIris;
	int m_nIrisFrame;
	int m_nEyeOrigin;
	int m_nIrisU;
	int m_nIrisV;
	int m_nDilation;
	int m_nGlossiness;
	int m_nIntro;
	int m_nEntityOrigin; // Needed for intro
	int m_nWarpParam;
	int m_nCorneaTexture;
	int m_nAmbientOcclTexture;
	int m_nEnvmap;
	int m_nSphereTexKillCombo;
	int m_nRaytraceSphere;
	int m_nParallaxStrength;
	int m_nCorneaBumpStrength;
	int m_nAmbientOcclColor;
	int m_nEyeballRadius;
	int m_nDiffuseWarpTexture;
};

// Default values (Arrays should only be vec[4])
static const int kDefaultIntro = 0;
static const float kDefaultEyeOrigin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const float kDefaultIrisU[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const float kDefaultIrisV[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const float kDefaultDilation = 0.5f;
static const float kDefaultGlossiness = 1.0f;
static const float kDefaultWarpParam = 0.0f;
static const int kDefaultSphereTexKillCombo = 0;
static const int kDefaultRaytraceSphere = 0;
static const float kDefaultParallaxStrength = 0.25f;
static const float kDefaultCorneaBumpStrength = 1.0f;
static const float kDefaultAmbientOcclColor[4] = { 0.33f, 0.33f, 0.33f, 0.0f };
static const float kDefaultEyeballRadius = 0.5f;

void InitParams_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, Eye_Refract_Vars_t &info );
void Init_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, Eye_Refract_Vars_t &info );
void Draw_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					    IShaderShadow* pShaderShadow, Eye_Refract_Vars_t &info, VertexCompressionType_t vertexCompression );

#endif // EYES_DX8_DX9_HELPER_H
