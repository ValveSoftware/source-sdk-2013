//========= Copyright Valve Corporation, All rights reserved. ============//

/* Example how to plug this into an existing shader:

		In the VMT:
			// Flesh Interior Pass
			"$FleshInteriorEnabled"      "1" // Enables effect
			"$FleshInteriorTexture"      "models/Alyx/alyx_flesh_color" // Mask in alpha
			"$FleshNormalTexture"		 "models/Alyx/alyx_flesh_normal"
			"$FleshBorderTexture1D"      "models/Alyx/alyx_flesh_border"
			"$FleshInteriorNoiseTexture" "Engine/noise-blur-256x256"
			"$FleshSubsurfaceTexture"	 "models/Alyx/alyx_flesh_subsurface"
			"$FleshBorderNoiseScale"     "1.5" // Flesh Noise UV scalar for border
			"$FleshBorderWidth"			 "0.3" // Width of flesh border
			"$FleshBorderSoftness"		 "0.42" // Border softness must be greater than 0.0 and up tp 0.5
			"$FleshBorderTint"			 "[1 1 1]" // Tint / brighten the border 1D texture
			"$FleshGlossBrightness"		 "0.66" // Change the brightness of the glossy layer
			"$FleshDebugForceFleshOn"	 "0" // DEBUG: This will force on full flesh for testing
			"$FleshScrollSpeed"			 "1.0"
			"Proxies"
			{
				"FleshInterior"
				{
				}
			}

		#include "flesh_interior_blended_pass_helper.h"

		In BEGIN_SHADER_PARAMS:
			// Flesh Interior Pass
			SHADER_PARAM( FLESHINTERIORENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable Flesh interior blend pass" )
			SHADER_PARAM( FLESHINTERIORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh color texture" )
			SHADER_PARAM( FLESHINTERIORNOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh noise texture" )
			SHADER_PARAM( FLESHBORDERTEXTURE1D, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh border 1D texture" )
			SHADER_PARAM( FLESHNORMALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh normal texture" )
			SHADER_PARAM( FLESHSUBSURFACETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh subsurface texture" )
			SHADER_PARAM( FLESHCUBETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh cubemap texture" )
			SHADER_PARAM( FLESHBORDERNOISESCALE, SHADER_PARAM_TYPE_FLOAT, "1.5", "Flesh Noise UV scalar for border" )
			SHADER_PARAM( FLESHDEBUGFORCEFLESHON, SHADER_PARAM_TYPE_BOOL, "0", "Flesh Debug full flesh" )
			SHADER_PARAM( FLESHEFFECTCENTERRADIUS1, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
			SHADER_PARAM( FLESHEFFECTCENTERRADIUS2, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
			SHADER_PARAM( FLESHEFFECTCENTERRADIUS3, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
			SHADER_PARAM( FLESHEFFECTCENTERRADIUS4, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
			SHADER_PARAM( FLESHSUBSURFACETINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Subsurface Color" )
			SHADER_PARAM( FLESHBORDERWIDTH, SHADER_PARAM_TYPE_FLOAT, "0.3", "Flesh border" )
			SHADER_PARAM( FLESHBORDERSOFTNESS, SHADER_PARAM_TYPE_FLOAT, "0.42", "Flesh border softness (> 0.0 && <= 0.5)" )
			SHADER_PARAM( FLESHBORDERTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Flesh border Color" )
			SHADER_PARAM( FLESHGLOBALOPACITY, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh global opacity" )
			SHADER_PARAM( FLESHGLOSSBRIGHTNESS, SHADER_PARAM_TYPE_FLOAT, "0.66", "Flesh gloss brightness" )
			SHADER_PARAM( FLESHSCROLLSPEED, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh scroll speed" )

		Add this above SHADER_INIT_PARAMS()
			// Flesh Interior Pass
			void SetupVarsFleshInteriorBlendedPass( FleshInteriorBlendedPassVars_t &info )
			{
				info.m_nFleshTexture = FLESHINTERIORTEXTURE;
				info.m_nFleshNoiseTexture = FLESHINTERIORNOISETEXTURE;
				info.m_nFleshBorderTexture1D = FLESHBORDERTEXTURE1D;
				info.m_nFleshNormalTexture = FLESHNORMALTEXTURE;
				info.m_nFleshSubsurfaceTexture = FLESHSUBSURFACETEXTURE;
				info.m_nFleshCubeTexture = FLESHCUBETEXTURE;

				info.m_nflBorderNoiseScale = FLESHBORDERNOISESCALE;
				info.m_nflDebugForceFleshOn = FLESHDEBUGFORCEFLESHON;
				info.m_nvEffectCenterRadius1 = FLESHEFFECTCENTERRADIUS1;
				info.m_nvEffectCenterRadius2 = FLESHEFFECTCENTERRADIUS2;
				info.m_nvEffectCenterRadius3 = FLESHEFFECTCENTERRADIUS3;
				info.m_nvEffectCenterRadius4 = FLESHEFFECTCENTERRADIUS4;

				info.m_ncSubsurfaceTint = FLESHSUBSURFACETINT;
				info.m_nflBorderWidth = FLESHBORDERWIDTH;
				info.m_nflBorderSoftness = FLESHBORDERSOFTNESS;
				info.m_ncBorderTint = FLESHBORDERTINT;
				info.m_nflGlobalOpacity = FLESHGLOBALOPACITY;
				info.m_nflGlossBrightness = FLESHGLOSSBRIGHTNESS;
				info.m_nflScrollSpeed = FLESHSCROLLSPEED;
			}

		In SHADER_INIT_PARAMS()
			// Flesh Interior Pass
			if ( !params[FLESHINTERIORENABLED]->IsDefined() )
			{
				params[FLESHINTERIORENABLED]->SetIntValue( 0 );
			}
			else if ( params[FLESHINTERIORENABLED]->GetIntValue() )
			{
				FleshInteriorBlendedPassVars_t info;
				SetupVarsFleshInteriorBlendedPass( info );
				InitParamsFleshInteriorBlendedPass( this, params, pMaterialName, info );
			}

		In SHADER_INIT
			// Flesh Interior Pass
			if ( params[FLESHINTERIORENABLED]->GetIntValue() )
			{
				FleshInteriorBlendedPassVars_t info;
				SetupVarsFleshInteriorBlendedPass( info );
				InitFleshInteriorBlendedPass( this, params, info );
			}

		At the very end of SHADER_DRAW
			// Flesh Interior Pass
			if ( params[FLESHINTERIORENABLED]->GetIntValue() )
			{
				// If ( snapshotting ) or ( we need to draw this frame )
				if ( ( pShaderShadow != NULL ) || ( true ) )
				{
					FleshInteriorBlendedPassVars_t info;
					SetupVarsFleshInteriorBlendedPass( info );
					DrawFleshInteriorBlendedPass( this, params, pShaderAPI, pShaderShadow, info );
				}
				else // We're not snapshotting and we don't need to draw this frame
				{
					// Skip this pass!
					Draw( false );
				}
			}

==================================================================================================== */

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "convar.h"
#include "flesh_interior_blended_pass_helper.h"

// Auto generated inc files
#include "flesh_interior_blended_pass_vs20.inc"
#include "flesh_interior_blended_pass_ps20.inc"
#include "flesh_interior_blended_pass_ps20b.inc"

void InitParamsFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FleshInteriorBlendedPassVars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

	SET_PARAM_STRING_IF_NOT_DEFINED( info.m_nFleshCubeTexture, "env_cubemap" ); // Default to in-game env map
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflBorderNoiseScale, kDefaultBorderNoiseScale );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflDebugForceFleshOn, kDefaultDebugForceFleshOn );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nvEffectCenterRadius1, kDefaultEffectCenterRadius, 4 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nvEffectCenterRadius2, kDefaultEffectCenterRadius, 4 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nvEffectCenterRadius3, kDefaultEffectCenterRadius, 4 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_nvEffectCenterRadius4, kDefaultEffectCenterRadius, 4 );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_ncSubsurfaceTint, kDefaultSubsurfaceTint, 4 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflBorderWidth, kDefaultBorderWidth );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflBorderSoftness, kDefaultBorderSoftness );
	SET_PARAM_VEC_IF_NOT_DEFINED( info.m_ncBorderTint, kDefaultBorderTint, 4 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflGlobalOpacity, kDefaultGlobalOpacity );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflGlossBrightness, kDefaultGlossBrightness );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nflScrollSpeed, kDefaultScrollSpeed );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );
}

void InitFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, FleshInteriorBlendedPassVars_t &info )
{
	// Load textures
	pShader->LoadTexture( info.m_nFleshTexture, TEXTUREFLAGS_SRGB );
	pShader->LoadTexture( info.m_nFleshNoiseTexture );
	pShader->LoadTexture( info.m_nFleshBorderTexture1D, TEXTUREFLAGS_SRGB );
	pShader->LoadTexture( info.m_nFleshNormalTexture );
	pShader->LoadTexture( info.m_nFleshSubsurfaceTexture, TEXTUREFLAGS_SRGB );
	pShader->LoadCubeMap( info.m_nFleshCubeTexture, TEXTUREFLAGS_SRGB );
}

void DrawFleshInteriorBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								  IShaderShadow* pShaderShadow, FleshInteriorBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
{
	SHADOW_STATE
	{
		// Reset shadow state manually since we're drawing from two materials
		pShader->SetInitialShadowState();

		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

		// Vertex Shader
		DECLARE_STATIC_VERTEX_SHADER( flesh_interior_blended_pass_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT, IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );
		SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
		SET_STATIC_VERTEX_SHADER( flesh_interior_blended_pass_vs20 );

		// Pixel Shader
		if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			DECLARE_STATIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20b );
			SET_STATIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20b );
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20 );
			SET_STATIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20 );
		}

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false ); // Noise texture not sRGB
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false ); // Normal texture not sRGB
		pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );
		pShaderShadow->EnableSRGBWrite( true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
	}
	DYNAMIC_STATE
	{
		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

		bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

		// Set Vertex Shader Combos
		LightState_t lightState = { 0, false, false };
		pShaderAPI->GetDX9LightState( &lightState );
		DECLARE_DYNAMIC_VERTEX_SHADER( flesh_interior_blended_pass_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT, lightState.m_bStaticLightVertex ? 1 : 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
		SET_DYNAMIC_VERTEX_SHADER( flesh_interior_blended_pass_vs20 );

		// Set Vertex Shader Constants 
		pShader->SetAmbientCubeDynamicStateVertexShader();

		// Time % 1000
		float flCurrentTime = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
		flCurrentTime *= IS_PARAM_DEFINED( info.m_nflScrollSpeed ) ? params[info.m_nflScrollSpeed]->GetFloatValue() : kDefaultScrollSpeed; // This is a dirty hack, but it works well enough

		float vVsConst0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vVsConst0[0] = flCurrentTime;
		vVsConst0[0] -= (float)( (int)( vVsConst0[0] / 1000.0f ) ) * 1000.0f;

		// Noise UV scroll
		vVsConst0[1] = flCurrentTime / 100.0f;
		vVsConst0[1] -= (float)( (int)( vVsConst0[1] ) );

		// Border noise scale
		vVsConst0[2] = IS_PARAM_DEFINED( info.m_nflBorderNoiseScale ) ? params[info.m_nflBorderNoiseScale]->GetFloatValue() : kDefaultBorderNoiseScale;

		// Debug force flesh on
		vVsConst0[3] = IS_PARAM_DEFINED( info.m_nflDebugForceFleshOn ) ? params[info.m_nflDebugForceFleshOn]->GetFloatValue() : kDefaultDebugForceFleshOn;

		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vVsConst0, 1 );

		// Flesh effect centers and radii
		float vVsConst1[4] = { kDefaultEffectCenterRadius[0], kDefaultEffectCenterRadius[1], kDefaultEffectCenterRadius[2], kDefaultEffectCenterRadius[3] };
		if ( IS_PARAM_DEFINED( info.m_nvEffectCenterRadius1 ) )
		{
			params[info.m_nvEffectCenterRadius1]->GetVecValue( vVsConst1, 4 );
			if ( vVsConst1[3] < 0.001f )
				vVsConst1[3] = 0.001f;
			vVsConst1[3] = 1.0f / vVsConst1[3]; // Pass 1.0/radius so we do a mul instead of a divide in the shader
		}
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, vVsConst1, 1 );

		float vVsConst2[4] = { kDefaultEffectCenterRadius[0], kDefaultEffectCenterRadius[1], kDefaultEffectCenterRadius[2], kDefaultEffectCenterRadius[3] };
		if ( IS_PARAM_DEFINED( info.m_nvEffectCenterRadius2 ) )
		{
			params[info.m_nvEffectCenterRadius2]->GetVecValue( vVsConst2, 4 );
			if ( vVsConst2[3] < 0.001f )
				vVsConst2[3] = 0.001f;
			vVsConst2[3] = 1.0f / vVsConst2[3]; // Pass 1.0/radius so we do a mul instead of a divide in the shader
		}
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, vVsConst2, 2 );

		float vVsConst3[4] = { kDefaultEffectCenterRadius[0], kDefaultEffectCenterRadius[1], kDefaultEffectCenterRadius[2], kDefaultEffectCenterRadius[3] };
		if ( IS_PARAM_DEFINED( info.m_nvEffectCenterRadius3 ) )
		{
			params[info.m_nvEffectCenterRadius3]->GetVecValue( vVsConst3, 4 );
			if ( vVsConst3[3] < 0.001f )
				vVsConst3[3] = 0.001f;
			vVsConst3[3] = 1.0f / vVsConst3[3]; // Pass 1.0/radius so we do a mul instead of a divide in the shader
		}
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vVsConst3, 3 );

		float vVsConst4[4] = { kDefaultEffectCenterRadius[0], kDefaultEffectCenterRadius[1], kDefaultEffectCenterRadius[2], kDefaultEffectCenterRadius[3] };
		if ( IS_PARAM_DEFINED( info.m_nvEffectCenterRadius4 ) )
		{
			params[info.m_nvEffectCenterRadius4]->GetVecValue( vVsConst4, 4 );
			if ( vVsConst4[3] < 0.001f )
				vVsConst4[3] = 0.001f;
			vVsConst4[3] = 1.0f / vVsConst4[3]; // Pass 1.0/radius so we do a mul instead of a divide in the shader
		}
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, vVsConst4, 4 );

		// Set Pixel Shader Combos
		if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20b );
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( flesh_interior_blended_pass_ps20 );
		}

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nFleshTexture );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nFleshNoiseTexture );
		pShader->BindTexture( SHADER_SAMPLER2, info.m_nFleshBorderTexture1D );
		pShader->BindTexture( SHADER_SAMPLER3, info.m_nFleshNormalTexture );
		pShader->BindTexture( SHADER_SAMPLER4, info.m_nFleshSubsurfaceTexture );
		pShader->BindTexture( SHADER_SAMPLER5, info.m_nFleshCubeTexture );

		// Set Pixel Shader Constants 

		// Subsurface tint
		pShaderAPI->SetPixelShaderConstant( 0, IS_PARAM_DEFINED( info.m_ncSubsurfaceTint ) ? params[info.m_ncSubsurfaceTint]->GetVecValue() : kDefaultSubsurfaceTint, 1 );

		// Border width
		float vPsConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPsConst1[0] = IS_PARAM_DEFINED( info.m_nflBorderWidth ) ? params[info.m_nflBorderWidth]->GetFloatValue() : kDefaultBorderWidth;
		vPsConst1[0] = 1.0f / vPsConst1[0]; // ( 1.0f / g_flBorderWidthFromVmt )
		vPsConst1[1] = vPsConst1[0] - 1.0f; // ( 1.0f / g_flBorderWidthFromVmt ) - 1.0f
		pShaderAPI->SetPixelShaderConstant( 1, vPsConst1, 1 );

		// Border softness
		float vPsConst2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPsConst2[0] = IS_PARAM_DEFINED( info.m_nflBorderSoftness ) ? params[info.m_nflBorderSoftness]->GetFloatValue() : kDefaultBorderSoftness;
		if ( vPsConst2[0] < 0.01f )
			vPsConst2[0] = 0.01f;
		else if ( vPsConst2[0] > 0.5f )
			vPsConst2[0] = 0.5f;
		pShaderAPI->SetPixelShaderConstant( 2, vPsConst2, 1 );

		// Border color tint
		pShaderAPI->SetPixelShaderConstant( 3, IS_PARAM_DEFINED( info.m_ncBorderTint ) ? params[info.m_ncBorderTint]->GetVecValue() : kDefaultBorderTint, 1 );

		// Global opacity
		float vPsConst4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPsConst4[0] = IS_PARAM_DEFINED( info.m_nflGlobalOpacity ) ? params[info.m_nflGlobalOpacity]->GetFloatValue() : kDefaultGlobalOpacity;
		pShaderAPI->SetPixelShaderConstant( 4, vPsConst4, 1 );

		// Gloss brightness
		float vPsConst5[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPsConst5[0] = IS_PARAM_DEFINED( info.m_nflGlossBrightness ) ? params[info.m_nflGlossBrightness]->GetFloatValue() : kDefaultGlossBrightness;
		pShaderAPI->SetPixelShaderConstant( 5, vPsConst5, 1 );
	}
	pShader->Draw();
}
