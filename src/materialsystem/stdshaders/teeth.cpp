//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

#include "teeth_vs20.inc"
#include "teeth_flashlight_vs20.inc"
#include "teeth_bump_vs20.inc"
#include "teeth_ps20.inc"
#include "teeth_ps20b.inc"
#include "teeth_flashlight_ps20.inc"
#include "teeth_flashlight_ps20b.inc"
#include "teeth_bump_ps20.inc"
#include "teeth_bump_ps20b.inc"

#ifndef _X360
#include "teeth_vs30.inc"
#include "teeth_ps30.inc"
#include "teeth_bump_vs30.inc"
#include "teeth_bump_ps30.inc"
#include "teeth_flashlight_vs30.inc"
#include "teeth_flashlight_ps30.inc"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Teeth, Teeth_DX9 )

extern ConVar r_flashlight_version2;
BEGIN_VS_SHADER( Teeth_DX9, "Help for Teeth_DX9" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ILLUMFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "Amount to darken or brighten the teeth" )
		SHADER_PARAM( FORWARD, SHADER_PARAM_TYPE_VEC3, "[1 0 0]", "Forward direction vector for teeth lighting" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
		SHADER_PARAM( PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "100", "phong exponent" )
		SHADER_PARAM( INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is teeth in the ep1 intro" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if ( g_pHardwareConfig->SupportsBorderColor() )
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
		}
		else
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
		}

		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[INTRO]->IsDefined() )
		{
			params[INTRO]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 || g_pConfig->bSoftwareLighting )
		{
			return "Teeth_dx8";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE, TEXTUREFLAGS_SRGB );
		LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );

		if( params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
	}

	void DrawUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, VertexCompressionType_t vertexCompression )
	{
		bool hasBump = params[BUMPMAP]->IsTexture();

		BlendType_t nBlendType = EvaluateBlendRequirements( BASETEXTURE, true );
		bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use
		
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );		// Base map

			int flags = VERTEX_POSITION | VERTEX_NORMAL;
			int nTexCoordCount = 1;
			int userDataSize = 0;

			if ( hasBump )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );	// Bump map
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );	// Normalization sampler for per-pixel lighting
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, false );
				userDataSize = 4;										// tangent S
			}

			// This shader supports compressed vertices, so OR in that flag:
			flags |= VERTEX_FORMAT_COMPRESSED;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			if ( hasBump )
			{
#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_STATIC_VERTEX_SHADER( teeth_bump_vs20 );
					SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
					SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
					SET_STATIC_VERTEX_SHADER( teeth_bump_vs20 );

					// ps_2_b version which does phong
					if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_STATIC_PIXEL_SHADER( teeth_bump_ps20b );
						SET_STATIC_PIXEL_SHADER( teeth_bump_ps20b );
					}
					else
					{
						DECLARE_STATIC_PIXEL_SHADER( teeth_bump_ps20 );
						SET_STATIC_PIXEL_SHADER( teeth_bump_ps20 );
					}
				}
#ifndef _X360
				else
				{
					// The vertex shader uses the vertex id stream
					SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

					DECLARE_STATIC_VERTEX_SHADER( teeth_bump_vs30 );
					SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
					SET_STATIC_VERTEX_SHADER( teeth_bump_vs30 );

					DECLARE_STATIC_PIXEL_SHADER( teeth_bump_ps30 );
					SET_STATIC_PIXEL_SHADER( teeth_bump_ps30 );
				}
#endif
			}
			else
			{
#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_STATIC_VERTEX_SHADER( teeth_vs20 );
					SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
					SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
					SET_STATIC_VERTEX_SHADER( teeth_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_STATIC_PIXEL_SHADER( teeth_ps20b );
						SET_STATIC_PIXEL_SHADER( teeth_ps20b );
					}
					else
					{
						DECLARE_STATIC_PIXEL_SHADER( teeth_ps20 );
						SET_STATIC_PIXEL_SHADER( teeth_ps20 );
					}
				}
#ifndef _X360
				else
				{
					// The vertex shader uses the vertex id stream
					SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

					DECLARE_STATIC_VERTEX_SHADER( teeth_vs30 );
					SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
					SET_STATIC_VERTEX_SHADER( teeth_vs30 );

					DECLARE_STATIC_PIXEL_SHADER( teeth_ps30 );
					SET_STATIC_PIXEL_SHADER( teeth_ps30 );
				}
#endif
			}

			// On DX9, do sRGB
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBWrite( true );

			FogToFogColor();

			pShaderShadow->EnableAlphaWrites( bFullyOpaque );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			if ( hasBump )
			{
				BindTexture( SHADER_SAMPLER1, BUMPMAP );
			}
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );
			pShaderAPI->SetPixelShaderStateAmbientLightCube( PSREG_AMBIENT_CUBE );
			pShaderAPI->CommitPixelShaderLighting( PSREG_LIGHT_INFO_ARRAY );

			Vector4D lighting;
			params[FORWARD]->GetVecValue( lighting.Base(), 3 );
			lighting[3] = params[ILLUMFACTOR]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lighting.Base() );

			LightState_t lightState;
			pShaderAPI->GetDX9LightState( &lightState );

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			if ( hasBump )
			{	
#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_DYNAMIC_VERTEX_SHADER( teeth_bump_vs20 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
					SET_DYNAMIC_VERTEX_SHADER( teeth_bump_vs20 );
		
					// ps_2_b version which does Phong
					if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						Vector4D vSpecExponent;
						vSpecExponent[3] = params[PHONGEXPONENT]->GetFloatValue();

						pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vSpecExponent.Base(), 1 );

						DECLARE_DYNAMIC_PIXEL_SHADER( teeth_bump_ps20b );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS,  lightState.m_nNumLights );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
						SET_DYNAMIC_PIXEL_SHADER( teeth_bump_ps20b );
					}
					else
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( teeth_bump_ps20 );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
						SET_DYNAMIC_PIXEL_SHADER( teeth_bump_ps20 );
					}
				}
#ifndef _X360
				else
				{
					SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

					DECLARE_DYNAMIC_VERTEX_SHADER( teeth_bump_vs30 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING,  pShaderAPI->IsHWMorphingEnabled() );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
					SET_DYNAMIC_VERTEX_SHADER( teeth_bump_vs30 );

					Vector4D vSpecExponent;
					vSpecExponent[3] = params[PHONGEXPONENT]->GetFloatValue();
					pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vSpecExponent.Base(), 1 );

					DECLARE_DYNAMIC_PIXEL_SHADER( teeth_bump_ps30 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS,  lightState.m_nNumLights );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER( teeth_bump_ps30 );
				}
#endif
			}
			else
			{
				// For non-bumped case, ambient cube is computed in the vertex shader
				SetAmbientCubeDynamicStateVertexShader();

#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_DYNAMIC_VERTEX_SHADER( teeth_vs20 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
					SET_DYNAMIC_VERTEX_SHADER( teeth_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( teeth_ps20b );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
						SET_DYNAMIC_PIXEL_SHADER( teeth_ps20b );
					}
					else
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( teeth_ps20 );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER( teeth_ps20 );
					}
				}
#ifndef _X360
				else
				{
					SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

					DECLARE_DYNAMIC_VERTEX_SHADER( teeth_vs30 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING,  pShaderAPI->IsHWMorphingEnabled() );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
					SET_DYNAMIC_VERTEX_SHADER( teeth_vs30 );

					DECLARE_DYNAMIC_PIXEL_SHADER( teeth_ps30 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER( teeth_ps30 );
				}
#endif
			}

			if( params[INTRO]->GetIntValue() )
			{
				float curTime = params[WARPPARAM]->GetFloatValue();
				float timeVec[4] = { 0.0f, 0.0f, 0.0f, curTime };
				Assert( params[ENTITYORIGIN]->IsDefined() );
				params[ENTITYORIGIN]->GetVecValue( timeVec, 3 );
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, timeVec, 1 );
			}
		}
		Draw();
	}

	void DrawFlashlight( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, VertexCompressionType_t vertexCompression )
	{
		SHADOW_STATE
		{
			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );		// Base map
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );		// Flashlight spot
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );

			// Additive blend the teeth, lit by the flashlight
			s_pShaderShadow->EnableAlphaTest( false );
			s_pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			s_pShaderShadow->EnableBlending( true );

			// Set stream format (note that this shader supports compression)
			int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			int nShadowFilterMode = 0;
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );		// shadow depth map
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER2 );
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );		// shadow noise

				nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
			}

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				DECLARE_STATIC_VERTEX_SHADER( teeth_flashlight_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
				SET_STATIC_VERTEX_SHADER( teeth_flashlight_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( teeth_flashlight_ps20b );
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
					SET_STATIC_PIXEL_SHADER( teeth_flashlight_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( teeth_flashlight_ps20 );
					SET_STATIC_PIXEL_SHADER( teeth_flashlight_ps20 );
				}
			}
#ifndef _X360
			else
			{
				// The vertex shader uses the vertex id stream
				SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

				DECLARE_STATIC_VERTEX_SHADER( teeth_flashlight_vs30 );
				SET_STATIC_VERTEX_SHADER_COMBO( INTRO, params[INTRO]->GetIntValue() ? 1 : 0 );
				SET_STATIC_VERTEX_SHADER( teeth_flashlight_vs30 );

				DECLARE_STATIC_PIXEL_SHADER( teeth_flashlight_ps30 );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
				SET_STATIC_PIXEL_SHADER( teeth_flashlight_ps30 );
			}
#endif
			// On DX9, do sRGB
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBWrite( true );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// State for spotlight projection, attenuation etc
			SetFlashlightVertexShaderConstants( false, -1, false, -1, true );

			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			SetFlashLightColorFromState( state, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			bool bFlashlightShadows = g_pHardwareConfig->SupportsPixelShaders_2_b() ? state.m_bEnableShadows && ( pFlashlightDepthTexture != NULL ) : false;
			if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
			{
				BindTexture( SHADER_SAMPLER2, pFlashlightDepthTexture, 0 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_SHADOW_NOISE_2D );
			}

			Vector4D lighting;
			params[FORWARD]->GetVecValue( lighting.Base(), 3 );
			lighting[3] = params[ILLUMFACTOR]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, lighting.Base() );

			float atten[4], pos[4], tweaks[4];

			const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );
			SetFlashLightColorFromState( flashlightState, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			BindTexture( SHADER_SAMPLER1, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

			atten[0] = flashlightState.m_fConstantAtten;		// Set the flashlight attenuation factors
			atten[1] = flashlightState.m_fLinearAtten;
			atten[2] = flashlightState.m_fQuadraticAtten;
			atten[3] = flashlightState.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

			pos[0] = flashlightState.m_vecLightOrigin[0];		// Set the flashlight origin
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1 );	// steps on rim boost

			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_TO_WORLD_TEXTURE, worldToTexture.Base(), 4 );

			// Tweaks associated with a given flashlight
			tweaks[0] = ShadowFilterFromState( flashlightState );
			tweaks[1] = ShadowAttenFromState( flashlightState );
			HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			vScreenScale[0] = (float) nWidth  / 32.0f;
			vScreenScale[1] = (float) nHeight / 32.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );

			float vFlashlightPos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vFlashlightPos );
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, vFlashlightPos, 1 );

			if ( IsX360() )
			{
				pShaderAPI->SetBooleanPixelShaderConstant( 0, &flashlightState.m_nShadowQuality, 1 );
			}

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( teeth_flashlight_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( teeth_flashlight_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
					SET_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps20 );
				}
			}
#ifndef _X360
			else
			{
				SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

				DECLARE_DYNAMIC_VERTEX_SHADER( teeth_flashlight_vs30 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING,  pShaderAPI->IsHWMorphingEnabled() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( teeth_flashlight_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
				SET_DYNAMIC_PIXEL_SHADER( teeth_flashlight_ps30 );
			}
#endif

			if( params[INTRO]->GetIntValue() )
			{
				float curTime = params[WARPPARAM]->GetFloatValue();
				float timeVec[4] = { 0.0f, 0.0f, 0.0f, curTime };
				Assert( params[ENTITYORIGIN]->IsDefined() );
				params[ENTITYORIGIN]->GetVecValue( timeVec, 3 );
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_9, timeVec, 1 );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		}
		bool hasFlashlight = UsingFlashlight( params );
		if ( !hasFlashlight || ( IsX360() || r_flashlight_version2.GetInt() ) )
		{
			DrawUsingVertexShader( params, pShaderAPI, pShaderShadow, vertexCompression );
			SHADOW_STATE
			{
				SetInitialShadowState();
			}
		}
		if( hasFlashlight )
		{
			DrawFlashlight( params, pShaderAPI, pShaderShadow, vertexCompression );
		}
	}
END_SHADER
