//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

/* Example how to plug this into an existing shader:

		In the VMT:
			// Emissive Scroll Pass
			"$emissiveBlendEnabled"      "1" // Enables effect
			"$emissiveBlendTexture"      "models/vortigaunt/vortigaunt_illum"
			"$emissiveBlendBaseTexture"  "Models/Vortigaunt/vortigaunt_blue"
			"$emissiveBlendFlowTexture"  "models/vortigaunt/vortigaunt_flow"
			"$emissiveBlendTint"         "[10 10 10]"
			"$emissiveBlendStrength"     "1.0" // Set by game code
			"$emissiveBlendScrollVector" "[0.11 0.124]"
			"Proxies"
			{
				"VortEmissive" // For setting $selfillumstrength
				{
				}
			}

		#include "emissive_scroll_blended_pass_helper.h"

		In BEGIN_SHADER_PARAMS:
			// Emissive Scroll Pass
			SHADER_PARAM( EMISSIVEBLENDENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable emissive blend pass" )
			SHADER_PARAM( EMISSIVEBLENDBASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
			SHADER_PARAM( EMISSIVEBLENDSCROLLVECTOR, SHADER_PARAM_TYPE_VEC2, "[0.11 0.124]", "Emissive scroll vec" )
			SHADER_PARAM( EMISSIVEBLENDSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive blend strength" )
			SHADER_PARAM( EMISSIVEBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
			SHADER_PARAM( EMISSIVEBLENDTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
			SHADER_PARAM( EMISSIVEBLENDFLOWTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "flow map" )

		Add this above SHADER_INIT_PARAMS()
			// Emissive Scroll Pass
			void SetupVarsEmissiveScrollBlendedPass( EmissiveScrollBlendedPassVars_t &info )
			{
				info.m_nBlendStrength = EMISSIVEBLENDSTRENGTH;
				info.m_nBaseTexture = EMISSIVEBLENDBASETEXTURE;
				info.m_nFlowTexture = EMISSIVEBLENDFLOWTEXTURE;
				info.m_nEmissiveTexture = EMISSIVEBLENDTEXTURE;
				info.m_nEmissiveTint = EMISSIVEBLENDTINT;
				info.m_nEmissiveScrollVector = EMISSIVEBLENDSCROLLVECTOR;
			}

		In SHADER_INIT_PARAMS()
			// Emissive Scroll Pass
			if ( !params[EMISSIVEBLENDENABLED]->IsDefined() )
			{
				params[EMISSIVEBLENDENABLED]->SetIntValue( 0 );
			}
			else if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
			{
				EmissiveScrollBlendedPassVars_t info;
				SetupVarsEmissiveScrollBlendedPass( info );
				InitParamsEmissiveScrollBlendedPass( this, params, pMaterialName, info );
			}

		In SHADER_INIT
			// Emissive Scroll Pass
			if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
			{
				EmissiveScrollBlendedPassVars_t info;
				SetupVarsEmissiveScrollBlendedPass( info );
				InitEmissiveScrollBlendedPass( this, params, info );
			}

		At the very end of SHADER_DRAW
			// Emissive Scroll Pass
			if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
			{
				// If ( snapshotting ) or ( we need to draw this frame )
				if ( ( pShaderShadow != NULL ) || ( params[EMISSIVEBLENDSTRENGTH]->GetFloatValue() > 0.0f ) )
				{
					EmissiveScrollBlendedPassVars_t info;
					SetupVarsEmissiveScrollBlendedPass( info );
					DrawEmissiveScrollBlendedPass( this, params, pShaderAPI, pShaderShadow, info );
				}
				else // We're not snapshotting and we don't need to draw this frame
				{
					// Skip this pass!
					Draw( false );
				}
			}

==================================================================================================== */

#include "BaseVSShader.h"
#include "mathlib/VMatrix.h"
#include "emissive_scroll_blended_pass_helper.h"
#include "convar.h"

// Auto generated inc files
#include "SDK_emissive_scroll_blended_pass_vs20.inc"
#include "SDK_emissive_scroll_blended_pass_ps20.inc"
#include "SDK_emissive_scroll_blended_pass_ps20b.inc"

#ifndef _X360
#include "SDK_emissive_scroll_blended_pass_vs30.inc"
#include "SDK_emissive_scroll_blended_pass_ps30.inc"
#endif

void InitParamsEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, EmissiveScrollBlendedPassVars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

	if ( ( info.m_nEmissiveScrollVector != -1 ) && ( !params[info.m_nEmissiveScrollVector]->IsDefined() ) )
	{
		params[info.m_nEmissiveScrollVector]->SetVecValue( kDefaultEmissiveScrollVector, 4 );
	}

	if ( ( info.m_nBlendStrength != -1 ) && ( !params[info.m_nBlendStrength]->IsDefined() ) )
	{
        params[info.m_nBlendStrength]->SetFloatValue( kDefaultEmissiveBlendStrength );
	}

	if ( ( info.m_nEmissiveTint != -1 ) && ( !params[info.m_nEmissiveTint]->IsDefined() ) )
	{
		params[info.m_nEmissiveTint]->SetVecValue( kDefaultEmissiveTint, 4 );
	}

	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );
}

void InitEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, EmissiveScrollBlendedPassVars_t &info )
{
	// Load textures
	pShader->LoadTexture( info.m_nBaseTexture );
	pShader->LoadTexture( info.m_nFlowTexture );
	pShader->LoadTexture( info.m_nEmissiveTexture );
}

void DrawEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								   IShaderShadow* pShaderShadow, EmissiveScrollBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
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

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs20 );
			SET_STATIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs20 );

			// Pixel Shader
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20b );
				SET_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20 );
				SET_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs30 );
			SET_STATIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps30 );
			SET_STATIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps30 );
		}
#endif

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false ); // Flow texture not sRGB
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBWrite( true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
		pShaderShadow->EnableAlphaWrites( false );
	}
	DYNAMIC_STATE
	{
		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs20 );

			// Set Vertex Shader Constants 
			// None?

			// Set Pixel Shader Combos
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( sdk_emissive_scroll_blended_pass_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( sdk_emissive_scroll_blended_pass_ps30 );
		}
#endif

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nFlowTexture );
		pShader->BindTexture( SHADER_SAMPLER2, info.m_nEmissiveTexture );

		// Set Pixel Shader Constants 
		//float vConstZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		// This brings in the electricity and the second base texture when the second base texture is present
		float vPsConst0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (1)
		{
			// Overall blend strength
			vPsConst0[0] = IS_PARAM_DEFINED( info.m_nBlendStrength ) ? params[info.m_nBlendStrength]->GetFloatValue() : kDefaultEmissiveBlendStrength;
			if ( vPsConst0[0] < 0.0f )
				vPsConst0[0] = 0.0f;
			if ( vPsConst0[0] > 1.0f )
				vPsConst0[0] = 1.0f;

			// Time % 1000 for scrolling
			vPsConst0[1] = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
			vPsConst0[1] -= (float)( (int)( vPsConst0[1] / 1000.0f ) ) * 1000.0f;

			// Dest alpha value for warping mask - NOTE: If we want to use this, we have to modify the blending mode above!
			//if ( ( params[info.m_nWarpParam]->GetFloatValue() > 0.0f ) && ( params[info.m_nWarpParam]->GetFloatValue() < 1.0f ) )
			//	tmpVec[2] = 1.0f;
			//else
			//	tmpVec[2] = 0.0f;
		}
		pShaderAPI->SetPixelShaderConstant( 0, vPsConst0, 1 );

		// Scroll vector
		pShaderAPI->SetPixelShaderConstant( 1, IS_PARAM_DEFINED( info.m_nEmissiveScrollVector ) ? params[info.m_nEmissiveScrollVector]->GetVecValue() : kDefaultEmissiveScrollVector, 1 );

		// Self illum tint
		pShaderAPI->SetPixelShaderConstant( 2, IS_PARAM_DEFINED( info.m_nEmissiveTint ) ? params[info.m_nEmissiveTint]->GetVecValue() : kDefaultEmissiveTint, 1 );
	}
	pShader->Draw();
}
