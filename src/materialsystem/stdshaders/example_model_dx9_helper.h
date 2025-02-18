//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader that can be applied to models
//
//==================================================================================================

#ifndef EXAMPLE_MODEL_DX9_HELPER_H
#define EXAMPLE_MODEL_DX9_HELPER_H

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
struct ExampleModel_DX9_Vars_t
{
	ExampleModel_DX9_Vars_t() { memset( this, 0xFF, sizeof(*this) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlphaTestReference;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
};

void InitParamsExampleModel_DX9( CBaseVSShader *pShader, IMaterialVar** params,
						 const char *pMaterialName, ExampleModel_DX9_Vars_t &info );

void InitExampleModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
				   ExampleModel_DX9_Vars_t &info );

void DrawExampleModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				   IShaderShadow* pShaderShadow,
				   ExampleModel_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr );

#endif // EXAMPLE_MODEL_DX9_HELPER_H
