//========= Copyright Valve Corporation, All rights reserved. ============//
//
// DirectX 9 Cloud shader
//
//===============================================================================

#include "BaseVSShader.h"
#include "cloud_vs20.inc"
#include "cloud_ps20.inc"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Cloud, Cloud_dx9 )

BEGIN_VS_SHADER( Cloud_dx9, "Help for Cloud" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cloud", "cloud texture", 0 )
		SHADER_PARAM( CLOUDALPHATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cloudalpha", "cloud alpha texture" )
		SHADER_PARAM( CLOUDSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "cloudscale" )
		SHADER_PARAM( MASKSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "maskscale" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Cloud_dx8";

		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		LoadTexture( CLOUDALPHATEXTURE, TEXTUREFLAGS_SRGB );
		if ( !params[CLOUDSCALE]->IsDefined() )
		{
			params[CLOUDSCALE]->SetVecValue( 1.0f, 1.0f );
		}
		if ( !params[MASKSCALE]->IsDefined() )
		{
			params[MASKSCALE]->SetVecValue( 1.0f, 1.0f );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
			{
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			}
			else
			{
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 2, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( cloud_vs20 );
			SET_STATIC_VERTEX_SHADER( cloud_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( cloud_ps20 );
			SET_STATIC_PIXEL_SHADER( cloud_ps20 );

			DefaultFog();
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER1, CLOUDALPHATEXTURE );

			// Handle scrolling of base texture
			SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM, CLOUDSCALE );
			SetVertexShaderTextureScale( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, MASKSCALE );

			DECLARE_DYNAMIC_VERTEX_SHADER( cloud_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( cloud_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( cloud_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( cloud_ps20 );
		}

		Draw();
	}
END_SHADER
