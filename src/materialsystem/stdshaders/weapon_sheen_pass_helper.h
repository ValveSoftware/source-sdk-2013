//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_SHEEN_PASS_HELPER_H
#define WEAPON_SHEEN_PASS_HELPER_H
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
struct WeaponSheenPassVars_t
{
	WeaponSheenPassVars_t() { memset( this, 0xFF, sizeof(WeaponSheenPassVars_t) ); }

	int m_nSheenMap;
	int m_nSheenMapMask;
	int m_nSheenMapMaskFrame;
	int m_nSheenMapTint;
	int m_nSheenMapMaskScaleX;
	int m_nSheenMapMaskScaleY;
	int m_nSheenMapMaskOffsetX;
	int m_nSheenMapMaskOffsetY;
	int m_nSheenMapMaskDirection;
	int m_nSheenIndex;

	int m_nBumpmap;
	int m_nBumpFrame;
	int m_nBumpTransform;
};

static const float kDefaultSheenColorTint[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

void InitParamsWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, WeaponSheenPassVars_t &info );
void InitWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, WeaponSheenPassVars_t &info );
void DrawWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						   IShaderShadow* pShaderShadow, WeaponSheenPassVars_t &info, VertexCompressionType_t vertexCompression );

bool ShouldDrawMaterialSheen ( IMaterialVar** params, WeaponSheenPassVars_t &info );

#endif // WEAPON_SHEEN_PASS_HELPER_H
