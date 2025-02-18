//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "cloak_blended_pass_helper.h"
#include "cpp_shader_constant_register_map.h"

#include "unlittwotexture_vs20.inc"
#include "unlittwotexture_ps20.inc"
#include "unlittwotexture_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( UnlitTwoTexture, UnlitTwoTexture_DX9 )

extern ConVar r_flashlight_version2;

BEGIN_VS_SHADER( UnlitTwoTexture_DX9, "Help for UnlitTwoTexture_DX9" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "second texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$texture2 texcoord transform" )

		// Cloak Pass
		SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90)
		{
			return "UnlitTwoTexture_DX8";
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
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		if (params[TEXTURE2]->IsDefined())
			LoadTexture( TEXTURE2, TEXTUREFLAGS_SRGB );

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

		// Skip flashlight pass for unlit stuff
		bool bNewFlashlightPath = IsX360() || ( r_flashlight_version2.GetInt() != 0 );
		if ( bDrawStandardPass && ( pShaderShadow == NULL ) && ( pShaderAPI != NULL ) &&
			!bNewFlashlightPath && ( pShaderAPI->InFlashlightMode() ) ) // not snapshotting && flashlight pass)
		{
			bDrawStandardPass = false;
		}

		// Standard rendering pass
		if ( bDrawStandardPass )
		{
			BlendType_t nBlendType = EvaluateBlendRequirements( BASETEXTURE, true );
			bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use

			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );

				s_pShaderShadow->EnableSRGBWrite( true );

				// Either we've got a constant modulation
				bool isTranslucent = IsAlphaModulating();

				// Or we've got a texture alpha on either texture
				isTranslucent = isTranslucent || TextureIsTranslucent( BASETEXTURE, true ) ||
					TextureIsTranslucent( TEXTURE2, true );

				if ( isTranslucent )
				{
					if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
					{
						EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					}
					else
					{
						EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
					}
				}
				else
				{
					if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
					{
						EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
					}
					else
					{
						DisableAlphaBlending( );
					}
				}

				// Set stream format (note that this shader supports compression)
				unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
				int nTexCoordCount = 1;
				int userDataSize = 0;
				if (IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ))
				{
					flags |= VERTEX_COLOR;
				}
				pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

				DECLARE_STATIC_VERTEX_SHADER( unlittwotexture_vs20 );
				SET_STATIC_VERTEX_SHADER( unlittwotexture_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( unlittwotexture_ps20b );
					SET_STATIC_PIXEL_SHADER( unlittwotexture_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( unlittwotexture_ps20 );
					SET_STATIC_PIXEL_SHADER( unlittwotexture_ps20 );
				}

				DefaultFog();

				pShaderShadow->EnableAlphaWrites( bFullyOpaque );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				BindTexture( SHADER_SAMPLER1, TEXTURE2, FRAME2 );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, TEXTURE2TRANSFORM );
				SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );

				pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

				float vEyePos_SpecExponent[4];
				pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
				vEyePos_SpecExponent[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

				MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
				int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
				int numBones = pShaderAPI->GetCurrentNumBones();

				DECLARE_DYNAMIC_VERTEX_SHADER( unlittwotexture_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( unlittwotexture_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( unlittwotexture_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO(	LIGHTING_PREVIEW, 
						pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) );
					SET_DYNAMIC_PIXEL_SHADER( unlittwotexture_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( unlittwotexture_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO(	LIGHTING_PREVIEW, 
						pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) );
					SET_DYNAMIC_PIXEL_SHADER( unlittwotexture_ps20 );
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
