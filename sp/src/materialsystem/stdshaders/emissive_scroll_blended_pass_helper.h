//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

#ifndef EMISSIVE_SCROLL_BLENDED_PASS_HELPER_H
#define EMISSIVE_SCROLL_BLENDED_PASS_HELPER_H
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
struct EmissiveScrollBlendedPassVars_t
{
	EmissiveScrollBlendedPassVars_t() { memset( this, 0xFF, sizeof(EmissiveScrollBlendedPassVars_t) ); }

	int m_nBlendStrength; // Amount this layer is blended in globally
	int m_nBaseTexture;
	int m_nFlowTexture;
	int m_nEmissiveTexture;
	int m_nEmissiveTint;
	int m_nEmissiveScrollVector;
	int m_nTime;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultEmissiveBlendStrength = 0.0f;
static const float kDefaultEmissiveTint[4] = { 1.0f, 1.0f, 1.0f, 0.0f } ;
static const float kDefaultEmissiveScrollVector[4] = { 0.11f, 0.124f, 0.0f, 0.0f } ;

void InitParamsEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, EmissiveScrollBlendedPassVars_t &info );
void InitEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, EmissiveScrollBlendedPassVars_t &info );
void DrawEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
									IShaderShadow* pShaderShadow, EmissiveScrollBlendedPassVars_t &info, VertexCompressionType_t vertexCompression );

#endif // EMISSIVE_SCROLL_BLENDED_PASS_HELPER_H
