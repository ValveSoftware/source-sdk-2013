//========= Copyright Valve Corporation, All rights reserved. ============//

/* Based heavily on cloak_blended_pass, look at cloak_blended_pass_helper.cpp

==================================================================================================== */

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "weapon_sheen_pass_helper.h"
#include "convar.h"

// Auto generated inc files
#include "weapon_sheen_pass_vs20.inc"
#include "weapon_sheen_pass_ps20.inc"
#include "weapon_sheen_pass_ps20b.inc"

#ifndef _X360
#include "weapon_sheen_pass_vs30.inc"
#include "weapon_sheen_pass_ps30.inc"
#endif

void InitParamsWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, WeaponSheenPassVars_t &info )
{
	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS( MATERIAL_VAR_MODEL );
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );

	// Set material parameter default values
	if ( ( info.m_nSheenMapMaskFrame != -1 ) && ( !params[info.m_nSheenMapMaskFrame]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskFrame]->SetFloatValue( 0 );
	}

	if ( ( info.m_nSheenMapTint != -1 ) && ( !params[info.m_nSheenMapTint]->IsDefined() ) )
	{
		params[info.m_nSheenMapTint]->SetVecValue( 1.0f, 1.0f, 1.0f, 1.0f );
	}

	if ( ( info.m_nSheenMapMaskScaleX != -1 ) && ( !params[info.m_nSheenMapMaskScaleX]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskScaleX]->SetFloatValue( 1.0f );
	}
	if ( ( info.m_nSheenMapMaskScaleY != -1 ) && ( !params[info.m_nSheenMapMaskScaleY]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskScaleY]->SetFloatValue( 1.0f );
	}

	if ( ( info.m_nSheenMapMaskOffsetX != -1 ) && ( !params[info.m_nSheenMapMaskOffsetX]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskOffsetX]->SetFloatValue( 0 );
	}
	if ( ( info.m_nSheenMapMaskOffsetY != -1 ) && ( !params[info.m_nSheenMapMaskOffsetY]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskOffsetY]->SetFloatValue( 0 );
	}

	if ( ( info.m_nSheenMapMaskDirection != -1 ) && ( !params[info.m_nSheenMapMaskDirection]->IsDefined() ) )
	{
		params[info.m_nSheenMapMaskDirection]->SetFloatValue( 0 );
	}

	if( (info.m_nSheenIndex != -1 ) && !params[info.m_nSheenIndex]->IsDefined() )
	{
		params[info.m_nSheenIndex]->SetIntValue( 0 );
	}

	if( (info.m_nBumpFrame != -1 ) && !params[info.m_nBumpFrame]->IsDefined() )
	{
		params[info.m_nBumpFrame]->SetIntValue( 0 );
	}
}

void InitWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, WeaponSheenPassVars_t &info )
{
	// Load textures
	if ( g_pConfig->UseBumpmapping() )
	{
		if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
		{
			pShader->LoadTexture( info.m_nBumpmap );
		}
	}

	if ( (info.m_nSheenMap != -1) && params[info.m_nSheenMap]->IsDefined() )
	{
		pShader->LoadCubeMap( info.m_nSheenMap, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0  );
	}

	if ( (info.m_nSheenMapMask != -1) && params[info.m_nSheenMapMask]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nSheenMapMask );
	}
	
}

void DrawWeaponSheenPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
						  IShaderShadow* pShaderShadow, WeaponSheenPassVars_t &info, VertexCompressionType_t vertexCompression )
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
			DECLARE_STATIC_VERTEX_SHADER( weapon_sheen_pass_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( weapon_sheen_pass_vs20 );

			// Pixel Shader
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
				SET_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( weapon_sheen_pass_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_VERTEX_SHADER( weapon_sheen_pass_vs30 );

			// Pixel Shader
			DECLARE_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bBumpMapping ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER( weapon_sheen_pass_ps30 );
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

		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
		}

		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, true );
		}

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
			DECLARE_DYNAMIC_VERTEX_SHADER( weapon_sheen_pass_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( weapon_sheen_pass_vs20 );

			// Set Pixel Shader Combos
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps20 );
			}
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( weapon_sheen_pass_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( weapon_sheen_pass_vs30 );

			// Set Pixel Shader Combos
			DECLARE_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( weapon_sheen_pass_ps30 );
		}
#endif

		// Bind textures
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 ); // Refraction Map
		if ( bBumpMapping )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nBumpmap, info.m_nBumpFrame );
		}

		if ( info.m_nSheenMap != -1 )
		{
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nSheenMap, -1 );
		}
		
		if ( info.m_nSheenMapMask != -1 )
		{
			pShader->BindTexture( SHADER_SAMPLER3, info.m_nSheenMapMask, info.m_nSheenMapMaskFrame );
		}

		// Set Pixel Shader Constants 
		float vEyePos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
		pShaderAPI->SetPixelShaderConstant( 5, vEyePos, 1 );

		float vPackedConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst1[0] = IS_PARAM_DEFINED( info.m_nSheenMapMaskScaleX ) ? params[info.m_nSheenMapMaskScaleX]->GetFloatValue() : 1.0f;
		vPackedConst1[1] = IS_PARAM_DEFINED( info.m_nSheenMapMaskScaleY ) ? params[info.m_nSheenMapMaskScaleY]->GetFloatValue() : 1.0f;
		vPackedConst1[2] = IS_PARAM_DEFINED( info.m_nSheenMapMaskOffsetX ) ? params[info.m_nSheenMapMaskOffsetX]->GetFloatValue() : 0.0f;
		vPackedConst1[3] = IS_PARAM_DEFINED( info.m_nSheenMapMaskOffsetY ) ? params[info.m_nSheenMapMaskOffsetY]->GetFloatValue() : 0.0f;
		pShaderAPI->SetPixelShaderConstant( 6, vPackedConst1, 1 );

		float vPackedConst2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst2[0] = IS_PARAM_DEFINED( info.m_nSheenMapMaskDirection ) ? params[info.m_nSheenMapMaskDirection]->GetFloatValue() : 0.0f;
		vPackedConst2[1] = IS_PARAM_DEFINED( info.m_nSheenIndex ) ? params[info.m_nSheenIndex]->GetFloatValue() : 0.0f;
		pShaderAPI->SetPixelShaderConstant( 7, vPackedConst2, 1 );

		// Map color tint
		pShaderAPI->SetPixelShaderConstant( 8, IS_PARAM_DEFINED( info.m_nSheenMapTint ) ? params[info.m_nSheenMapTint]->GetVecValue() : kDefaultSheenColorTint, 1 );

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

bool ShouldDrawMaterialSheen ( IMaterialVar** params, WeaponSheenPassVars_t &info )
{
	// If the frame is zero we're not rendering
	if ( IS_PARAM_DEFINED( info.m_nSheenMapMaskFrame ) && params[info.m_nSheenMapMaskFrame]->GetIntValue() > 0 )
	{
		return true;
	}
	
	return false;
}