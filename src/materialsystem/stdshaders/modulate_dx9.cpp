//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "BaseVSShader.h"

#include "unlitgeneric_vs20.inc"
#include "modulate_ps20.inc"
#include "modulate_ps20b.inc"

#include "cpp_shader_constant_register_map.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "cloak_blended_pass_helper.h"

DEFINE_FALLBACK_SHADER( Modulate, Modulate_DX9 )

BEGIN_VS_SHADER( Modulate_DX9,
			  "Help for Modulate" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( WRITEZ, SHADER_PARAM_TYPE_BOOL, "0", "Forces z to be written if set" )
		SHADER_PARAM( MOD2X, SHADER_PARAM_TYPE_BOOL, "0", "forces a 2x modulate so that you can brighten and darken things" )

		// Cloak Pass
		SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( !(g_pHardwareConfig->SupportsPixelShaders_2_0() && g_pHardwareConfig->SupportsVertexShaders_2_0()) ||
			(g_pHardwareConfig->GetDXSupportLevel() < 90) )
		{
			return "Modulate_DX8";
		}

		return 0;
	}

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

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

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
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );
		}

		// Cloak Pass
		if ( params[CLOAKPASSENABLED]->GetIntValue() )
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			InitCloakBlendedPass( this, params, info );
		}
	}

	SHADER_DRAW
	{
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
			bool bMod2X = params[MOD2X]->IsDefined() && params[MOD2X]->GetIntValue();
			bool bVertexColorOrAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) || IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
			bool bWriteZ = params[WRITEZ]->GetIntValue() != 0;
			BlendType_t nBlendType = EvaluateBlendRequirements( BASETEXTURE, true );
			bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use

			SHADOW_STATE
			{
				if( bMod2X )
				{
					EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
				}

				if ( bWriteZ )
				{
					// This overrides the disabling of depth writes performed in
					// EnableAlphaBlending
					pShaderShadow->EnableDepthWrites( true );
				}

				unsigned int flags = VERTEX_POSITION;
				int numTexCoords = 0;
				int userDataSize = 0;

				if( params[BASETEXTURE]->IsTexture() )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			
					numTexCoords = 1;
				}

				if( bVertexColorOrAlpha )
				{
					flags |= VERTEX_COLOR;
				}

				// HACK:  add 1 texcoord if these verts are too thin (to do with how we
				//        bind stream 2 - see CShaderShadowDX8::VertexShaderVertexFormat)
				// FIXME: instead of this, don't add stream 2 elements to all vertex decls!
				if ( !( flags & VERTEX_COLOR ) && ( numTexCoords == 0 ) )
				{
					numTexCoords = 1;
				}

				// This shader supports compressed vertices, so OR in that flag:
				flags |= VERTEX_FORMAT_COMPRESSED;

				pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, NULL, userDataSize );

				DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, bVertexColorOrAlpha ? 1 : 0  );
				SET_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( modulate_ps20b );
					SET_STATIC_PIXEL_SHADER( modulate_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( modulate_ps20 );
					SET_STATIC_PIXEL_SHADER( modulate_ps20 );
				}

				// We need to fog to *white* regardless of overbrighting...
				if( bMod2X )
				{
					FogToGrey();
				}
				else
				{
					FogToOOOverbright();
				}

				pShaderShadow->EnableAlphaWrites( bWriteZ && bFullyOpaque );
			}
			DYNAMIC_STATE
			{
				if( params[BASETEXTURE]->IsTexture() )
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
					SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
				}

				// set constant color for modulation
				SetModulationVertexShaderDynamicState();

				// We need to fog to *white* regardless of overbrighting...
				if( bMod2X )
				{
					float grey[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
					pShaderAPI->SetPixelShaderConstant( 0, grey );
				}
				else
				{
					float white[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
					pShaderAPI->SetPixelShaderConstant( 0, white );
				}

				pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

				float vEyePos_SpecExponent[4];
				pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
				vEyePos_SpecExponent[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

				float vVertexColor[4] = { bVertexColorOrAlpha ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, vVertexColor, 1 );

				DECLARE_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( modulate_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteZ && bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER( modulate_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( modulate_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER( modulate_ps20 );
				}
			}
			Draw();
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
	}
END_SHADER
