//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"
#include "unlittwotexture_vs20.inc"
#include "monitorscreen_ps20.inc"
#include "monitorscreen_ps20b.inc"
#include "cpp_shader_constant_register_map.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( MonitorScreen, MonitorScreen_DX9 )

BEGIN_VS_SHADER( MonitorScreen_DX9,
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
		if( params && !params[BASETEXTURE]->IsDefined() )
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

		if ( !(g_pHardwareConfig->SupportsPixelShaders_2_0() && g_pHardwareConfig->SupportsVertexShaders_2_0()) ||
			(g_pHardwareConfig->GetDXSupportLevel() < 90) )
		{
			return "MonitorScreen_DX8";
		}		

		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		}
		if (params[TEXTURE2]->IsDefined())
		{
			LoadTexture( TEXTURE2, TEXTUREFLAGS_SRGB );
		}
	}

	SHADER_DRAW
	{
		bool bHasTexture2 = params[TEXTURE2]->IsTexture();
		BlendType_t nBlendType = EvaluateBlendRequirements( BASETEXTURE, true );
		bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use

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

			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			DECLARE_STATIC_VERTEX_SHADER( unlittwotexture_vs20 );
			SET_STATIC_VERTEX_SHADER( unlittwotexture_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( monitorscreen_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( TEXTURE2, (bHasTexture2)?(1):(0) );
				SET_STATIC_PIXEL_SHADER( monitorscreen_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( monitorscreen_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( TEXTURE2, (bHasTexture2)?(1):(0) );
				SET_STATIC_PIXEL_SHADER( monitorscreen_ps20 );
			}

			DefaultFog();

			pShaderShadow->EnableAlphaWrites( bFullyOpaque );
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

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );


			DECLARE_DYNAMIC_VERTEX_SHADER( unlittwotexture_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( unlittwotexture_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( monitorscreen_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
				SET_DYNAMIC_PIXEL_SHADER( monitorscreen_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( monitorscreen_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( monitorscreen_ps20 );
			}
		}
		Draw();
	}
END_SHADER
