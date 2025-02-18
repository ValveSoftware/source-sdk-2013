//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef WORLDVERTEXTRANSITION_DX6_HELPER_H
#define WORLDVERTEXTRANSITION_DX6_HELPER_H

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;


//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct WorldVertexTransition_DX6_Vars_t
{
	WorldVertexTransition_DX6_Vars_t() { memset( this, 0xFF, sizeof(WorldVertexTransition_DX6_Vars_t) ); }

	int m_nBaseTextureVar;
	int m_nBaseTextureFrameVar;
	int m_nBaseTexture2Var;
	int m_nFlashlightTextureVar;
};

void InitParamsWorldVertexTransition_DX6( IMaterialVar** params, WorldVertexTransition_DX6_Vars_t &info );
void InitWorldVertexTransition_DX6( CBaseShader *pShader, IMaterialVar** params, WorldVertexTransition_DX6_Vars_t &info );
void DrawWorldVertexTransition_DX6( CBaseShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, WorldVertexTransition_DX6_Vars_t &info );


#endif // WORLDVERTEXTRANSITION_DX6_HELPER_H