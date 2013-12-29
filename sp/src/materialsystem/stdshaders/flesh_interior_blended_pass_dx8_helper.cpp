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
#include "flesh_interior_blended_pass_dx8_vs11.inc"

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
	pShader->LoadTexture( info.m_nFleshTexture );
	//pShader->LoadTexture( info.m_nFleshNoiseTexture );
	//pShader->LoadTexture( info.m_nFleshBorderTexture1D );
	//pShader->LoadTexture( info.m_nFleshNormalTexture );
	//pShader->LoadTexture( info.m_nFleshSubsurfaceTexture );
	//pShader->LoadCubeMap( info.m_nFleshCubeTexture );
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

		// Vertex Shader
		flesh_interior_blended_pass_dx8_vs11_Static_Index vshIndex;
		pShaderShadow->SetVertexShader( "flesh_interior_blended_pass_dx8_vs11", vshIndex.GetIndex() );

		// Pixel Shader
		pShaderShadow->SetPixelShader( "flesh_interior_blended_pass_dx8_ps11", 0 );

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
	}
	DYNAMIC_STATE
	{
		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

		// Set Vertex Shader Combos
		flesh_interior_blended_pass_dx8_vs11_Dynamic_Index vshIndex;
		vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
		pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

		// Set Vertex Shader Constants 

		// Time % 1000
		float flCurrentTime = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
		flCurrentTime *= IS_PARAM_DEFINED( info.m_nflScrollSpeed ) ? params[info.m_nflScrollSpeed]->GetFloatValue() : kDefaultScrollSpeed; // This is a dirty hack, but it works well enough

		float vVsConst0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vVsConst0[0] = flCurrentTime;
		vVsConst0[0] -= (float)( (int)( vVsConst0[0] / 1000.0f ) ) * 1000.0f;

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
		/* None */

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nFleshTexture );

		// Set Pixel Shader Constants 

		// Border color tint
		pShaderAPI->SetPixelShaderConstant( 3, IS_PARAM_DEFINED( info.m_ncBorderTint ) ? params[info.m_ncBorderTint]->GetVecValue() : kDefaultBorderTint, 1 );

		// Global opacity
		float vPsConst4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPsConst4[0] = IS_PARAM_DEFINED( info.m_nflGlobalOpacity ) ? params[info.m_nflGlobalOpacity]->GetFloatValue() : kDefaultGlobalOpacity;
		pShaderAPI->SetPixelShaderConstant( 4, vPsConst4, 1 );

		float vPsConst5[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		pShaderAPI->SetPixelShaderConstant( 5, vPsConst5, 1 );
	}
	pShader->Draw();
}
