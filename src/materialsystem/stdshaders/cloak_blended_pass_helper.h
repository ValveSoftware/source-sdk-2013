//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef CLOAK_BLENDED_PASS_HELPER_H
#define CLOAK_BLENDED_PASS_HELPER_H
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
struct CloakBlendedPassVars_t
{
	CloakBlendedPassVars_t() { memset( this, 0xFF, sizeof(CloakBlendedPassVars_t) ); }

	int m_nCloakFactor;
	int m_nCloakColorTint;
	int m_nRefractAmount;

	int m_nBumpmap;
	int m_nBumpFrame;
	int m_nBumpTransform;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultCloakFactor = 0.0f;
static const float kDefaultCloakColorTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const float kDefaultRefractAmount = 0.1f;

void InitParamsCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, CloakBlendedPassVars_t &info );
void InitCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, CloakBlendedPassVars_t &info );
void DrawCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						   IShaderShadow* pShaderShadow, CloakBlendedPassVars_t &info, VertexCompressionType_t vertexCompression );
bool CloakBlendedPassIsFullyOpaque ( IMaterialVar** params, CloakBlendedPassVars_t &info );

#endif // CLOAK_BLENDED_PASS_HELPER_H
