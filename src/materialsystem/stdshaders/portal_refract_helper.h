//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef PORTALREFRACT_HELPER_H
#define PORTALREFRACT_HELPER_H
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
struct PortalRefractVars_t
{
	PortalRefractVars_t() { memset( this, 0xFF, sizeof(PortalRefractVars_t) ); }

	int m_nStage;
	int m_nPortalOpenAmount;
	int m_nPortalStatic;
	int m_nPortalMaskTexture;
	int m_nTextureTransform;
	int m_nPortalColorTexture;
	int m_nPortalColorScale;
	int m_nTime;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultPortalStatic = 0.0f;
static const float kDefaultPortalOpenAmount = 0.0f;
static const float kDefaultPortalColorScale = 1.0f;

void InitParamsPortalRefract( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, PortalRefractVars_t &info );
void InitPortalRefract( CBaseVSShader *pShader, IMaterialVar** params, PortalRefractVars_t &info );
void DrawPortalRefract( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						   IShaderShadow* pShaderShadow, PortalRefractVars_t &info );

#endif // PortalRefract_HELPER_H
