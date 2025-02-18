//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "BaseVSShader.h"
#include "unlitgeneric_vs11.inc"
#include "cloak_blended_pass_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Modulate, Modulate_DX8 )

BEGIN_VS_SHADER( Modulate_DX8,
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
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
			return "Modulate_DX6";
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
			LoadTexture( BASETEXTURE );

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
				// There is some strangeness in DX8 when trying to skip the main pass, so leave this alone for now
				//bDrawStandardPass = false;
			}
		}

		// Standard rendering pass
		if ( bDrawStandardPass )
		{
			bool bMod2X = params[MOD2X]->IsDefined() && params[MOD2X]->GetIntValue();
			bool bVertexColorOrAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) || IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );

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

				if (params[WRITEZ]->GetIntValue() != 0)
				{
					// This overrides the disabling of depth writes performed in
					// EnableAlphaBlending
					pShaderShadow->EnableDepthWrites( true );
				}

				unsigned int flags = VERTEX_POSITION;
				int numTexCoords = 1;

				if( params[BASETEXTURE]->IsTexture() )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				}

				if( bVertexColorOrAlpha )
				{
					flags |= VERTEX_COLOR;
				}

				pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, NULL, 0 );

				unlitgeneric_vs11_Static_Index vshIndex;
				vshIndex.SetDETAIL( false );
				vshIndex.SetENVMAP( false );
				vshIndex.SetENVMAPCAMERASPACE( false );
				vshIndex.SetENVMAPSPHERE( false );
				vshIndex.SetSEPARATEDETAILUVS( false );
				vshIndex.SetVERTEXCOLOR( bVertexColorOrAlpha );
				
				pShaderShadow->SetVertexShader( "unlitgeneric_vs11", vshIndex.GetIndex() );
				pShaderShadow->SetPixelShader( "Modulate_ps11" );

				// We need to fog to *white* regardless of overbrighting...
				if( bMod2X )
				{
					FogToGrey();
				}
				else
				{
					FogToOOOverbright();
				}
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
				unlitgeneric_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			Draw( );
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
