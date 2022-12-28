//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

#include "SDK_decalmodulate_vs20.inc"
#include "SDK_decalmodulate_ps20.inc"
#include "SDK_decalmodulate_ps20b.inc"

#ifndef _X360
#include "SDK_decalmodulate_vs30.inc"
#include "SDK_decalmodulate_ps30.inc"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef MAPBASE
ConVar mat_decalmodulate_flashdraw( "mat_decalmodulate_flashdraw", "0" );
#endif

DEFINE_FALLBACK_SHADER( SDK_DecalModulate, SDK_DecalModulate_DX9 )

BEGIN_VS_SHADER( SDK_DecalModulate_dx9, 
			  "Help for SDK_DecalModulate_dx9" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FOGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "0.4", "exponent to tweak fog fade" )
		SHADER_PARAM( FOGSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "scale to tweak fog fade" )
	END_SHADER_PARAMS
	
	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		if( !params[ FOGEXPONENT ]->IsDefined() )
		{
			params[ FOGEXPONENT ]->SetFloatValue( 0.4f );
		}

		if( !params[ FOGSCALE ]->IsDefined() )
		{
			params[ FOGSCALE ]->SetFloatValue( 1.0f );
		}

		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );

#ifndef _X360
		if ( g_pHardwareConfig->HasFastVertexTextures() )
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		}
#endif
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
#ifdef MAPBASE
		// It is now believed the decals not appearing is a sorting issue.
		// The flashlight part is transparent and overlaid on top of the decal.
		// When a fix is found, this flashlight code could be removed.
		bool bHasFlashlight = UsingFlashlight( params );
		if (bHasFlashlight && !mat_decalmodulate_flashdraw.GetBool())
			return;
#endif
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			//SRGB conversions hose the blend on some hardware, so keep everything in gamma space.
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
			pShaderShadow->EnableSRGBWrite( false );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			pShaderShadow->DisableFogGammaCorrection( true ); //fog should stay exactly middle grey
			FogToGrey();

#ifdef MAPBASE
			int userDataSize = 0;
			int nShadowFilterMode = 0;
			if ( bHasFlashlight )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );	// Depth texture
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER8 );
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );	// Noise map
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	// Flashlight cookie
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER7, true );
				userDataSize = 4; // tangent S

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
				}
			}
#endif

			bool bHasVertexAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) && IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				DECLARE_STATIC_VERTEX_SHADER( sdk_decalmodulate_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  bHasVertexAlpha );
				SET_STATIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, false );
#ifdef MAPBASE
				SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
#endif
				SET_STATIC_VERTEX_SHADER( sdk_decalmodulate_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps20b );
					SET_STATIC_PIXEL_SHADER_COMBO( VERTEXALPHA,  bHasVertexAlpha );
#ifdef MAPBASE
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
#endif
					SET_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps20 );
					SET_STATIC_PIXEL_SHADER_COMBO( VERTEXALPHA,  bHasVertexAlpha );
#ifdef MAPBASE
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
#endif
					SET_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps20 );
				}
			}
#ifndef _X360
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( sdk_decalmodulate_vs30 );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  bHasVertexAlpha );
				SET_STATIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, false );
#ifdef MAPBASE
				SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
#endif
				SET_STATIC_VERTEX_SHADER( sdk_decalmodulate_vs30 );

				DECLARE_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps30 );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEXALPHA,  bHasVertexAlpha );
#ifdef MAPBASE
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
#endif
				SET_STATIC_PIXEL_SHADER( sdk_decalmodulate_ps30 );
			}
#endif

			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;

			if ( bHasVertexAlpha )
			{
				flags |= VERTEX_COLOR;
			}

#ifndef _X360
			// The VS30 shader offsets decals along the normal (for morphed geom)
			flags |= g_pHardwareConfig->HasFastVertexTextures() ? VERTEX_NORMAL : 0;
#endif
			int pTexCoordDim[3] = { 2, 0, 3 };
			int nTexCoordCount = 1;
#ifndef MAPBASE
			int userDataSize = 0;
#endif

#ifndef _X360
			if ( g_pHardwareConfig->HasFastVertexTextures() )
			{
#ifdef MAPBASE
				if (nTexCoordCount == 0)
					nTexCoordCount = 3;
#else
				nTexCoordCount = 3;
#endif
			}
#endif

			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, pTexCoordDim, userDataSize );
		}
		DYNAMIC_STATE
		{
#ifdef MAPBASE // This fixes blood decals, etc. not showing up under flashlights.
			//bHasFlashlight = pShaderAPI->InFlashlightMode();
			bool bFlashlightShadows = false;
			if ( bHasFlashlight )
			{
				VMatrix worldToTexture;
				ITexture *pFlashlightDepthTexture;
				FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
				bFlashlightShadows = state.m_bEnableShadows && ( pFlashlightDepthTexture != NULL );

				if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
				{
					BindTexture( SHADER_SAMPLER8, pFlashlightDepthTexture, 0 );
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER6, TEXTURE_SHADOW_NOISE_2D );
				}

				SetFlashLightColorFromState( state, pShaderAPI, 28 );

				Assert( state.m_pSpotlightTexture && state.m_nSpotlightTextureFrame >= 0 );
				BindTexture( SHADER_SAMPLER7, state.m_pSpotlightTexture, state.m_nSpotlightTextureFrame );

				float atten_pos[8];
				atten_pos[0] = state.m_fConstantAtten;			// Set the flashlight attenuation factors
				atten_pos[1] = state.m_fLinearAtten;
				atten_pos[2] = state.m_fQuadraticAtten;
				atten_pos[3] = state.m_FarZ;
				atten_pos[4] = state.m_vecLightOrigin[0];			// Set the flashlight origin
				atten_pos[5] = state.m_vecLightOrigin[1];
				atten_pos[6] = state.m_vecLightOrigin[2];
				atten_pos[7] = 1.0f;
				pShaderAPI->SetPixelShaderConstant( 22, atten_pos, 2 );

				pShaderAPI->SetPixelShaderConstant( 24, worldToTexture.Base(), 4 );
			}

			//if ( pShaderAPI->InFlashlightMode() && mat_decalmodulate_noflashdraw.GetBool() )
#else
			if ( pShaderAPI->InFlashlightMode() && !IsX360() )
			{
				// Don't draw anything for the flashlight pass
				Draw( false );
				return;
			}
#endif

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// Set an identity base texture transformation
			Vector4D transformation[2];
			transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
		 	pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );					

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			// fog tweaks
			float fConsts[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fConsts[0] = params[ FOGEXPONENT ]->GetFloatValue();
			fConsts[1] = params[ FOGSCALE ]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 0, fConsts );

			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( sdk_decalmodulate_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
//				SET_DYNAMIC_VERTEX_SHADER_COMBO( TESSELLATION, 0 );             // JasonM TODO: set this appropriately when we care about decals on subds				
				SET_DYNAMIC_VERTEX_SHADER( sdk_decalmodulate_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
#ifdef MAPBASE
					SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
#endif
					SET_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps20 );
				}
			}
#ifndef _X360
			else
			{
				SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

				DECLARE_DYNAMIC_VERTEX_SHADER( sdk_decalmodulate_vs30 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
//				SET_DYNAMIC_VERTEX_SHADER_COMBO( TESSELLATION, 0 );             // JasonM TODO: set this appropriately when we care about decals on subds				
				SET_DYNAMIC_VERTEX_SHADER( sdk_decalmodulate_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
#ifdef MAPBASE
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
#endif
				SET_DYNAMIC_PIXEL_SHADER( sdk_decalmodulate_ps30 );

				bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() };
				pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
			}
#endif
		}
		Draw( );
	}
END_SHADER
