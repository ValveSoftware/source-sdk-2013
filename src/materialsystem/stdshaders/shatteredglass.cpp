//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "ShatteredGlass_ps20.inc"
#include "ShatteredGlass_ps20b.inc"
#include "ShatteredGlass_vs20.inc"

BEGIN_VS_SHADER( ShatteredGlass,
			  "Help for ShatteredGlass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "Glass/glasswindowbreak070b", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "Glass/glasswindowbreak070b", "detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "detail scale" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "glass/glasswindowbreak070b_mask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "1.0", "1.0 == mirror, 0.0 == water" )
		SHADER_PARAM( UNLITFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.7", "0.0 == multiply by lightmap, 1.0 == multiply by 1" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		
		if( !params[UNLITFACTOR]->IsDefined() )
			params[UNLITFACTOR]->SetFloatValue( 0.3f );

		if( !params[FRESNELREFLECTION]->IsDefined() )
			params[FRESNELREFLECTION]->SetFloatValue( 1.0f );

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		
		if( !params[ENVMAPFRAME]->IsDefined() )
			params[ENVMAPFRAME]->SetIntValue( 0 );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}
	}

	SHADER_FALLBACK
	{
        // MMW Shattered Glass runs as a DX9 effect for 8.2 hardware
        bool isDX9 = (g_pHardwareConfig->GetDXSupportLevel() >= 82);
		if( !isDX9 )
		{
			return "ShatteredGlass_DX8";
		}
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );

			if ( !params[BASETEXTURE]->GetTextureValue()->IsTranslucent() )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if ( params[DETAIL]->IsDefined() )
		{					 
			LoadTexture( DETAIL, TEXTUREFLAGS_SRGB );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if ( IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			LoadCubeMap( ENVMAP );
			if ( params[ENVMAPMASK]->IsDefined() )
			{
				LoadTexture( ENVMAPMASK, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0 );
			}
		}
	}

	SHADER_DRAW
	{
		bool bHasEnvmapMask = false;
		bool bHasEnvmap = false;
		if ( params[ENVMAP]->IsTexture() )
		{
			bHasEnvmap = true;
			if ( params[ENVMAPMASK]->IsTexture() )
			{
				bHasEnvmapMask = true;
			}
		}
		bool bHasVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		bool bHasBaseAlphaEnvmapMask = IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	
		// Base
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending, enable alpha blending if the detail texture is translucent
			bool detailIsTranslucent = TextureIsTranslucent( DETAIL, false );
			if ( detailIsTranslucent )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				else
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			pShaderShadow->EnableSRGBWrite( true );

			// Base texture
			unsigned int flags = VERTEX_POSITION;
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			// Lightmap
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
			}
			else
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
			}

			// Detail texture
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, true );

			// Envmap
			if ( bHasEnvmap )
			{
				flags |= VERTEX_NORMAL;
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
				{
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
				}
			
				if( bHasEnvmapMask )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
				}
			}

			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );

			if ( bHasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, 3, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( shatteredglass_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( ENVMAP_MASK,  bHasEnvmapMask );
			SET_STATIC_VERTEX_SHADER( shatteredglass_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( shatteredglass_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor );
				SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  bHasEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  bHasBaseAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
				SET_STATIC_PIXEL_SHADER( shatteredglass_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( shatteredglass_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor );
				SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  bHasEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  bHasBaseAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
				SET_STATIC_PIXEL_SHADER( shatteredglass_ps20 );
			}

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetVertexShaderTextureScale( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, DETAILSCALE );

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			BindTexture( SHADER_SAMPLER3, DETAIL );

			if( bHasEnvmap )
			{
				BindTexture( SHADER_SAMPLER2, ENVMAP, ENVMAPFRAME );
				if( bHasEnvmapMask )
				{
					BindTexture( SHADER_SAMPLER5, ENVMAPMASK, ENVMAPMASKFRAME );
				}
			}

			pShaderAPI->BindStandardTexture( SHADER_SAMPLER6, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );

			/*
			"DOWATERFOG"				"0..1"
			"ENVMAP_MASK"			"0..1"
			*/
			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
			int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

			DECLARE_DYNAMIC_VERTEX_SHADER( shatteredglass_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
			SET_DYNAMIC_VERTEX_SHADER( shatteredglass_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( shatteredglass_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( shatteredglass_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( shatteredglass_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( shatteredglass_ps20 );
			}

			SetEnvMapTintPixelShaderDynamicState( 0, ENVMAPTINT, -1 );
			SetModulationPixelShaderDynamicState( 1 );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );

			// [ 0, 0 ,0, R(0) ]
			float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fresnel[3] = params[FRESNELREFLECTION]->GetFloatValue();
			fresnel[0] = fresnel[1] = fresnel[2] = 1.0f - fresnel[3];
			pShaderAPI->SetPixelShaderConstant( 4, fresnel );

			float eyePos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
			pShaderAPI->SetPixelShaderConstant( 5, eyePos, 1 );

			pShaderAPI->SetPixelShaderFogParams( 12 );

			float overbright[4];
			overbright[0] = OVERBRIGHT;
			overbright[1] = params[UNLITFACTOR]->GetFloatValue();
			overbright[2] = overbright[3] = 1.0f - params[UNLITFACTOR]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 6, overbright );
		}
		Draw();
	}
END_SHADER
