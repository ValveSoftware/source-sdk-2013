//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "vertexlitgeneric_dx9_helper.h"
#include "vortwarp_vs20.inc"
#include "vortwarp_ps20.inc"
#include "vortwarp_ps20b.inc"
#include "convar.h"

#ifndef _X360
#include "vortwarp_vs30.inc"
#include "vortwarp_ps30.inc"
#endif

DEFINE_FALLBACK_SHADER( VortWarp, VortWarp_dx9 )

extern ConVar r_flashlight_version2;

struct VortWarp_DX9_Vars_t : public VertexLitGeneric_DX9_Vars_t
{
	VortWarp_DX9_Vars_t() { memset( this, 0xFF, sizeof(*this) ); }
	int m_nEntityOrigin;
	int m_nWarpParam;
	int m_nFlowMap;
	int m_nSelfIllumMap;
	int m_nUnlit;
};


//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
void DrawVortWarp_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow* pShaderShadow, bool bVertexLitGeneric, bool hasFlashlight, VortWarp_DX9_Vars_t &info, VertexCompressionType_t vertexCompression )
{
	bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
	bool hasBump = (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsTexture();
	bool hasDetailTexture = !hasBump && params[info.m_nDetail]->IsTexture();
	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	bool hasVertexColor = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool hasVertexAlpha = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool hasSelfIllumInEnvMapMask =
		( info.m_nSelfIllumEnvMapMask_Alpha != -1 ) &&
		( params[info.m_nSelfIllumEnvMapMask_Alpha]->GetFloatValue() != 0.0 ) ;
	bool bHasFlowMap = ( info.m_nFlowMap != -1 ) && params[info.m_nFlowMap]->IsTexture();
	bool bHasSelfIllumMap = ( info.m_nSelfIllumMap != -1 ) && params[info.m_nSelfIllumMap]->IsTexture();
	
	BlendType_t blendType;
	if ( params[info.m_nBaseTexture]->IsTexture() )
	{
		blendType = pShader->EvaluateBlendRequirements( info.m_nBaseTexture, true );
	}
	else
	{
		blendType = pShader->EvaluateBlendRequirements( info.m_nEnvmapMask, false );
	}

	
	if( pShader->IsSnapshotting() )
	{
		// look at color and alphamod stuff.
		// Unlit generic never uses the flashlight
		bool hasEnvmap = !hasFlashlight && params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = (hasSelfIllumInEnvMapMask || !hasFlashlight) && 
			params[info.m_nEnvmapMask]->IsTexture();
		bool bHasNormal = bVertexLitGeneric || hasEnvmap;

		if( hasFlashlight )
		{
			hasEnvmapMask = false;
		}

		bool bHalfLambert = IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT );
		// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );

		if( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
		{
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue() );
		}

		if( hasFlashlight )
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetAdditiveBlendingShadowState( info.m_nBaseTexture, true );
			}
			else
			{
				pShader->SetAdditiveBlendingShadowState( info.m_nEnvmapMask, false );
			}
			if( bIsAlphaTested )
			{
				// disable alpha test and use the zfunc zequals since alpha isn't guaranteed to 
				// be the same on both the regular pass and the flashlight pass.
				pShaderShadow->EnableAlphaTest( false );
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
			}
			pShaderShadow->EnableBlending( true );
			pShaderShadow->EnableDepthWrites( false );
		}
		else
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetDefaultBlendingShadowState( info.m_nBaseTexture, true );
			}
			else
			{
				pShader->SetDefaultBlendingShadowState( info.m_nEnvmapMask, false );
			}
		}
		
		unsigned int flags = VERTEX_POSITION;
		int nTexCoordCount = 1; // texcoord0 : base texcoord
		int userDataSize = 0;
		if( bHasNormal )
		{
			flags |= VERTEX_NORMAL;
		}

		if( hasBaseTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		}
		if( hasEnvmap )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
			}
		}
		if( hasFlashlight )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			userDataSize = 4; // tangent S
		}
		if( hasDetailTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		}
		if( hasBump )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			userDataSize = 4; // tangent S
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		}
		if( hasEnvmapMask )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
		}

		if( hasVertexColor || hasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}

		pShaderShadow->EnableSRGBWrite( true );
		
		if( bHasSelfIllumMap )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );
		}

		if( bHasFlowMap )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		}

		// This shader supports compressed vertices, so OR in that flag:
		flags |= VERTEX_FORMAT_COMPRESSED;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		Assert( hasBump );
	
#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_STATIC_VERTEX_SHADER( vortwarp_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT, bHalfLambert);
			SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
			SET_STATIC_VERTEX_SHADER( vortwarp_vs20 );
			
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( vortwarp_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE,  hasBaseTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
				SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  !params[info.m_nUnlit]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
				SET_STATIC_PIXEL_SHADER_COMBO( TRANSLUCENT, blendType == BT_BLEND );
				SET_STATIC_PIXEL_SHADER( vortwarp_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( vortwarp_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE,  hasBaseTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
				SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  !params[info.m_nUnlit]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
				SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
				SET_STATIC_PIXEL_SHADER_COMBO( TRANSLUCENT, blendType == BT_BLEND );
				SET_STATIC_PIXEL_SHADER( vortwarp_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( vortwarp_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
			SET_STATIC_VERTEX_SHADER( vortwarp_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( vortwarp_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE,  hasBaseTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  hasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  !params[info.m_nUnlit]->GetIntValue() );
			SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
			SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( TRANSLUCENT, blendType == BT_BLEND );
			SET_STATIC_PIXEL_SHADER( vortwarp_ps30 );
		}
#endif

		if( hasFlashlight )
		{
			pShader->FogToBlack();
		}
		else
		{
			pShader->DefaultFog();
		}

		if( blendType == BT_BLEND )
		{
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableAlphaWrites( false );
		}
		else
		{
			pShaderShadow->EnableAlphaWrites( true );
		}
	}
	else
	{
		bool hasEnvmap = !hasFlashlight && params[info.m_nEnvmap]->IsTexture();
		bool hasEnvmapMask = !hasFlashlight && params[info.m_nEnvmapMask]->IsTexture();

		if( hasBaseTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}
		if( hasEnvmap )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nEnvmap, info.m_nEnvmapFrame );
		}
		if( hasDetailTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nDetail, info.m_nDetailFrame );
		}
		if( !g_pConfig->m_bFastNoBump )
		{
			if( hasBump )
			{
				pShader->BindTexture( SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
			}
		}
		else
		{
			if( hasBump )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
			}
		}
		if( hasEnvmapMask )
		{
			pShader->BindTexture( SHADER_SAMPLER4, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
		}

		if( hasFlashlight )
		{
			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			pShader->BindTexture( SHADER_SAMPLER7, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame );
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			SetFlashLightColorFromState( state, pShaderAPI );
		}

		// Set up light combo state
		LightState_t lightState = {0, false, false};
		if ( bVertexLitGeneric && !hasFlashlight )
		{
			pShaderAPI->GetDX9LightState( &lightState );
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		int numBones = pShaderAPI->GetCurrentNumBones();

		Assert( hasBump );

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_DYNAMIC_VERTEX_SHADER( vortwarp_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER( vortwarp_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( vortwarp_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
					blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				float warpParam = params[info.m_nWarpParam]->GetFloatValue();
		//		float selfIllumTint = params[info.m_nSelfIllumTint]->GetFloatValue();
		//		DevMsg( 1, "warpParam: %f %f\n", warpParam, selfIllumTint );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WARPINGIN, warpParam > 0.0f && warpParam < 1.0f );
				SET_DYNAMIC_PIXEL_SHADER( vortwarp_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( vortwarp_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
					blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				float warpParam = params[info.m_nWarpParam]->GetFloatValue();
		//		float selfIllumTint = params[info.m_nSelfIllumTint]->GetFloatValue();
		//		DevMsg( 1, "warpParam: %f %f\n", warpParam, selfIllumTint );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WARPINGIN, warpParam > 0.0f && warpParam < 1.0f );
				SET_DYNAMIC_PIXEL_SHADER( vortwarp_ps20 );
			}
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			DECLARE_DYNAMIC_VERTEX_SHADER( vortwarp_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( vortwarp_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( vortwarp_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
				blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			float warpParam = params[info.m_nWarpParam]->GetFloatValue();
			//		float selfIllumTint = params[info.m_nSelfIllumTint]->GetFloatValue();
			//		DevMsg( 1, "warpParam: %f %f\n", warpParam, selfIllumTint );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WARPINGIN, warpParam > 0.0f && warpParam < 1.0f );
			SET_DYNAMIC_PIXEL_SHADER( vortwarp_ps30 );
		}
#endif

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );

		if( hasDetailTexture )
		{
			pShader->SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTextureTransform, info.m_nDetailScale );
			Assert( !hasBump );
		}
		if( hasBump )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBumpTransform );
			Assert( !hasDetailTexture );
		}
		if( hasEnvmapMask )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, info.m_nEnvmapMaskTransform );
		}
		
		if( hasEnvmap )
		{
			pShader->SetEnvMapTintPixelShaderDynamicState( 0, info.m_nEnvmapTint, -1, true );
		}
		if( ( info.m_nHDRColorScale != -1 ) && pShader->IsHDREnabled() )
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( 1, params[info.m_nHDRColorScale]->GetFloatValue() );
		}
		else
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );
		}

		pShader->SetPixelShaderConstant( 2, info.m_nEnvmapContrast );
		pShader->SetPixelShaderConstant( 3, info.m_nEnvmapSaturation );

		pShader->SetPixelShaderConstant( 4, info.m_nSelfIllumTint );
		pShader->SetAmbientCubeDynamicStateVertexShader();
		if( hasBump )
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );
			pShaderAPI->SetPixelShaderStateAmbientLightCube( 5 );
            pShaderAPI->CommitPixelShaderLighting( 13 );
	
		}

		if( bHasSelfIllumMap )
		{
			pShader->BindTexture( SHADER_SAMPLER6, info.m_nSelfIllumMap, -1 );
		}

		if( bHasFlowMap )
		{
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nFlowMap, -1 );
		}

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		pShaderAPI->SetPixelShaderConstant( 20, eyePos, 1 );
		pShaderAPI->SetPixelShaderFogParams( 21 );

		// dynamic drawing code that extends vertexlitgeneric
		float curTime = params[info.m_nWarpParam]->GetFloatValue();
		float timeVec[4] = { 0.0f, 0.0f, 0.0f, curTime };
		Assert( params[info.m_nEntityOrigin]->IsDefined() );
		params[info.m_nEntityOrigin]->GetVecValue( timeVec, 3 );
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, timeVec, 1 );

		curTime = pShaderAPI->CurrentTime();
		timeVec[0] = curTime;
		timeVec[1] = curTime;
		timeVec[2] = curTime;
		timeVec[3] = curTime;
		pShaderAPI->SetPixelShaderConstant( 22, timeVec, 1 );

		// flashlightfixme: put this in common code.
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
			pShaderAPI->SetPixelShaderConstant( 22, atten, 1 );

			// Set the flashlight origin
			float pos[4];
			pos[0] = flashlightState.m_vecLightOrigin[0];
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pos[3] = 1.0f;
			pShaderAPI->SetPixelShaderConstant( 23, pos, 1 );

			pShaderAPI->SetPixelShaderConstant( 24, worldToTexture.Base(), 4 );
		}		
	}
	pShader->Draw();
}


BEGIN_VS_SHADER( VortWarp_DX9, 
				"Help for VortWarp_DX9" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALBEDO, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "albedo (Base texture with no baked lighting)" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
 	    SHADER_PARAM( SELFILLUM_ENVMAPMASK_ALPHA, SHADER_PARAM_TYPE_FLOAT,"0.0","defines that self illum value comes from env map mask alpha" )

		// Debugging term for visualizing ambient data on its own
		SHADER_PARAM( AMBIENTONLY, SHADER_PARAM_TYPE_INTEGER, "0", "Control drawing of non-ambient light ()" )
		

		// hack hack hack


 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )

		SHADER_PARAM( FLOWMAP, SHADER_PARAM_TYPE_TEXTURE, "", "flow map" )
		SHADER_PARAM( SELFILLUMMAP, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( UNLIT, SHADER_PARAM_TYPE_BOOL, "", "" )

		SHADER_PARAM( PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "5.0", "Phong exponent for local specular lights" )
		SHADER_PARAM( PHONGTINT, SHADER_PARAM_TYPE_VEC3, "5.0", "Phong tint for local specular lights" )
		SHADER_PARAM( PHONGALBEDOTINT, SHADER_PARAM_TYPE_BOOL, "1.0", "Apply tint by albedo (controlled by spec exponent texture" )
		SHADER_PARAM( LIGHTWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "1D ramp texture for tinting scalar diffuse term" )
		SHADER_PARAM( PHONGWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "warp specular term" )
		SHADER_PARAM( PHONGFRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "[0  0.5  1]", "Parameters for remapping fresnel output" )
		SHADER_PARAM( PHONGBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Phong overbrightening factor (specular mask channel should be authored to account for this)" )
		SHADER_PARAM( PHONGEXPONENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Phong Exponent map" )
		SHADER_PARAM( PHONG, SHADER_PARAM_TYPE_BOOL, "0", "enables phong lighting" )
	END_SHADER_PARAMS

	void SetupVars( VortWarp_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nAlbedo = ALBEDO;
		info.m_nSelfIllumTint = SELFILLUMTINT;
		info.m_nDetail = DETAIL;
		info.m_nDetailFrame = DETAILFRAME;
		info.m_nDetailScale = DETAILSCALE;
		info.m_nEnvmap = ENVMAP;
		info.m_nEnvmapFrame = ENVMAPFRAME;
		info.m_nEnvmapMask = ENVMAPMASK;
		info.m_nEnvmapMaskFrame = ENVMAPMASKFRAME;
		info.m_nEnvmapMaskTransform = ENVMAPMASKTRANSFORM;
		info.m_nEnvmapTint = ENVMAPTINT;
		info.m_nBumpmap = BUMPMAP;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpTransform = BUMPTRANSFORM;
		info.m_nEnvmapContrast = ENVMAPCONTRAST;
		info.m_nEnvmapSaturation = ENVMAPSATURATION;
		info.m_nAlphaTestReference = -1;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
		info.m_nSelfIllumEnvMapMask_Alpha = SELFILLUM_ENVMAPMASK_ALPHA;
		info.m_nAmbientOnly = AMBIENTONLY;
		info.m_nEntityOrigin = ENTITYORIGIN;
		info.m_nWarpParam = WARPPARAM;
		info.m_nFlowMap = FLOWMAP;
		info.m_nSelfIllumMap = SELFILLUMMAP;
		info.m_nUnlit = UNLIT;
		info.m_nPhongExponent = PHONGEXPONENT;
		info.m_nPhongExponentTexture = PHONGEXPONENTTEXTURE;
		info.m_nDiffuseWarpTexture = LIGHTWARPTEXTURE;
		info.m_nPhongWarpTexture = PHONGWARPTEXTURE;
		info.m_nPhongBoost = PHONGBOOST;
		info.m_nPhongFresnelRanges = PHONGFRESNELRANGES;
		info.m_nPhong = PHONG;

	}

	SHADER_INIT_PARAMS()
	{
		VortWarp_DX9_Vars_t vars;
		if( !params[BUMPMAP]->IsDefined() )
		{
			params[BUMPMAP]->SetStringValue( "dev/flat_normal" );
		}
		SetupVars( vars );
		if( !params[UNLIT]->IsDefined() )
		{
			params[UNLIT]->SetIntValue( 0 );
		}
		if( !params[SELFILLUMTINT]->IsDefined() )
		{
			params[SELFILLUMTINT]->SetVecValue( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		InitParamsVertexLitGeneric_DX9( this, params, pMaterialName, true, vars );
	}

	SHADER_FALLBACK
	{	
		if (g_pHardwareConfig->GetDXSupportLevel() < 90)
			return "vortwarp_DX8";

		return 0;
	}

	SHADER_INIT
	{
		VortWarp_DX9_Vars_t vars;
		SetupVars( vars );
		InitVertexLitGeneric_DX9( this, params, true, vars );
		if( params[FLOWMAP]->IsDefined() )
		{
			LoadTexture( FLOWMAP );
		}
		if( params[SELFILLUMMAP]->IsDefined() )
		{
			LoadTexture( SELFILLUMMAP );
		}
	}

	SHADER_DRAW
	{
		VortWarp_DX9_Vars_t vars;
		SetupVars( vars );
		// UGH!!!  FIXME!!!!!  Should fix VertexlitGeneric_dx9_helper so that you
		// can override the vertex shader/pixel shader used (along with the combo vars).
		bool bHasFlashlight = UsingFlashlight( params );
		if ( bHasFlashlight && ( IsX360() || r_flashlight_version2.GetInt() ) )
		{
			DrawVortWarp_DX9( this, params, pShaderAPI, pShaderShadow, true, false, vars, vertexCompression );
			SHADOW_STATE
			{
				SetInitialShadowState();
			}
		}
		DrawVortWarp_DX9( this, params, pShaderAPI, pShaderShadow, true, bHasFlashlight, vars, vertexCompression );
	}
END_SHADER
