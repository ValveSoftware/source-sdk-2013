//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "eye_refract_helper.h"

#include "cpp_shader_constant_register_map.h"

#include "eyes_flashlight_vs11.inc"
#include "eyes_flashlight_ps11.inc"

#include "eye_refract_vs20.inc"
#include "eye_refract_ps20.inc"
#include "eye_refract_ps20b.inc"

#ifndef _X360
#include "eye_refract_vs30.inc"
#include "eye_refract_ps30.inc"
#endif

#include "convar.h"

static ConVar r_lightwarpidentity( "r_lightwarpidentity","0", FCVAR_CHEAT );

void InitParams_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, Eye_Refract_Vars_t &info )
{
	// FLASHLIGHTFIXME

	if ( g_pHardwareConfig->SupportsBorderColor() )
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
	}
	else
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
	}


	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

	// Set material parameter default values
	if ( ( info.m_nIntro >= 0 ) && ( !params[info.m_nIntro]->IsDefined() ) )
	{
		params[info.m_nIntro]->SetIntValue( kDefaultIntro );
	}

	if ( ( info.m_nDilation >= 0 ) && ( !params[info.m_nDilation]->IsDefined() ) )
	{
		params[info.m_nDilation]->SetFloatValue( kDefaultDilation );
	}

	if ( ( info.m_nGlossiness >= 0 ) && ( !params[info.m_nGlossiness]->IsDefined() ) )
	{
		params[info.m_nGlossiness]->SetFloatValue( kDefaultGlossiness );
	}

	if ( ( info.m_nSphereTexKillCombo >= 0 ) && ( !params[info.m_nSphereTexKillCombo]->IsDefined() ) )
	{
		params[info.m_nSphereTexKillCombo]->SetIntValue( kDefaultSphereTexKillCombo );
	}

	if ( ( info.m_nRaytraceSphere >= 0 ) && ( !params[info.m_nRaytraceSphere]->IsDefined() ) )
	{
		params[info.m_nRaytraceSphere]->SetIntValue( kDefaultRaytraceSphere );
	}

	if ( ( info.m_nAmbientOcclColor >= 0 ) && ( !params[info.m_nAmbientOcclColor]->IsDefined() ) )
	{
		params[info.m_nAmbientOcclColor]->SetVecValue( kDefaultAmbientOcclColor, 4 );
	}

	if ( ( info.m_nEyeballRadius >= 0 ) && ( !params[info.m_nEyeballRadius]->IsDefined() ) )
	{
		params[info.m_nEyeballRadius]->SetFloatValue( kDefaultEyeballRadius );
	}

	if ( ( info.m_nParallaxStrength >= 0 ) && ( !params[info.m_nParallaxStrength]->IsDefined() ) )
	{
		params[info.m_nParallaxStrength]->SetFloatValue( kDefaultParallaxStrength );
	}

	if ( ( info.m_nCorneaBumpStrength >= 0 ) && ( !params[info.m_nCorneaBumpStrength]->IsDefined() ) )
	{
		params[info.m_nCorneaBumpStrength]->SetFloatValue( kDefaultCorneaBumpStrength );
	}
}

void Init_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, Eye_Refract_Vars_t &info )
{
	pShader->LoadTexture( info.m_nCorneaTexture );								// SHADER_SAMPLER0  (this is a normal, hence not sRGB)
	pShader->LoadTexture( info.m_nIris, TEXTUREFLAGS_SRGB );					// SHADER_SAMPLER1
	pShader->LoadCubeMap( info.m_nEnvmap, TEXTUREFLAGS_SRGB );					// SHADER_SAMPLER2
	pShader->LoadTexture( info.m_nAmbientOcclTexture, TEXTUREFLAGS_SRGB );		// SHADER_SAMPLER3

	if ( IS_PARAM_DEFINED( info.m_nDiffuseWarpTexture ) )
	{
		pShader->LoadTexture( info.m_nDiffuseWarpTexture );						// SHADER_SAMPLER4
	}

	pShader->LoadTexture( FLASHLIGHTTEXTURE, TEXTUREFLAGS_SRGB );				// SHADER_SAMPLER5
}

void Draw_Eyes_Refract_Internal( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow* pShaderShadow, bool bDrawFlashlightAdditivePass, Eye_Refract_Vars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bDiffuseWarp = IS_PARAM_DEFINED( info.m_nDiffuseWarpTexture );
	bool bIntro = IS_PARAM_DEFINED( info.m_nIntro ) ? ( params[info.m_nIntro]->GetIntValue() ? true : false ) : false;

	SHADOW_STATE
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );	// Cornea normal
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );	// Iris
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );	// Cube reflection
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );	// Ambient occlusion

		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		if ( bDiffuseWarp )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );	// Light warp
		}

		int nShadowFilterMode = 0;
		if ( bDrawFlashlightAdditivePass == true )
		{
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
			}

			pShaderShadow->EnableDepthWrites( false );
			pShader->EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE ); // Write over the eyes that were already there 
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );	// Flashlight cookie
		}

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			DECLARE_STATIC_VERTEX_SHADER( eye_refract_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT, IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );
			SET_STATIC_VERTEX_SHADER_COMBO( INTRO, bIntro ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, bDrawFlashlightAdditivePass ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER_COMBO( LIGHTWARPTEXTURE, bDiffuseWarp ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( eye_refract_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				bool bSphereTexKillCombo = IS_PARAM_DEFINED( info.m_nSphereTexKillCombo ) ? ( params[info.m_nSphereTexKillCombo]->GetIntValue() ? true : false ) : ( kDefaultSphereTexKillCombo ? true : false );
				bool bRayTraceSphere = IS_PARAM_DEFINED( info.m_nRaytraceSphere ) ? ( params[info.m_nRaytraceSphere]->GetIntValue() ? true : false ) : ( kDefaultRaytraceSphere ? true : false );

				DECLARE_STATIC_PIXEL_SHADER( eye_refract_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( SPHERETEXKILLCOMBO, bSphereTexKillCombo ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( RAYTRACESPHERE, bRayTraceSphere ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bDrawFlashlightAdditivePass ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bDiffuseWarp ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
				SET_STATIC_PIXEL_SHADER( eye_refract_ps20b );

				if ( bDrawFlashlightAdditivePass == true )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );	// Shadow depth map
					pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER6 );
					pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	// Noise map
				}
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( eye_refract_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bDrawFlashlightAdditivePass ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bDiffuseWarp ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER( eye_refract_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( eye_refract_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT, IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );
			SET_STATIC_VERTEX_SHADER_COMBO( INTRO, bIntro ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT, bDrawFlashlightAdditivePass ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER_COMBO( LIGHTWARPTEXTURE, bDiffuseWarp ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( eye_refract_vs30 );

			bool bSphereTexKillCombo = IS_PARAM_DEFINED( info.m_nSphereTexKillCombo ) ? ( params[info.m_nSphereTexKillCombo]->GetIntValue() ? true : false ) : ( kDefaultSphereTexKillCombo ? true : false );
			bool bRayTraceSphere = IS_PARAM_DEFINED( info.m_nRaytraceSphere ) ? ( params[info.m_nRaytraceSphere]->GetIntValue() ? true : false ) : ( kDefaultRaytraceSphere ? true : false );

			DECLARE_STATIC_PIXEL_SHADER( eye_refract_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( SPHERETEXKILLCOMBO, bSphereTexKillCombo ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( RAYTRACESPHERE, bRayTraceSphere ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bDrawFlashlightAdditivePass ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bDiffuseWarp ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
			SET_STATIC_PIXEL_SHADER( eye_refract_ps30 );

			if ( bDrawFlashlightAdditivePass == true )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );	// Shadow depth map
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	// Noise map
			}
		}
#endif

		// On DX9, get the gamma read and write correct
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );			// Iris
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );			// Cube map reflection
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, true );			// Ambient occlusion
		pShaderShadow->EnableSRGBWrite( true );

		if ( bDrawFlashlightAdditivePass == true )
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );		// Flashlight cookie
		}

		// Fog
		if ( bDrawFlashlightAdditivePass == true )
		{
			pShader->FogToBlack();
		}
		else
		{
			pShader->FogToFogColor();
		}
	}
	DYNAMIC_STATE
	{
		VMatrix worldToTexture;
		ITexture *pFlashlightDepthTexture = NULL;
		FlashlightState_t flashlightState;
		bool bFlashlightShadows = false;
		if ( bDrawFlashlightAdditivePass == true )
		{
			flashlightState = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			bFlashlightShadows = flashlightState.m_bEnableShadows && ( pFlashlightDepthTexture != NULL );
		}

		pShader->BindTexture( SHADER_SAMPLER0, info.m_nCorneaTexture );				// Cornea normal
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nIris, info.m_nIrisFrame );
		pShader->BindTexture( SHADER_SAMPLER2, info.m_nEnvmap );
		pShader->BindTexture( SHADER_SAMPLER3, info.m_nAmbientOcclTexture );
	
		if ( bDiffuseWarp )
		{
			if ( r_lightwarpidentity.GetBool() )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER4, TEXTURE_IDENTITY_LIGHTWARP );
			}
			else
			{
				pShader->BindTexture( SHADER_SAMPLER4, info.m_nDiffuseWarpTexture );
			}
		}

		if ( bDrawFlashlightAdditivePass == true )
			pShader->BindTexture( SHADER_SAMPLER5, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

		pShader->SetAmbientCubeDynamicStateVertexShader();

		pShader->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nEyeOrigin );
		pShader->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nIrisU );
		pShader->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, info.m_nIrisV );

		if ( bDrawFlashlightAdditivePass == true )
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, flashlightState.m_vecLightOrigin.Base(), 1 );

		LightState_t lightState = { 0, false, false };
		if ( bDrawFlashlightAdditivePass == false )
		{
			pShaderAPI->GetDX9LightState( &lightState );
		}

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( eye_refract_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT, lightState.m_bStaticLightVertex ? 1 : 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( eye_refract_vs20 );
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0 );

			DECLARE_DYNAMIC_VERTEX_SHADER( eye_refract_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT, lightState.m_bStaticLightVertex ? 1 : 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( eye_refract_vs30 );
		}
#endif

		// Get luminance of ambient cube and saturate it
		float fAverageAmbient = max(0.0f, min( pShaderAPI->GetAmbientLightCubeLuminance(), 1.0f ) );

		// Special constant for DX9 eyes: { Dilation, Glossiness, x, x };
		float vPSConst[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPSConst[0] = IS_PARAM_DEFINED( info.m_nDilation ) ? params[info.m_nDilation]->GetFloatValue() : kDefaultDilation;
		vPSConst[1] = IS_PARAM_DEFINED( info.m_nGlossiness ) ? params[info.m_nGlossiness]->GetFloatValue() : kDefaultGlossiness;
		vPSConst[2] = fAverageAmbient;
		vPSConst[3] = IS_PARAM_DEFINED( info.m_nCorneaBumpStrength ) ? params[info.m_nCorneaBumpStrength]->GetFloatValue() : kDefaultCorneaBumpStrength;
		pShaderAPI->SetPixelShaderConstant( 0, vPSConst, 1 );

		pShaderAPI->SetPixelShaderConstant( 1, IS_PARAM_DEFINED( info.m_nEyeOrigin ) ? params[info.m_nEyeOrigin]->GetVecValue() : kDefaultEyeOrigin, 1 );
		pShaderAPI->SetPixelShaderConstant( 2, IS_PARAM_DEFINED( info.m_nIrisU ) ? params[info.m_nIrisU]->GetVecValue() : kDefaultIrisU, 1 );
		pShaderAPI->SetPixelShaderConstant( 3, IS_PARAM_DEFINED( info.m_nIrisV ) ? params[info.m_nIrisV]->GetVecValue() : kDefaultIrisV, 1 );

		float vEyePos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
		pShaderAPI->SetPixelShaderConstant( 4, vEyePos, 1 );
		pShaderAPI->SetPixelShaderConstant( 5, IS_PARAM_DEFINED( info.m_nAmbientOcclColor ) ? params[info.m_nAmbientOcclColor]->GetVecValue() : kDefaultAmbientOcclColor, 1 );

		float vPackedConst6[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//vPackedConst6[0] Unused
		vPackedConst6[1] = IS_PARAM_DEFINED( info.m_nEyeballRadius ) ? params[info.m_nEyeballRadius]->GetFloatValue() : kDefaultEyeballRadius;
		//vPackedConst6[2] = IS_PARAM_DEFINED( info.m_nRaytraceSphere ) ? params[info.m_nRaytraceSphere]->GetFloatValue() : kDefaultRaytraceSphere;
		vPackedConst6[3] = IS_PARAM_DEFINED( info.m_nParallaxStrength ) ? params[info.m_nParallaxStrength]->GetFloatValue() : kDefaultParallaxStrength;
		pShaderAPI->SetPixelShaderConstant( 6, vPackedConst6, 1 );

		float fPixelFogType = pShaderAPI->GetPixelFogCombo() == 1 ? 1 : 0;

		// Controls for lerp-style paths through shader code
		float vShaderControls[4] = { fPixelFogType, 0, 0, 0 };
		pShaderAPI->SetPixelShaderConstant( 10, vShaderControls, 1 );

		if ( bDrawFlashlightAdditivePass == true )
		{
			SetFlashLightColorFromState( flashlightState, pShaderAPI );

			if ( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows )
			{
				pShader->BindTexture( SHADER_SAMPLER6, pFlashlightDepthTexture, 0 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER7, TEXTURE_SHADOW_NOISE_2D );
			}
		}

		// Flashlight tax
#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( eye_refract_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo1( true ) );
				SET_DYNAMIC_PIXEL_SHADER( eye_refract_ps20b );
			}
			else // ps.2.0
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( eye_refract_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER( eye_refract_ps20 );
			}
		}
#ifndef _X360
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( eye_refract_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo1( true ) );
			SET_DYNAMIC_PIXEL_SHADER( eye_refract_ps30 );
		}
#endif

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		if ( bDrawFlashlightAdditivePass == true )
		{
			float atten[4], pos[4], tweaks[4];
			atten[0] = flashlightState.m_fConstantAtten;		// Set the flashlight attenuation factors
			atten[1] = flashlightState.m_fLinearAtten;
			atten[2] = flashlightState.m_fQuadraticAtten;
			atten[3] = flashlightState.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( 7, atten, 1 );

			pos[0] = flashlightState.m_vecLightOrigin[0];		// Set the flashlight origin
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pShaderAPI->SetPixelShaderConstant( 8, pos, 1 );

			//pShaderAPI->SetPixelShaderConstant( 9, worldToTexture.Base(), 4 );
			//10
			//11
			//12

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, worldToTexture[0], 1 );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, worldToTexture[1], 1 );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, worldToTexture[2], 1 );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_9, worldToTexture[3], 1 );

			// Tweaks associated with a given flashlight
			tweaks[0] = flashlightState.m_flShadowFilterSize / flashlightState.m_flShadowMapResolution;
			tweaks[1] = ShadowAttenFromState( flashlightState );
			pShader->HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( 9, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			vScreenScale[0] = (float) nWidth  / 32.0f;
			vScreenScale[1] = (float) nHeight / 32.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );
		}
		else // Lighting constants when not drawing flashlight
		{
			pShaderAPI->CommitPixelShaderLighting( PSREG_LIGHT_INFO_ARRAY );
		}

		// Intro tax
		if ( bIntro )
		{
			float curTime = params[info.m_nWarpParam]->GetFloatValue();
			float timeVec[4] = { 0.0f, 0.0f, 0.0f, curTime };
			if ( IS_PARAM_DEFINED( info.m_nEntityOrigin ) )
			{
				params[info.m_nEntityOrigin]->GetVecValue( timeVec, 3 );
			}
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, timeVec, 1 );
		}
	}
	pShader->Draw();
}


extern ConVar r_flashlight_version2;
void Draw_Eyes_Refract( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow* pShaderShadow, Eye_Refract_Vars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bHasFlashlight = pShader->UsingFlashlight( params );
	if( bHasFlashlight && ( IsX360() || r_flashlight_version2.GetInt() ) )
	{
		Draw_Eyes_Refract_Internal( pShader, params, pShaderAPI, pShaderShadow, false, info, vertexCompression );
		if ( pShaderShadow )
		{
			pShader->SetInitialShadowState( );
		}
	}
	Draw_Eyes_Refract_Internal( pShader, params, pShaderAPI, pShaderShadow, bHasFlashlight, info, vertexCompression );
}
