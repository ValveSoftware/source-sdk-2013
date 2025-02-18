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

			// This should already exist
			//SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
			//SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
			//SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )

		Add this above SHADER_INIT_PARAMS()
			// Cloak Pass
			void SetupVarsCloakBlendedPass( CloakBlendedPassVars_t &info )
			{
				info.m_nCloakFactor = CLOAKFACTOR;
				info.m_nCloakColorTint = CLOAKCOLORTINT;
				info.m_nRefractAmount = REFRACTAMOUNT;

				// Delete these lines if not bump mapping!
 				info.m_nBumpmap = BUMPMAP;
 				info.m_nBumpFrame = BUMPFRAME;
 				info.m_nBumpTransform = BUMPTRANSFORM;
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
					DrawCloakBlendedPass( this, params, pShaderAPI, pShaderShadow, info );
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
#include "cloak_blended_pass_vs20.inc"
#include "cloak_blended_pass_ps20.inc"
#include "cloak_blended_pass_ps20b.inc"

#ifndef _X360
#include "cloak_blended_pass_vs30.inc"
#include "cloak_blended_pass_ps30.inc"
#endif

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

	if( (info.m_nBumpFrame != -1 ) && !params[info.m_nBumpFrame]->IsDefined() )
	{
		params[info.m_nBumpFrame]->SetIntValue( 0 );
	}
}

void InitCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, CloakBlendedPassVars_t &info )
{
	// Load textures
	if ( g_pConfig->UseBumpmapping() )
	{
		if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
		{
			pShader->LoadTexture( info.m_nBumpmap );
		}
	}
}

void DrawCloakBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						  IShaderShadow* pShaderShadow, CloakBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bBumpMapping = ( !g_pConfig->UseBumpmapping() ) || ( info.m_nBumpmap == -1 ) || !params[info.m_nBumpmap]->IsTexture() ? 0 : 1;

	SHADOW_STATE
	{
		// Reset shadow state manually since we're drawing from two materials
		pShader->SetInitialShadowState( );

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
			DECLARE_STATIC_VERTEX_SHADER( cloak_blended_pass_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( cloak_blended_pass_vs20 );

			// Pixel Shader
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( cloak_blended_pass_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER( cloak_blended_pass_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( cloak_blended_pass_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER( cloak_blended_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( cloak_blended_pass_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( cloak_blended_pass_vs30 );

			// Pixel Shader
			DECLARE_STATIC_PIXEL_SHADER( cloak_blended_pass_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER( cloak_blended_pass_ps30 );
		}
#endif

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true ); // Refraction texture
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		if ( bBumpMapping )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true ); // Bump
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false ); // Not sRGB
		}
		pShaderShadow->EnableSRGBWrite( true );

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

		// Set Vertex Shader Constants 
		if ( ( bBumpMapping ) && ( info.m_nBumpTransform != -1 ) )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBumpTransform );
		}

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( cloak_blended_pass_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( cloak_blended_pass_vs20 );

			// Set Pixel Shader Combos
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( cloak_blended_pass_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( cloak_blended_pass_vs30 );

			// Set Pixel Shader Combos
			DECLARE_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( cloak_blended_pass_ps30 );
		}
#endif

		// Bind textures
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 ); // Refraction Map
		if ( bBumpMapping )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nBumpmap, info.m_nBumpFrame );
		}

		// Set Pixel Shader Constants 
		float vEyePos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
		pShaderAPI->SetPixelShaderConstant( 5, vEyePos, 1 );

		float vPackedConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst1[0] = IS_PARAM_DEFINED( info.m_nCloakFactor ) ? params[info.m_nCloakFactor]->GetFloatValue() : kDefaultCloakFactor;
		vPackedConst1[1] = IS_PARAM_DEFINED( info.m_nRefractAmount ) ? params[info.m_nRefractAmount]->GetFloatValue() : kDefaultRefractAmount;
		pShaderAPI->SetPixelShaderConstant( 6, vPackedConst1, 1 );

		// Refract color tint
		pShaderAPI->SetPixelShaderConstant( 7, IS_PARAM_DEFINED( info.m_nCloakColorTint ) ? params[info.m_nCloakColorTint]->GetVecValue() : kDefaultCloakColorTint, 1 );

		// Set c0 and c1 to contain first two rows of ViewProj matrix
		VMatrix mView, mProj;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, mView.m[0] );
		pShaderAPI->GetMatrix( MATERIAL_PROJECTION, mProj.m[0] );
		VMatrix mViewProj = mView * mProj;
		mViewProj = mViewProj.Transpose3x3();
		pShaderAPI->SetPixelShaderConstant( 0, mViewProj.m[0], 2 );
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
