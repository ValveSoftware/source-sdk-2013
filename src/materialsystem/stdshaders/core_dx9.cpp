//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "core_vs20.inc"
#include "core_ps20.inc"
#include "core_ps20b.inc"

#define MAXBLUR 1

DEFINE_FALLBACK_SHADER( Core, Core_DX90 )

BEGIN_VS_SHADER( Core_DX90, 
			  "Help for Core" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0f", "" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "envmap frame number" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( FLOWMAP, SHADER_PARAM_TYPE_TEXTURE, "", "flowmap" )
		SHADER_PARAM( FLOWMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $flowmap" )
		SHADER_PARAM( FLOWMAPSCROLLRATE, SHADER_PARAM_TYPE_VEC2, "[0 0", "2D rate to scroll $flowmap" )
		SHADER_PARAM( CORECOLORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" );
		SHADER_PARAM( CORECOLORTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" );
		SHADER_PARAM( FLOWMAPTEXCOORDOFFSET, SHADER_PARAM_TYPE_FLOAT, "0.0", "" );
	END_SHADER_PARAMS
	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[ENVMAPCONTRAST]->IsDefined() )
		{
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		}
		if( !params[ENVMAPSATURATION]->IsDefined() )
		{
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		}
		if( !params[ENVMAPFRAME]->IsDefined() )
		{
			params[ENVMAPFRAME]->SetIntValue( 0 );
		}
		if( !params[BASETEXTURE]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );
		}
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Core_dx80";

		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
		if ( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP, TEXTUREFLAGS_SRGB );
		}
		if ( params[FLOWMAP]->IsDefined() )
		{
			LoadTexture( FLOWMAP );
		}
		if ( params[CORECOLORTEXTURE]->IsDefined() )
		{
			LoadTexture( CORECOLORTEXTURE );
		}
	}

	inline void DrawPass( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, int nPass, VertexCompressionType_t vertexCompression ) 
	{
		bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
		bool bHasEnvmap = params[ENVMAP]->IsTexture();
		bool bHasFlowmap = params[FLOWMAP]->IsTexture();
		bool bHasCoreColorTexture = params[CORECOLORTEXTURE]->IsTexture();

		SHADOW_STATE
		{
			SetInitialShadowState( );

			if( nPass == 0 )
			{
				// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
				pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );
			}
			else
			{
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
				EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			}

			// If envmap is not specified, the alpha channel is the translucency
			// (If envmap *is* specified, alpha channel is the reflection amount)
			if ( params[NORMALMAP]->IsTexture() && !bHasEnvmap )
			{
				SetDefaultBlendingShadowState( NORMALMAP, false );
			}

			// source render target that contains the image that we are warping.
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
			}

			// normal map
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			if( bHasEnvmap )
			{
				// envmap
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
				{
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
				}
			}

			if( bHasFlowmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );
			}

			if( bHasCoreColorTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
			}

			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBWrite( true );
			}

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			int userDataSize = 0;
			int nTexCoordCount = 1;
			if( bIsModel )
			{
				userDataSize = 4;
			}
			else
			{
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			}

			// This shader supports compressed vertices, so OR in that flag:
			flags |= VERTEX_FORMAT_COMPRESSED;

			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			DECLARE_STATIC_VERTEX_SHADER( core_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
			SET_STATIC_VERTEX_SHADER( core_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( core_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap && ( nPass == 1 ) );
				SET_STATIC_PIXEL_SHADER_COMBO( FLOWMAP, bHasFlowmap );
				SET_STATIC_PIXEL_SHADER_COMBO( CORECOLORTEXTURE, bHasCoreColorTexture && ( nPass == 0 ) );
				SET_STATIC_PIXEL_SHADER_COMBO( REFRACT, nPass == 0 );
				SET_STATIC_PIXEL_SHADER( core_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( core_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap && ( nPass == 1 ) );
				SET_STATIC_PIXEL_SHADER_COMBO( FLOWMAP, bHasFlowmap );
				SET_STATIC_PIXEL_SHADER_COMBO( CORECOLORTEXTURE, bHasCoreColorTexture && ( nPass == 0 ) );
				SET_STATIC_PIXEL_SHADER_COMBO( REFRACT, nPass == 0 );
				SET_STATIC_PIXEL_SHADER( core_ps20 );
			}

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			if ( params[BASETEXTURE]->IsTexture() )
			{
				BindTexture( SHADER_SAMPLER2, BASETEXTURE, FRAME );
			}
			else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			}

			BindTexture( SHADER_SAMPLER3, NORMALMAP, BUMPFRAME );

			if( bHasEnvmap )
			{
				BindTexture( SHADER_SAMPLER4, ENVMAP, ENVMAPFRAME );
			}

			if( bHasFlowmap )
			{
				BindTexture( SHADER_SAMPLER6, FLOWMAP, FLOWMAPFRAME );
			}

			if( bHasCoreColorTexture )
			{
				BindTexture( SHADER_SAMPLER7, CORECOLORTEXTURE, CORECOLORTEXTUREFRAME );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( core_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( core_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( core_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( core_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( core_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( core_ps20 );
			}

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				SetPixelShaderConstant( 0, ENVMAPTINT );
				SetPixelShaderConstant( 1, REFRACTTINT );
			}
			else
			{
				SetPixelShaderConstantGammaToLinear( 0, ENVMAPTINT );
				SetPixelShaderConstantGammaToLinear( 1, REFRACTTINT );
			}
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );
			float c5[4] = { params[REFRACTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

			float eyePos[4];
			s_pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
			s_pShaderAPI->SetPixelShaderConstant( 8, eyePos, 1 );
			pShaderAPI->SetPixelShaderFogParams( 11 );



			if( bHasFlowmap )
			{
				float curTime = pShaderAPI->CurrentTime();
				float timeVec[4] = { curTime, curTime, curTime, curTime };
				pShaderAPI->SetPixelShaderConstant( 6, timeVec, 1 );

				SetPixelShaderConstant( 7, FLOWMAPSCROLLRATE );

				SetPixelShaderConstant( 9, FLOWMAPTEXCOORDOFFSET );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		DrawPass( params, pShaderShadow, pShaderAPI, 0, vertexCompression );
		DrawPass( params, pShaderShadow, pShaderAPI, 1, vertexCompression );
	}
END_SHADER

