//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "rift_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Rift_DX8, 
			  "Help for Rift_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()	
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

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
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

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

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );
			pShaderShadow->SetVertexShader( "rift_vs11", 0 );
			pShaderShadow->SetPixelShader( "rift_ps11", 0 );

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			pShaderAPI->SetTextureTransformDimension( 0, 3, true );

			BindTexture( SHADER_TEXTURE_STAGE1, TEXTURE2, FRAME2 );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, TEXTURE2TRANSFORM );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetModulationPixelShaderDynamicState( 0 );

			rift_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
