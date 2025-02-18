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

DEFINE_FALLBACK_SHADER( UnlitTwoTexture, UnlitTwoTexture_DX6 )

BEGIN_SHADER( UnlitTwoTexture_DX6, 
			  "Help for UnlitTwoTexture_DX6" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
		if (params[TEXTURE2]->IsDefined())
			LoadTexture( TEXTURE2 );
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
				else
				{
					DisableAlphaBlending( );
				}
			}

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 |
				SHADER_DRAW_TEXCOORD1 );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			BindTexture( SHADER_SAMPLER1, TEXTURE2, FRAME2 );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, TEXTURE2TRANSFORM );
			SetModulationDynamicState();
		}
		Draw();
	}
END_SHADER
