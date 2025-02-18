//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "refract_dx9_helper.h"
#include "convar.h"
#include "Refract_vs20.inc"
#include "Refract_ps20.inc"
#include "Refract_ps20b.inc"
#include "cpp_shader_constant_register_map.h"

#define MAXBLUR 1

// FIXME: doesn't support fresnel!
void InitParamsRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, Refract_DX9_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	if( !params[info.m_nEnvmapTint]->IsDefined() )
	{
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}
	if( !params[info.m_nEnvmapContrast]->IsDefined() )
	{
		params[info.m_nEnvmapContrast]->SetFloatValue( 0.0f );
	}
	if( !params[info.m_nEnvmapSaturation]->IsDefined() )
	{
		params[info.m_nEnvmapSaturation]->SetFloatValue( 1.0f );
	}
	if( !params[info.m_nEnvmapFrame]->IsDefined() )
	{
		params[info.m_nEnvmapFrame]->SetIntValue( 0 );
	}
	if( !params[info.m_nFresnelReflection]->IsDefined() )
	{
		params[info.m_nFresnelReflection]->SetFloatValue( 1.0f );
	}
	if( !params[info.m_nMasked]->IsDefined() )
	{
		params[info.m_nMasked]->SetIntValue( 0 );
	}
	if( !params[info.m_nBlurAmount]->IsDefined() )
	{
		params[info.m_nBlurAmount]->SetIntValue( 0 );
	}
	if( !params[info.m_nFadeOutOnSilhouette]->IsDefined() )
	{
		params[info.m_nFadeOutOnSilhouette]->SetIntValue( 0 );
	}
	if( !params[info.m_nForceAlphaWrite]->IsDefined() )
	{
		params[info.m_nForceAlphaWrite]->SetIntValue( 0 );
	}
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );
}

void InitRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, Refract_DX9_Vars_t &info )
{
	if (params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture, TEXTUREFLAGS_SRGB );
	}
	if (params[info.m_nNormalMap]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nNormalMap );
	}
	if (params[info.m_nNormalMap2]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nNormalMap2 );
	}
	if( params[info.m_nEnvmap]->IsDefined() )
	{
		pShader->LoadCubeMap( info.m_nEnvmap, TEXTUREFLAGS_SRGB  );
	}
	if( params[info.m_nRefractTintTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nRefractTintTexture, TEXTUREFLAGS_SRGB  );
	}
}

void DrawRefract_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					 IShaderShadow* pShaderShadow, Refract_DX9_Vars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
	bool bHasEnvmap = params[info.m_nEnvmap]->IsTexture();
	bool bRefractTintTexture = params[info.m_nRefractTintTexture]->IsTexture();
	bool bFadeOutOnSilhouette = params[info.m_nFadeOutOnSilhouette]->GetIntValue() != 0;
	int blurAmount = params[info.m_nBlurAmount]->GetIntValue();
	bool bMasked = (params[info.m_nMasked]->GetIntValue() != 0);
	bool bSecondaryNormal = ( ( info.m_nNormalMap2 != -1 ) && ( params[info.m_nNormalMap2]->IsTexture() ) );
	bool bColorModulate = ( ( info.m_nVertexColorModulate != -1 ) && ( params[info.m_nVertexColorModulate]->GetIntValue() ) );
	bool bWriteZ = params[info.m_nNoWriteZ]->GetIntValue() == 0;

	if( blurAmount < 0 )
	{
		blurAmount = 0;
	}
	else if( blurAmount > MAXBLUR )
	{
		blurAmount = MAXBLUR;
	}

	BlendType_t nBlendType = pShader->EvaluateBlendRequirements( BASETEXTURE, true );
	bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !IS_FLAG_SET(MATERIAL_VAR_ALPHATEST); //dest alpha is free for special use
	bFullyOpaque &= !bMasked;

	bool bTranslucentNormal = pShader->TextureIsTranslucent( info.m_nNormalMap, false );
	bFullyOpaque &= (! bTranslucentNormal );

	SHADOW_STATE
	{
		pShader->SetInitialShadowState( );

		pShaderShadow->EnableDepthWrites( bWriteZ );

		// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		// If envmap is not specified, the alpha channel is the translucency
		// (If envmap *is* specified, alpha channel is the reflection amount)
		if ( params[info.m_nNormalMap]->IsTexture() && !bHasEnvmap )
		{
			pShader->SetDefaultBlendingShadowState( info.m_nNormalMap, false );
		}

		// source render target that contains the image that we are warping.
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );

		// normal map
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		if ( bSecondaryNormal )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		}

		if( bHasEnvmap )
		{
			// envmap
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
		}
		if( bRefractTintTexture )
		{
			// refract tint texture
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );
		}

		pShaderShadow->EnableSRGBWrite( true );

		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
		int userDataSize = 0;
		int nTexCoordCount = 1;
		if( bIsModel )
		{
			userDataSize = 4;
		}
		else
		{
			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}

		if ( bColorModulate )
		{
			flags |= VERTEX_COLOR;
		}
		
		// This shader supports compressed vertices, so OR in that flag:
		flags |= VERTEX_FORMAT_COMPRESSED;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );
		
		DECLARE_STATIC_VERTEX_SHADER( refract_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
		SET_STATIC_VERTEX_SHADER_COMBO( COLORMODULATE, bColorModulate );
		SET_STATIC_VERTEX_SHADER( refract_vs20 );

		// We have to do this in the shader on R500 or Leopard
		bool bShaderSRGBConvert = IsOSX() && ( g_pHardwareConfig->FakeSRGBWrite() || !g_pHardwareConfig->CanDoSRGBReadFromRTs() );
		if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // always send OpenGL down the ps2b path
		{
			DECLARE_STATIC_PIXEL_SHADER( refract_ps20b );
			SET_STATIC_PIXEL_SHADER_COMBO( BLUR,  blurAmount );
			SET_STATIC_PIXEL_SHADER_COMBO( FADEOUTONSILHOUETTE,  bFadeOutOnSilhouette );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( REFRACTTINTTEXTURE,  bRefractTintTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( MASKED, bMasked );
			SET_STATIC_PIXEL_SHADER_COMBO( COLORMODULATE, bColorModulate );
			SET_STATIC_PIXEL_SHADER_COMBO( SECONDARY_NORMAL, bSecondaryNormal );
			SET_STATIC_PIXEL_SHADER_COMBO( NORMAL_DECODE_MODE, (int) NORMAL_DECODE_NONE );
			SET_STATIC_PIXEL_SHADER_COMBO( SHADER_SRGB_READ, bShaderSRGBConvert );
			SET_STATIC_PIXEL_SHADER( refract_ps20b );
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER( refract_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( BLUR,  blurAmount );
			SET_STATIC_PIXEL_SHADER_COMBO( FADEOUTONSILHOUETTE,  bFadeOutOnSilhouette );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( REFRACTTINTTEXTURE,  bRefractTintTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( MASKED, bMasked );
			SET_STATIC_PIXEL_SHADER_COMBO( COLORMODULATE, bColorModulate );
			SET_STATIC_PIXEL_SHADER_COMBO( SECONDARY_NORMAL, bSecondaryNormal );
			SET_STATIC_PIXEL_SHADER_COMBO( NORMAL_DECODE_MODE, (int) NORMAL_DECODE_NONE );
			SET_STATIC_PIXEL_SHADER( refract_ps20 );
		}
		pShader->DefaultFog();
		if( bMasked )
		{
			pShader->EnableAlphaBlending( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );
		}

		bool bAlphaWrites = bFullyOpaque || ( params[ info.m_nForceAlphaWrite ]->GetIntValue() != 0 );
		pShaderShadow->EnableAlphaWrites( bAlphaWrites );
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		if ( params[info.m_nBaseTexture]->IsTexture() )
		{
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nBaseTexture, info.m_nFrame );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
		}

		pShader->BindTexture( SHADER_SAMPLER3, info.m_nNormalMap, info.m_nBumpFrame );

		if ( bSecondaryNormal )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nNormalMap2, info.m_nBumpFrame2 );
		}

		if( bHasEnvmap )
		{
			pShader->BindTexture( SHADER_SAMPLER4, info.m_nEnvmap, info.m_nEnvmapFrame );
		}

		if( bRefractTintTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER5, info.m_nRefractTintTexture, info.m_nRefractTintTextureFrame );
		}

		DECLARE_DYNAMIC_VERTEX_SHADER( refract_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER( refract_vs20 );

		if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // always send Posix down the ps2b path
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( refract_ps20b );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo1( true ) );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteZ && bFullyOpaque && pShaderAPI->ShouldWriteDepthToDestAlpha() );
			SET_DYNAMIC_PIXEL_SHADER( refract_ps20b );
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( refract_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( refract_ps20 );
		}

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.m_nBumpTransform );	// 1 & 2
		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, info.m_nBumpTransform2 );	// 3 & 4

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
		vEyePos_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		pShader->SetPixelShaderConstantGammaToLinear( 0, info.m_nEnvmapTint );
		pShader->SetPixelShaderConstantGammaToLinear( 1, info.m_nRefractTint );
		pShader->SetPixelShaderConstant( 2, info.m_nEnvmapContrast );
		pShader->SetPixelShaderConstant( 3, info.m_nEnvmapSaturation );
		float c5[4] = { params[info.m_nRefractAmount]->GetFloatValue(), 
			params[info.m_nRefractAmount]->GetFloatValue(), 0.0f, 0.0f };

		// Time % 1000
		c5[3] = pShaderAPI->CurrentTime();
		c5[3] -= (float)( (int)( c5[3] / 1000.0f ) ) * 1000.0f;
		pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

		float cVs3[4] = { c5[3], 0.0f, 0.0f, 0.0f };
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, cVs3, 1 );
	}
	pShader->Draw();
}

