//========= Copyright Valve Corporation, All rights reserved. ============//

/* Example how to plug this into an existing shader:

		In the VMT:
			// Cloak Pass
			"$cloakPassEnabled" "1"

		#include "cloak_blended_pass_helper.h"
 
		In BEGIN_SHADER_PARAMS:
			// Cloak Pass
			SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
			SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
			SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
			SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )

		Add this above SHADER_INIT_PARAMS()
			// Cloak Pass
			void SetupVarsCloakBlendedPass( CloakBlendedPassVars_t &info )
			{
				info.m_nCloakFactor = CLOAKFACTOR;
				info.m_nCloakColorTint = CLOAKCOLORTINT;
				info.m_nRefractAmount = REFRACTAMOUNT;
			}

			bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const 
			{ 
				if ( params[CLOAKPASSENABLED]->GetIntValue() ) // If material supports cloaking
				{
					if ( bCheckSpecificToThisFrame == false ) // For setting model flag at load time
						return true;
					else if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
						return true;
					// else, not cloaking this frame, so check flag2 in case the base material still needs it
				}

				// Check flag2 if not drawing cloak pass
				return IS_FLAG2_SET( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE ); 
			}

			bool IsTranslucent( IMaterialVar **params ) const
			{
				if ( params[CLOAKPASSENABLED]->GetIntValue() ) // If material supports cloaking
				{
					if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
						return true;
					// else, not cloaking this frame, so check flag in case the base material still needs it
				}

				// Check flag if not drawing cloak pass
				return IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT ); 
			}

		In SHADER_INIT_PARAMS()
			// Cloak Pass
			if ( !params[CLOAKPASSENABLED]->IsDefined() )
			{
				params[CLOAKPASSENABLED]->SetIntValue( 0 );
			}
			else if ( params[CLOAKPASSENABLED]->GetIntValue() )
			{
				CloakBlendedPassVars_t info;
				SetupVarsCloakBlendedPass( info );
				InitParamsCloakBlendedPass( this, params, pMaterialName, info );
			}

		In SHADER_INIT
			// Cloak Pass
			if ( params[CLOAKPASSENABLED]->GetIntValue() )
			{
				CloakBlendedPassVars_t info;
				SetupVarsCloakBlendedPass( info );
				InitCloakBlendedPass( this, params, info );
			}

		Modify SHADER_DRAW to look something like this:
			// Skip the standard rendering if cloak pass is fully opaque
			bool bDrawStandardPass = true;
			if ( params[CLOAKPASSENABLED]->GetIntValue() && ( pShaderShadow == NULL ) ) // && not snapshotting
			{
				CloakBlendedPassVars_t info;
				SetupVarsCloakBlendedPass( info );
				if ( CloakBlendedPassIsFullyOpaque( params, info ) )
				{
					bDrawStandardPass = false;
				}
			}

			// Standard rendering pass
			if ( bDrawStandardPass )
			{
				Eye_Refract_Vars_t info;
				SetupVarsEyeRefract( info );
				Draw_Eyes_Refract( this, params, pShaderAPI, pShaderShadow, info );
			}
			else
			{
				// Skip this pass!
				Draw( false );
			}

			// Cloak Pass
			if ( params[CLOAKPASSENABLED]->GetIntValue() )
			{
				// If ( snapshotting ) or ( we need to draw this frame )
				if ( ( pShaderShadow != NULL ) || ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) )
				{
					CloakBlendedPassVars_t info;
					SetupVarsCloakBlendedPass( info );
					DrawCloakBlendedPass( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
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
#include "cloak_blended_pass_helper.h"
#include "convar.h"

// Auto generated inc files
#include "cloak_blended_pass_dx8_vs11.inc"

void InitParamsCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, CloakBlendedPassVars_t &info )
{
	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS( MATERIAL_VAR_MODEL );
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );

	// Set material parameter default values
	if ( ( info.m_nCloakFactor != -1 ) && ( !params[info.m_nCloakFactor]->IsDefined() ) )
	{
		params[info.m_nCloakFactor]->SetFloatValue( kDefaultCloakFactor );
	}

	if ( ( info.m_nRefractAmount != -1 ) && ( !params[info.m_nRefractAmount]->IsDefined() ) )
	{
		params[info.m_nRefractAmount]->SetFloatValue( kDefaultRefractAmount );
	}

	if ( ( info.m_nCloakColorTint != -1 ) && ( !params[info.m_nCloakColorTint]->IsDefined() ) )
	{
		params[info.m_nCloakColorTint]->SetVecValue( kDefaultCloakColorTint[0], kDefaultCloakColorTint[1], kDefaultCloakColorTint[2], kDefaultCloakColorTint[3] );
	}
}

void InitCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, CloakBlendedPassVars_t &info )
{
	// No textures
}

void DrawCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						  IShaderShadow* pShaderShadow, CloakBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
{
	SHADOW_STATE
	{
		// Reset shadow state manually since we're drawing from two materials
		pShader->SetInitialShadowState( );

		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		// Vertex Shader
		cloak_blended_pass_dx8_vs11_Static_Index vshIndex;
		pShaderShadow->SetVertexShader( "cloak_blended_pass_dx8_vs11", vshIndex.GetIndex() );

		// Pixel Shader
		pShaderShadow->SetPixelShader( "cloak_blended_pass_dx8_ps11", 0 );

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true ); // Renderable texture for refraction

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		pShaderShadow->EnableAlphaWrites( false );

		// !!! We need to turn this back on because EnableAlphaBlending() above disables it!
		pShaderShadow->EnableDepthWrites( true );
	}
	DYNAMIC_STATE
	{
		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

		// Set Vertex Shader Combos
		cloak_blended_pass_dx8_vs11_Dynamic_Index vshIndex;
		vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
		pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

		// Set Vertex Shader Constants 
		float vVsConst3[4] = { 1.35f, 0.0f, 0.4f, ( 1.0f / ( 0.425f - 0.4f ) ) };
		vVsConst3[1] = clamp( IS_PARAM_DEFINED( info.m_nCloakFactor ) ? params[info.m_nCloakFactor]->GetFloatValue() : kDefaultCloakFactor, 0.0f, 1.0f );

		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vVsConst3 );

		float vVsConst4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vVsConst4[0] = params[info.m_nRefractAmount]->GetFloatValue() * ( 1.0f - clamp( params[info.m_nCloakFactor]->GetFloatValue(), 0.0f, 1.0f ) );
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, vVsConst4 );

		float vVsConst5[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, vVsConst5, 1 );

		// Bind textures
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 ); // Refraction Map

		// Set Pixel Shader Constants 

		// Refract color tint
		float vPsConst0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float fColorTintStrength = clamp( ( clamp( IS_PARAM_DEFINED( info.m_nCloakFactor ) ? params[info.m_nCloakFactor]->GetFloatValue() : kDefaultCloakFactor, 0.0f, 1.0f ) - 0.75f ) * 4.0f, 0.0f, 1.0f );
		vPsConst0[0] = IS_PARAM_DEFINED( info.m_nCloakColorTint ) ? params[info.m_nCloakColorTint]->GetVecValue()[0] : kDefaultCloakColorTint[0];
		vPsConst0[1] = IS_PARAM_DEFINED( info.m_nCloakColorTint ) ? params[info.m_nCloakColorTint]->GetVecValue()[1] : kDefaultCloakColorTint[1];
		vPsConst0[2] = IS_PARAM_DEFINED( info.m_nCloakColorTint ) ? params[info.m_nCloakColorTint]->GetVecValue()[2] : kDefaultCloakColorTint[2];

		vPsConst0[0] = SrgbLinearToGamma( vPsConst0[0] );
		vPsConst0[1] = SrgbLinearToGamma( vPsConst0[1] );
		vPsConst0[2] = SrgbLinearToGamma( vPsConst0[2] );

		vPsConst0[0] = ( vPsConst0[0] * ( 1.0f - fColorTintStrength ) ) + ( fColorTintStrength );
		vPsConst0[1] = ( vPsConst0[1] * ( 1.0f - fColorTintStrength ) ) + ( fColorTintStrength );
		vPsConst0[2] = ( vPsConst0[2] * ( 1.0f - fColorTintStrength ) ) + ( fColorTintStrength );

		pShaderAPI->SetPixelShaderConstant( 0, vPsConst0, 1 );
	}
	pShader->Draw();
}

bool CloakBlendedPassIsFullyOpaque ( IMaterialVar** params, CloakBlendedPassVars_t &info )
{
	float flCloakFactor = IS_PARAM_DEFINED( info.m_nCloakFactor ) ? params[info.m_nCloakFactor]->GetFloatValue() : kDefaultCloakFactor;
	//float flRefractAmount = IS_PARAM_DEFINED( info.m_nRefractAmount ) ? params[info.m_nRefractAmount]->GetFloatValue() : kDefaultRefractAmount;

	// NOTE: If this math changes, you need to update the pixel shader code!
	float flFresnel = 1.0f - ( 0.0f ); // Assume V.N = 0.0f;
	float flCloakLerpFactor = clamp( Lerp( clamp( flCloakFactor, 0.0f, 1.0f ), 1.0f, flFresnel - 1.35f ), 0.0f, 1.0f );
	//flCloakLerpFactor = 1.0f - smoothstep( 0.4f, 0.425f, flCloakLerpFactor );

	if ( flCloakLerpFactor <= 0.4f )
		return true;

	return false;
}
