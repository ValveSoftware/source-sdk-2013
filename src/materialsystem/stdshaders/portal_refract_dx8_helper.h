//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef PORTALREFRACT_DX8_HELPER_H
#define PORTALREFRACT_DX8_HELPER_H
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
struct PortalRefractVarsDX8_t
{
	PortalRefractVarsDX8_t() { memset( this, 0xFF, sizeof(PortalRefractVarsDX8_t) ); }

	int m_nStage;
	int m_nPortalOpenAmount;
	int m_nPortalStatic;
	int m_nPortalMaskTexture;
	int m_nTextureTransform;
	int m_nPortalColorTexture;
	int m_nPortalColorScale;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultPortalStatic = 0.0f;
static const float kDefaultPortalOpenAmount = 0.0f;
static const float kDefaultPortalColorScale = 1.0f;

void InitParamsPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, PortalRefractVarsDX8_t &info );
void InitPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, PortalRefractVarsDX8_t &info );
void DrawPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						    IShaderShadow* pShaderShadow, PortalRefractVarsDX8_t &info );

#endif // PortalRefract_DX8_HELPER_H
