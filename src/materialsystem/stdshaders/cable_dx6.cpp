//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Cable, Cable_DX6 )

BEGIN_SHADER( Cable_DX6,
			  "Help for Cable_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( MINLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.25", "Minimum amount of light (0-1 value)" )
		SHADER_PARAM( MAXLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.25", "Maximum amount of light" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	int GetDrawFlagsPass1(IMaterialVar** params )
	{
		int flags = SHADER_DRAW_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		flags |= SHADER_DRAW_TEXCOORD0;
		return flags;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			SetNormalBlendingShadowState( BASETEXTURE, true );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params) );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			Vector min, max;
			params[MINLIGHT]->GetVecValue( &min.x, 3 );
			params[MAXLIGHT]->GetVecValue( &max.x, 3 );
			Vector avg = ( min + max ) * 0.5f;
			pShaderAPI->Color3fv( &avg.x );	
		}
		Draw( );
	}
END_SHADER
