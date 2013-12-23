//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "convar.h"

#include "lightmappedgeneric_vs20.inc"
#include "WorldTwoTextureBlend_ps20.inc"
#include "WorldTwoTextureBlend_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_flashlight_version2;

// FIXME: Need to make a dx9 version so that "CENTROID" works.
BEGIN_VS_SHADER( WorldTwoTextureBlend, 
			  "Help for WorldTwoTextureBlend" )

BEGIN_SHADER_PARAMS
    SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend", "iris texture", 0 )
	SHADER_PARAM( ALBEDO, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend", "albedo (Base texture with no baked lighting)" )
	SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
	SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend_detail", "detail texture" )
	SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
	SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "scale of the detail texture" )
	SHADER_PARAM( DETAIL_ALPHA_MASK_BASE_TEXTURE, SHADER_PARAM_TYPE_BOOL, "0", 
				  "If this is 1, then when detail alpha=0, no base texture is blended and when "
				  "detail alpha=1, you get detail*base*lightmap" )
	SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
	SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
	SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
	SHADER_PARAM( NODIFFUSEBUMPLIGHTING, SHADER_PARAM_TYPE_INTEGER, "0", "0 == Use diffuse bump lighting, 1 = No diffuse bump lighting" )
	SHADER_PARAM( SEAMLESS_SCALE, SHADER_PARAM_TYPE_FLOAT, "0", "Scale factor for 'seamless' texture mapping. 0 means to use ordinary mapping" )
END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "WorldTwoTextureBlend_DX6";

		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "WorldTwoTextureBlend_DX8";

		return 0;
	}

	SHADER_INIT_PARAMS()
	{

		if( !params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->IsDefined() )
		{
			params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->SetIntValue( 0 );
		}
	
		if ( g_pHardwareConfig->SupportsBorderColor() )
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
		}
		else
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
		}
	
		// Write over $basetexture with $albedo if we are going to be using diffuse normal mapping.
		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() && params[ALBEDO]->IsDefined() &&
			params[BASETEXTURE]->IsDefined() && 
			!( params[NODIFFUSEBUMPLIGHTING]->IsDefined() && params[NODIFFUSEBUMPLIGHTING]->GetIntValue() ) )
		{
			params[BASETEXTURE]->SetStringValue( params[ALBEDO]->GetStringValue() );
		}
	
		if( !params[NODIFFUSEBUMPLIGHTING]->IsDefined() )
		{
			params[NODIFFUSEBUMPLIGHTING]->SetIntValue( 0 );
		}

		if( !params[SELFILLUMTINT]->IsDefined() )
		{
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}

		if( !params[DETAILSCALE]->IsDefined() )
		{
			params[DETAILSCALE]->SetFloatValue( 4.0f );
		}

		if( !params[BUMPFRAME]->IsDefined() )
		{
			params[BUMPFRAME]->SetIntValue( 0 );
		}

		if( !params[DETAILFRAME]->IsDefined() )
		{
			params[DETAILFRAME]->SetIntValue( 0 );
		}

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() && (params[NODIFFUSEBUMPLIGHTING]->GetIntValue() == 0) )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
		}
	}

	SHADER_INIT
	{
		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() )
		{
			LoadBumpMap( BUMPMAP );
		}
		
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		LoadTexture( FLASHLIGHTTEXTURE, TEXTUREFLAGS_SRGB );
		
		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
		{
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
		}
			
		// We always need this because of the flashlight.
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}

	void DrawPass( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
		IShaderShadow* pShaderShadow, bool hasFlashlight, VertexCompressionType_t vertexCompression )
	{
		bool hasBump = params[BUMPMAP]->IsTexture();
		bool hasDiffuseBumpmap = hasBump && (params[NODIFFUSEBUMPLIGHTING]->GetIntValue() == 0);
		bool hasBaseTexture = params[BASETEXTURE]->IsTexture();
		bool hasDetailTexture = /*!hasBump && */params[DETAIL]->IsTexture();
		bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0;
		bool bHasDetailAlpha = params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->GetIntValue() != 0;
		bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;

		BlendType_t nBlendType = EvaluateBlendRequirements( BASETEXTURE, true );
		bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use

		bool bSeamlessMapping = params[SEAMLESS_SCALE]->GetFloatValue() != 0.0;

		SHADOW_STATE
		{
			int nShadowFilterMode = 0;

			// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
			pShaderShadow->EnableAlphaTest( bIsAlphaTested );
			if( hasFlashlight )
			{
				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
				}

				SetAdditiveBlendingShadowState( BASETEXTURE, true );
				pShaderShadow->EnableDepthWrites( false );

				// Be sure not to write to dest alpha
				pShaderShadow->EnableAlphaWrites( false );
			}
			else
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			unsigned int flags = VERTEX_POSITION;
			if( hasBaseTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			}
			//			if( hasLightmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE );
			}
			if( hasFlashlight )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER7 );
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
			}
			if( hasDetailTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			}
			if( hasBump )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			}
			if( hasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );

			// texcoord0 : base texcoord
			// texcoord1 : lightmap texcoord
			// texcoord2 : lightmap texcoord offset
			int numTexCoords = 2;
			if( hasBump )
			{
				numTexCoords = 3;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			// Pre-cache pixel shaders
			bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );

			pShaderShadow->EnableSRGBWrite( true );

			DECLARE_STATIC_VERTEX_SHADER( lightmappedgeneric_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( ENVMAP_MASK,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMASK,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( TANGENTSPACE,  hasFlashlight );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP,  hasBump );
			SET_STATIC_VERTEX_SHADER_COMBO( DIFFUSEBUMPMAP,  hasDiffuseBumpmap );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  hasVertexColor );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXALPHATEXBLENDFACTOR, false );
			SET_STATIC_VERTEX_SHADER_COMBO( RELIEF_MAPPING, 0 ); //( bumpmap_variant == 2 )?1:0);
			SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS, bSeamlessMapping ); //( bumpmap_variant == 2 )?1:0);
#ifdef _X360
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, hasFlashlight );
#endif
			SET_STATIC_VERTEX_SHADER( lightmappedgeneric_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( worldtwotextureblend_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  hasDetailTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP,  hasBump );
				SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSEBUMPMAP,  hasDiffuseBumpmap );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  hasVertexColor );
				SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_ALPHA_MASK_BASE_TEXTURE,  bHasDetailAlpha );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
				SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS,  bSeamlessMapping );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
				SET_STATIC_PIXEL_SHADER( worldtwotextureblend_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( worldtwotextureblend_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  hasDetailTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP,  hasBump );
				SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSEBUMPMAP,  hasDiffuseBumpmap );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  hasVertexColor );
				SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_ALPHA_MASK_BASE_TEXTURE,  bHasDetailAlpha );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
				SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS,  bSeamlessMapping );
				SET_STATIC_PIXEL_SHADER( worldtwotextureblend_ps20 );
			}

			// HACK HACK HACK - enable alpha writes all the time so that we have them for
			// underwater stuff. 
			// But only do it if we're not using the alpha already for translucency
			pShaderShadow->EnableAlphaWrites( bFullyOpaque );


			if( hasFlashlight )
			{
				FogToBlack();
			}
			else
			{
				DefaultFog();
			}
		}
		DYNAMIC_STATE
		{
			if( hasBaseTexture )
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_WHITE );
			}

			//			if( hasLightmap )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			}

			bool bFlashlightShadows = false;
			if( hasFlashlight )
			{
				VMatrix worldToTexture;
				ITexture *pFlashlightDepthTexture;
				FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
				bFlashlightShadows = state.m_bEnableShadows && ( pFlashlightDepthTexture != NULL );

				SetFlashLightColorFromState( state, pShaderAPI );

				BindTexture( SHADER_SAMPLER2, state.m_pSpotlightTexture, state.m_nSpotlightTextureFrame );

				if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() )
				{
					BindTexture( SHADER_SAMPLER7, pFlashlightDepthTexture );
				}
			}
			if( hasDetailTexture )
			{
				BindTexture( SHADER_SAMPLER3, DETAIL, DETAILFRAME );
			}
			if( hasBump )
			{
				if( !g_pConfig->m_bFastNoBump )
				{
					BindTexture( SHADER_SAMPLER4, BUMPMAP, BUMPFRAME );
				}
				else
				{
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER4, TEXTURE_NORMALMAP_FLAT );
				}
			}
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER6, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );

			// If we don't have a texture transform, we don't have
			// to set vertex shader constants or run vertex shader instructions
			// for the texture transform.
			bool bHasTextureTransform = 
				!( params[BASETEXTURETRANSFORM]->MatrixIsIdentity() &&
				params[BUMPTRANSFORM]->MatrixIsIdentity() );

			bool bVertexShaderFastPath = !bHasTextureTransform;
			if( params[DETAIL]->IsTexture() )
			{
				bVertexShaderFastPath = false;
			}
			if( pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0 )
			{
				bVertexShaderFastPath = false;
			}

			float color[4] = { 1.0, 1.0, 1.0, 1.0 };
			ComputeModulationColor( color );
			if( !( bVertexShaderFastPath && color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f && color[3] == 1.0f ) )
			{
				bVertexShaderFastPath = false;
				s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );
				if (! bSeamlessMapping)
					SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
				if( hasBump && !bHasDetailAlpha )
				{
					SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, BUMPTRANSFORM );
					Assert( !hasDetailTexture );
				}
			}

			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
			DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( FASTPATH,  bVertexShaderFastPath );
			SET_DYNAMIC_VERTEX_SHADER_COMBO(
				LIGHTING_PREVIEW, pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
			SET_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_vs20 );

			bool bWriteDepthToAlpha;
			bool bWriteWaterFogToAlpha;
			if( bFullyOpaque ) 
			{
				bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
				bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);
				AssertMsg( !(bWriteDepthToAlpha && bWriteWaterFogToAlpha), "Can't write two values to alpha at the same time." );
			}
			else
			{
				//can't write a special value to dest alpha if we're actually using as-intended alpha
				bWriteDepthToAlpha = false;
				bWriteWaterFogToAlpha = false;
			}


			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( worldtwotextureblend_ps20b );

				// Don't write fog to alpha if we're using translucency
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
				SET_DYNAMIC_PIXEL_SHADER( worldtwotextureblend_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( worldtwotextureblend_ps20 );

				// Don't write fog to alpha if we're using translucency
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) && 
					(nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( worldtwotextureblend_ps20 );
			}


			// always set the transform for detail textures since I'm assuming that you'll
			// always have a detailscale.
			if( hasDetailTexture )
			{
				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, BASETEXTURETRANSFORM, DETAILSCALE );
				Assert( !( hasBump && !bHasDetailAlpha ) );
			}

			SetPixelShaderConstantGammaToLinear( 7, SELFILLUMTINT );

			float eyePos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
			pShaderAPI->SetPixelShaderConstant( 10, eyePos, 1 );
			pShaderAPI->SetPixelShaderFogParams( 11 );

			if ( bSeamlessMapping )
			{
				float map_scale[4]={ params[SEAMLESS_SCALE]->GetFloatValue(),0,0,0};
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, map_scale );
			}


			if( hasFlashlight )
			{
				VMatrix worldToTexture;
				const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );

				// Set the flashlight attenuation factors
				float atten[4];
				atten[0] = flashlightState.m_fConstantAtten;
				atten[1] = flashlightState.m_fLinearAtten;
				atten[2] = flashlightState.m_fQuadraticAtten;
				atten[3] = flashlightState.m_FarZ;
				pShaderAPI->SetPixelShaderConstant( 20, atten, 1 );

				// Set the flashlight origin
				float pos[4];
				pos[0] = flashlightState.m_vecLightOrigin[0];
				pos[1] = flashlightState.m_vecLightOrigin[1];
				pos[2] = flashlightState.m_vecLightOrigin[2];
				pos[3] = 1.0f;
				pShaderAPI->SetPixelShaderConstant( 15, pos, 1 );

				pShaderAPI->SetPixelShaderConstant( 16, worldToTexture.Base(), 4 );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		bool bHasFlashlight = UsingFlashlight( params );
		if ( bHasFlashlight && ( IsX360() || r_flashlight_version2.GetInt() ) )
		{
			DrawPass( params, pShaderAPI, pShaderShadow, false, vertexCompression );
			SHADOW_STATE
			{
				SetInitialShadowState( );
			}
		}
		DrawPass( params, pShaderAPI, pShaderShadow, bHasFlashlight, vertexCompression );
	}

END_SHADER

