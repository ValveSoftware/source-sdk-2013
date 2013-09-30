//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

#ifndef FLESH_INTERIOR_BLENDED_PASS_HELPER_H
#define FLESH_INTERIOR_BLENDED_PASS_HELPER_H
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
struct FleshInteriorBlendedPassVars_t
{
	FleshInteriorBlendedPassVars_t() { memset( this, 0xFF, sizeof(FleshInteriorBlendedPassVars_t) ); }

	int m_nFleshTexture;
	int m_nFleshNoiseTexture;
	int m_nFleshBorderTexture1D;
	int m_nFleshNormalTexture;
	int m_nFleshSubsurfaceTexture;
	int m_nFleshCubeTexture;

	int m_nflBorderNoiseScale;
	int m_nflDebugForceFleshOn;
	int m_nvEffectCenterRadius1;
	int m_nvEffectCenterRadius2;
	int m_nvEffectCenterRadius3;
	int m_nvEffectCenterRadius4;

	int m_ncSubsurfaceTint;
	int m_nflBorderWidth;
	int m_nflBorderSoftness; // > 0.0f && < 0.5f !
	int m_ncBorderTint;
	int m_nflGlobalOpacity;
	int m_nflGlossBrightness;
	int m_nflScrollSpeed;

	int m_nTime;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultBorderNoiseScale = 1.5f;
static const float kDefaultDebugForceFleshOn = 0.0f;
static const float kDefaultEffectCenterRadius[4] = { 0.0f, 0.0f, 0.0f, 0.0001f }; // Disabled by default
static const float kDefaultSubsurfaceTint[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Disabled by default
static const float kDefaultBorderWidth = 0.3f;
static const float kDefaultBorderSoftness = 0.42f; // > 0.0f && < 0.5f !
static const float kDefaultBorderTint[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
static const float kDefaultGlobalOpacity = 1.0f;
static const float kDefaultGlossBrightness = 0.66f;
static const float kDefaultScrollSpeed = 1.0f;

void InitParamsFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FleshInteriorBlendedPassVars_t &info );
void InitFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, FleshInteriorBlendedPassVars_t &info );
void DrawFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								   IShaderShadow* pShaderShadow, FleshInteriorBlendedPassVars_t &info, VertexCompressionType_t vertexCompression );

#endif // FLESH_INTERIOR_BLENDED_PASS_HELPER_H
