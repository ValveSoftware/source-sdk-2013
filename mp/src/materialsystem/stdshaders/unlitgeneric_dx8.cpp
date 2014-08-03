//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( UnlitGeneric, UnlitGeneric_DX8 )

BEGIN_VS_SHADER( UnlitGeneric_DX8,
			  "Help for UnlitGeneric_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "0", "Make the envmap only apply to dx9 and higher hardware" )
		SHADER_PARAM( DETAILBLENDMODE, SHADER_PARAM_TYPE_INTEGER, "0", "mode for combining detail texture with base. 0=normal, 1= additive, 2=alpha blend detail over base, 3=crossfade" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.7", "" )	
		SHADER_PARAM( OUTLINE, SHADER_PARAM_TYPE_BOOL, "0", "Enable outline for distance coded textures.")
		SHADER_PARAM( OUTLINECOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "color of outline for distance coded images." )
		SHADER_PARAM( OUTLINESTART0, SHADER_PARAM_TYPE_FLOAT, "0.0", "outer start value for outline")
		SHADER_PARAM( OUTLINESTART1, SHADER_PARAM_TYPE_FLOAT, "0.0", "inner start value for outline")
		SHADER_PARAM( OUTLINEEND0, SHADER_PARAM_TYPE_FLOAT, "0.0", "inner end value for outline")
		SHADER_PARAM( OUTLINEEND1, SHADER_PARAM_TYPE_FLOAT, "0.0", "outer end value for outline")
		SHADER_PARAM( SEPARATEDETAILUVS, SHADER_PARAM_TYPE_INTEGER, "0", "" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "UnlitGeneric_DX6";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		InitParamsUnlitGeneric_DX8(
			BASETEXTURE, DETAILSCALE, ENVMAPOPTIONAL,
			ENVMAP, ENVMAPTINT, ENVMAPMASKSCALE,
			DETAILBLENDMODE );
	}

	SHADER_INIT
	{
		InitUnlitGeneric_DX8( BASETEXTURE, DETAIL, ENVMAP, ENVMAPMASK );
	}

	SHADER_DRAW
	{
		VertexShaderUnlitGenericPass( 
			BASETEXTURE, FRAME, BASETEXTURETRANSFORM,
			DETAIL, DETAILSCALE, true, ENVMAP, ENVMAPFRAME, ENVMAPMASK,
			ENVMAPMASKFRAME, ENVMAPMASKSCALE, ENVMAPTINT, ALPHATESTREFERENCE,
			DETAILBLENDMODE,
			OUTLINE, OUTLINECOLOR, OUTLINESTART0, OUTLINEEND1, SEPARATEDETAILUVS );
	}
END_SHADER

