//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SKIN_DX9_HELPER_H
#define SKIN_DX9_HELPER_H

#include <string.h>

#include "vertexlitgeneric_dx9_helper.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

void InitParamsSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params,
						 const char *pMaterialName, VertexLitGeneric_DX9_Vars_t &info );
void InitSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
				   VertexLitGeneric_DX9_Vars_t &info );

void DrawSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				   IShaderShadow* pShaderShadow,
				   VertexLitGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr );

				   

#endif // SKIN_DX9_HELPER_H
