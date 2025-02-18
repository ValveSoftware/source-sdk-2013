//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "aftershock_helper.h"
#include "convar.h"

// Auto generated inc files
#include "aftershock_vs20.inc"
#include "aftershock_ps20.inc"
#include "aftershock_ps20b.inc"


void InitParamsAftershock( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, AftershockVars_t &info )
{
	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );

	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );

	// Set material parameter default values
	if ( ( info.m_nRefractAmount != -1 ) && ( !params[info.m_nRefractAmount]->IsDefined() ) )
	{
		params[info.m_nRefractAmount]->SetFloatValue( kDefaultRefractAmount );
	}

	if ( ( info.m_nColorTint != -1 ) && ( !params[info.m_nColorTint]->IsDefined() ) )
	{
		params[info.m_nColorTint]->SetVecValue( kDefaultColorTint[0], kDefaultColorTint[1], kDefaultColorTint[2], kDefaultColorTint[3] );
	}

	if( (info.m_nBumpFrame != -1 ) && !params[info.m_nBumpFrame]->IsDefined() )
	{
		params[info.m_nBumpFrame]->SetIntValue( 0 );
	}

	if ( ( info.m_nSilhouetteThickness != -1 ) && ( !params[info.m_nSilhouetteThickness]->IsDefined() ) )
	{
		params[info.m_nSilhouetteThickness]->SetFloatValue( kDefaultSilhouetteThickness );
	}

	if ( ( info.m_nSilhouetteColor != -1 ) && ( !params[info.m_nSilhouetteColor]->IsDefined() ) )
	{
		params[info.m_nSilhouetteColor]->SetVecValue( kDefaultSilhouetteColor[0], kDefaultSilhouetteColor[1], kDefaultSilhouetteColor[2], kDefaultSilhouetteColor[3] );
	}

	if ( ( info.m_nGroundMin != -1 ) && ( !params[info.m_nGroundMin]->IsDefined() ) )
	{
		params[info.m_nGroundMin]->SetFloatValue( kDefaultGroundMin );
	}

	if ( ( info.m_nGroundMax != -1 ) && ( !params[info.m_nGroundMax]->IsDefined() ) )
	{
		params[info.m_nGroundMax]->SetFloatValue( kDefaultGroundMax );
	}

	if ( ( info.m_nBlurAmount != -1 ) && ( !params[info.m_nBlurAmount]->IsDefined() ) )
	{
		params[info.m_nBlurAmount]->SetFloatValue( kDefaultBlurAmount );
	}

	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );
}

void InitAftershock( CBaseVSShader *pShader, IMaterialVar** params, AftershockVars_t &info )
{
	// Load textures
	if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBumpmap );
	}
}

void DrawAftershock( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					IShaderShadow* pShaderShadow, AftershockVars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bBumpMapping = ( info.m_nBumpmap == -1 ) || !params[info.m_nBumpmap]->IsTexture() ? 0 : 1;

	SHADOW_STATE
	{
		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		// Vertex Shader
		DECLARE_STATIC_VERTEX_SHADER( aftershock_vs20 );
		SET_STATIC_VERTEX_SHADER( aftershock_vs20 );
	
		// Pixel Shader
		if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			DECLARE_STATIC_PIXEL_SHADER( aftershock_ps20b );
			SET_STATIC_PIXEL_SHADER( aftershock_ps20b );
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER( aftershock_ps20 );
			SET_STATIC_PIXEL_SHADER( aftershock_ps20 );
		}

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true ); // Refraction texture
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true ); // Bump
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false ); // Not sRGB
		pShaderShadow->EnableSRGBWrite( true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		pShaderShadow->EnableAlphaWrites( false );

		// !!! We need to turn this back on because EnableAlphaBlending() above disables it!
		//pShaderShadow->EnableDepthWrites( true );
	}
	DYNAMIC_STATE
	{
		// Set Vertex Shader Combos
		DECLARE_DYNAMIC_VERTEX_SHADER( aftershock_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER( aftershock_vs20 );

		// Set Vertex Shader Constants 
		if ( info.m_nBumpTransform != -1 )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.m_nBumpTransform );
		}

		// Time % 1000
		float vPackedVsConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float flTime = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
		vPackedVsConst1[0] = flTime;
		vPackedVsConst1[0] -= (float)( (int)( vPackedVsConst1[0] / 1000.0f ) ) * 1000.0f;
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vPackedVsConst1, 1 );

		// Set Pixel Shader Combos
		if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( aftershock_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( aftershock_ps20b );
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( aftershock_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( aftershock_ps20 );
		}

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
		vPackedConst1[0] = IS_PARAM_DEFINED( info.m_nBlurAmount ) ? params[info.m_nBlurAmount]->GetFloatValue() : kDefaultBlurAmount;
		vPackedConst1[1] = IS_PARAM_DEFINED( info.m_nRefractAmount ) ? params[info.m_nRefractAmount]->GetFloatValue() : kDefaultRefractAmount;
		vPackedConst1[3] = vPackedVsConst1[0]; // Time
		pShaderAPI->SetPixelShaderConstant( 6, vPackedConst1, 1 );

		// Refract color tint
		pShaderAPI->SetPixelShaderConstant( 7, IS_PARAM_DEFINED( info.m_nColorTint ) ? params[info.m_nColorTint]->GetVecValue() : kDefaultColorTint, 1 );

		// Silhouette values
		float vPackedConst8[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst8[0] = IS_PARAM_DEFINED( info.m_nSilhouetteColor ) ? params[info.m_nSilhouetteColor]->GetVecValue()[0] : kDefaultSilhouetteColor[0];
		vPackedConst8[1] = IS_PARAM_DEFINED( info.m_nSilhouetteColor ) ? params[info.m_nSilhouetteColor]->GetVecValue()[1] : kDefaultSilhouetteColor[1];
		vPackedConst8[2] = IS_PARAM_DEFINED( info.m_nSilhouetteColor ) ? params[info.m_nSilhouetteColor]->GetVecValue()[2] : kDefaultSilhouetteColor[2];
		vPackedConst8[3] = IS_PARAM_DEFINED( info.m_nSilhouetteThickness ) ? params[info.m_nSilhouetteThickness]->GetFloatValue() : kDefaultSilhouetteThickness;
		pShaderAPI->SetPixelShaderConstant( 8, vPackedConst8, 1 );

		// Ground min/max
		float vPackedConst9[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst9[0] = IS_PARAM_DEFINED( info.m_nGroundMin ) ? params[info.m_nGroundMin]->GetFloatValue() : kDefaultGroundMin;
		vPackedConst9[1] = IS_PARAM_DEFINED( info.m_nGroundMax ) ? params[info.m_nGroundMax]->GetFloatValue() : kDefaultGroundMax;
		pShaderAPI->SetPixelShaderConstant( 9, vPackedConst9, 1 );

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
