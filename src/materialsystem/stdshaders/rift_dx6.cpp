//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER( Rift_DX6, 
			  "Help for Rift_DX6" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );
		}
		if (params[TEXTURE2]->IsDefined())
		{
			LoadTexture( TEXTURE2 );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			SetModulationShadowState();

			// Either we've got a constant modulation
			bool isTranslucent = IsAlphaModulating();

			// Or we've got a texture alpha on either texture
			isTranslucent = isTranslucent || TextureIsTranslucent( BASETEXTURE, true ) ||
				TextureIsTranslucent( TEXTURE2, true );

			if ( isTranslucent )
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
			}
			else
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
				{
					EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				}
			}

			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// 1) Take a coordinate in camera space
			// 2) Flip Y by multiplying by -1
			// 3) Transform from [-w,w] to [0,2*w]
			// 4) Transform from [0,2*w] to [0,w]
			// We'll end up dividing by w in the pixel shader to get to [0,1]
			VMatrix matProjection, matYFlip, matHalf, matOffset, matBaseTransform;
			s_pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProjection.m[0] );

			MatrixTranspose( matProjection, matProjection );

			MatrixBuildScale( matYFlip, 1.0f, -1.0f, 1.0f );
			MatrixBuildTranslation( matOffset, 1.0f, 1.0f, 0.0f );
			MatrixBuildScale( matHalf, 0.5f, 0.5f, 1.0f );

			MatrixMultiply( matYFlip, matProjection, matBaseTransform );
			MatrixMultiply( matOffset, matBaseTransform, matBaseTransform );
			MatrixMultiply( matHalf, matBaseTransform, matBaseTransform );

			// tranpose before going into the shaderapi. . . suck
			MatrixTranspose( matBaseTransform, matBaseTransform );
			s_pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
			s_pShaderAPI->LoadMatrix( &matBaseTransform[0][0] );

			// NOTE: This *must* be set after LoadMatrix since LoadMatrix slams the texture dimension
			pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE0, 3, true );

			BindTexture( SHADER_SAMPLER1, TEXTURE2, FRAME2 );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, TEXTURE2TRANSFORM );

			SetModulationDynamicState();
		}
		Draw();
	}
END_SHADER
