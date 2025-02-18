//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"

#include "unlittwotexture.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( MonitorScreen, MonitorScreen_DX8 )

BEGIN_VS_SHADER( MonitorScreen_DX8,
			  "This is a shader that does a contrast/saturation version of base times lightmap." )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( CONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( SATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( TINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "monitor tint" )
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/lightmappedtexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[CONTRAST]->IsDefined() )
		{
			params[CONTRAST]->SetFloatValue( 0.0f );
		}
		if( !params[SATURATION]->IsDefined() )
		{
			params[SATURATION]->SetFloatValue( 1.0f );
		}
		if( !params[TINT]->IsDefined() )
		{
			params[TINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
		{
			CLEAR_FLAGS( MATERIAL_VAR_MODEL );
		}
	}

	SHADER_FALLBACK
	{
		if( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 || (params && !params[BASETEXTURE]->IsDefined()) )
		{
			if( IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ) )
			{
				return "VertexLitGeneric_DX6";
			}
			else
			{
				return "LightmappedGeneric_DX6";
			}
		}
		return 0;
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
		bool bHasTexture2 = params[TEXTURE2]->IsTexture();
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			if ( bHasTexture2 )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
			}

			pShaderShadow->EnableSRGBWrite( true );

			// Either we've got a constant modulation
			bool isTranslucent = IsAlphaModulating();

			// Or we've got a texture alpha on either texture
			isTranslucent = isTranslucent || TextureIsTranslucent( BASETEXTURE, true ) ||
				TextureIsTranslucent( TEXTURE2, true );

			if ( isTranslucent )
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				else
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
					EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				else
					DisableAlphaBlending( );
			}

			int fmt = VERTEX_POSITION | VERTEX_NORMAL;
			if (IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ))
				fmt |= VERTEX_COLOR;

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );
			
			unlittwotexture_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "UnlitTwoTexture", vshIndex.GetIndex() );

			int pshIndex = bHasTexture2 ? 1 : 0;
			pShaderShadow->SetPixelShader( "MonitorScreen", pshIndex );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			if( bHasTexture2 )
			{
				BindTexture( SHADER_SAMPLER1, TEXTURE2, FRAME2 );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, TEXTURE2TRANSFORM );
			}
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetPixelShaderConstant( 1, CONTRAST );
			SetPixelShaderConstant( 2, SATURATION );
			SetPixelShaderConstant( 3, TINT );
			SetModulationVertexShaderDynamicState();

			unlittwotexture_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
