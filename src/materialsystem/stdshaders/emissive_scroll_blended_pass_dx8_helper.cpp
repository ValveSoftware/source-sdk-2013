//========= Copyright Valve Corporation, All rights reserved. ============//

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

		Add this above SHADER_INIT_PARAMS()
			// Emissive Scroll Pass
			void SetupVarsEmissiveScrollBlendedPass( EmissiveScrollBlendedPassVars_t &info )
			{
				info.m_nBlendStrength = EMISSIVEBLENDSTRENGTH;
				info.m_nBaseTexture = EMISSIVEBLENDBASETEXTURE;
				info.m_nFlowTexture = -1; // Not used in DX8
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
#include "mathlib/vmatrix.h"
#include "emissive_scroll_blended_pass_helper.h"
#include "convar.h"

// Auto generated inc files
#include "emissive_scroll_blended_pass_dx8_vs11.inc"

void InitParamsEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, EmissiveScrollBlendedPassVars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

	if ( ( info.m_nEmissiveScrollVector >= 0 ) && ( !params[info.m_nEmissiveScrollVector]->IsDefined() ) )
	{
		params[info.m_nEmissiveScrollVector]->SetVecValue( kDefaultEmissiveScrollVector, 4 );
	}

	if ( ( info.m_nBlendStrength >= 0 ) && ( !params[info.m_nBlendStrength]->IsDefined() ) )
	{
		params[info.m_nBlendStrength]->SetFloatValue( kDefaultEmissiveBlendStrength );
	}

	if ( ( info.m_nEmissiveTint >= 0 ) && ( !params[info.m_nEmissiveTint]->IsDefined() ) )
	{
		params[info.m_nEmissiveTint]->SetVecValue( kDefaultEmissiveTint, 4 );
	}

	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );
}

void InitEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, EmissiveScrollBlendedPassVars_t &info )
{
	// Load textures
	pShader->LoadTexture( info.m_nBaseTexture );
	pShader->LoadTexture( info.m_nEmissiveTexture );
}

void DrawEmissiveScrollBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								   IShaderShadow* pShaderShadow, EmissiveScrollBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
{
	SHADOW_STATE
	{
		// Reset shadow state manually since we're drawing from two materials
		pShader->SetInitialShadowState();

		// Set stream format
		pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION | VERTEX_NORMAL, 1, 0, 0 );

		// Vertex Shader
		emissive_scroll_blended_pass_dx8_vs11_Static_Index vshIndex;
		pShaderShadow->SetVertexShader( "emissive_scroll_blended_pass_dx8_vs11", vshIndex.GetIndex() );

		// Pixel Shader
		pShaderShadow->SetPixelShader( "emissive_scroll_blended_pass_dx8_ps11", 0 );

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
		pShaderShadow->EnableAlphaWrites( false );
	}
	DYNAMIC_STATE
	{
		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

		// Set Vertex Shader Combos
		emissive_scroll_blended_pass_dx8_vs11_Dynamic_Index vshIndex;
		vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
		pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

		// Set Vertex Shader Constants 
		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );

		float vEmissiveScrollVector[4] = { kDefaultEmissiveScrollVector[0], kDefaultEmissiveScrollVector[1], 0.0f, 0.0f };
		if ( IS_PARAM_DEFINED( info.m_nEmissiveScrollVector ) )
		{
			const float *flPtr = params[info.m_nEmissiveScrollVector]->GetVecValue();
			if ( flPtr != NULL )
			{
				vEmissiveScrollVector[0] = flPtr[0];
				vEmissiveScrollVector[1] = flPtr[1];
			}
		}

		float curTime = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
		float selfIllumScroll[4] = { vEmissiveScrollVector[0] * curTime, vEmissiveScrollVector[1] * curTime, 0.0f, 0.0f };
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, selfIllumScroll, 1 );

		// Set Pixel Shader Combos
		/* None */

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nEmissiveTexture );

		// Set Pixel Shader Constants (Copied from vortwarp code)
		pShader->SetModulationPixelShaderDynamicState( 3 );
		pShader->EnablePixelShaderOverbright( 0, true, true );

		pShaderAPI->SetPixelShaderConstant( 1, IS_PARAM_DEFINED( info.m_nEmissiveTint ) ? params[info.m_nEmissiveTint]->GetVecValue() : kDefaultEmissiveTint, 1 );

		float c4[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // NOTE: w is written to dest alpha
		c4[0] = IS_PARAM_DEFINED( info.m_nBlendStrength ) ? params[info.m_nBlendStrength]->GetFloatValue() : kDefaultEmissiveBlendStrength;
		c4[1] = c4[0];
		c4[2] = c4[0];
		pShaderAPI->SetPixelShaderConstant( 4, c4, 1 );

		float c5[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );
	}
	pShader->Draw();
}
