//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "BaseVSShader.h"
#include "vertexlitgeneric_dx9_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Wireframe, Wireframe_DX9 )

BEGIN_VS_SHADER( Wireframe_DX9,
			  "Help for Wireframe_DX9" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsWindows() && g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Wireframe_DX8";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		VertexLitGeneric_DX9_Vars_t vars;
		InitParamsVertexLitGeneric_DX9( this, params, pMaterialName, false, vars );

		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_NOFOG );
		SET_FLAGS( MATERIAL_VAR_WIREFRAME );
	}

	SHADER_INIT
	{
		VertexLitGeneric_DX9_Vars_t vars;
		InitVertexLitGeneric_DX9( this, params, false, vars );
	}

	SHADER_DRAW
	{
		VertexLitGeneric_DX9_Vars_t vars;
		DrawVertexLitGeneric_DX9( this, params, pShaderAPI, pShaderShadow, false, vars, vertexCompression, pContextDataPtr );
	}
END_SHADER

