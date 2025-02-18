//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER( Cloud,
			  "Help for Cloud" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cloud", "cloud texture", 0 )
		SHADER_PARAM( CLOUDALPHATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cloudalpha", "cloud alpha texture" )
		SHADER_PARAM( CLOUDSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "cloudscale" )
		SHADER_PARAM( MASKSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "maskscale" )
	END_SHADER_PARAMS
	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( CLOUDALPHATEXTURE );
		if( !params[CLOUDSCALE]->IsDefined() )
		{
			params[CLOUDSCALE]->SetVecValue( 1.0f, 1.0f );
		}
		if( !params[MASKSCALE]->IsDefined() )
		{
			params[MASKSCALE]->SetVecValue( 1.0f, 1.0f );
		}
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetSamplerCount() >= 2 )
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

				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | 
					SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
				DefaultFog();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				BindTexture( SHADER_SAMPLER1, CLOUDALPHATEXTURE );

				// handle scrolling of base texture
				SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE0,
					BASETEXTURETRANSFORM, CLOUDSCALE );
				SetFixedFunctionTextureScale( MATERIAL_TEXTURE1, MASKSCALE );
			}
			Draw();
		}
		else
		{
			ShaderWarning("Cloud not supported for single-texturing boards!\n");
		}
	}
END_SHADER
