//========= Copyright Valve Corporation, All rights reserved. ============//
#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "portal_refract_dx8_helper.h"
#include "convar.h"

// Auto generated inc files
#include "portal_refract_vs11.inc"
#include "portal_refract_ps11.inc"

void InitParamsPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, PortalRefractVarsDX8_t &info )
{
	// Set material flags
	SET_FLAGS( MATERIAL_VAR_MODEL );
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );

	// Set material parameter default values
	if ( ( info.m_nStage != -1 ) && ( !params[info.m_nStage]->IsDefined() ) )
	{
		params[info.m_nStage]->SetIntValue( 0 );
	}

	if ( ( info.m_nPortalOpenAmount != -1 ) && ( !params[info.m_nPortalOpenAmount]->IsDefined() ) )
	{
		params[info.m_nPortalOpenAmount]->SetFloatValue( kDefaultPortalOpenAmount );
	}

	if ( ( info.m_nPortalStatic != -1 ) && ( !params[info.m_nPortalStatic]->IsDefined() ) )
	{
		params[info.m_nPortalStatic]->SetFloatValue( kDefaultPortalStatic );
	}
}

void InitPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, PortalRefractVarsDX8_t &info )
{
	if ( (info.m_nPortalMaskTexture != -1) && params[info.m_nPortalMaskTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nPortalMaskTexture );
	}

	if ( (info.m_nPortalColorTexture != -1) && params[info.m_nPortalColorTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nPortalColorTexture );
	}
}

void DrawPortalRefract_DX8( CBaseVSShader *pShader, IMaterialVar** params, 
						   IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, PortalRefractVarsDX8_t &info )
{
	int nStage = IS_PARAM_DEFINED( info.m_nStage ) ? params[info.m_nStage]->GetIntValue() : 0;
		
	SHADOW_STATE
	{
		// Set stream format
		pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION | VERTEX_NORMAL, 1, NULL, 4 );

		// Vertex Shader
		DECLARE_STATIC_VERTEX_SHADER( portal_refract_vs11 );
		SET_STATIC_VERTEX_SHADER_COMBO( STAGE, nStage );
		SET_STATIC_VERTEX_SHADER( portal_refract_vs11 );

		// Pixel Shader
		DECLARE_STATIC_PIXEL_SHADER( portal_refract_ps11 );
		SET_STATIC_PIXEL_SHADER_COMBO( STAGE, nStage );
		SET_STATIC_PIXEL_SHADER( portal_refract_ps11 );

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		// Enable alpha testing for all stages
		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.5f );

		// Enable alpha blending for stage 2
		if ( nStage == 2 )
		{
			pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 1.0f/255.0f );
		}

		// Disable z-writes for all passes
		pShaderShadow->EnableDepthWrites( false );

		// Disable alpha-writes for all passes
		pShaderShadow->EnableAlphaWrites( false );

		pShader->DefaultFog();
	}
	DYNAMIC_STATE
	{
		// Set Vertex Shader Combos
		DECLARE_DYNAMIC_VERTEX_SHADER( portal_refract_vs11 );
		SET_DYNAMIC_VERTEX_SHADER( portal_refract_vs11 );

		// Set Vertex Shader Constants 
		if ( IS_PARAM_DEFINED( info.m_nTextureTransform ) )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.m_nTextureTransform );
		}

		// Time % 1000
		float vPackedVsConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedVsConst1[0] = pShaderAPI->CurrentTime();
		vPackedVsConst1[0] -= (float)( floor( vPackedVsConst1[0] / 1000.0f ) ) * 1000.0f;
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vPackedVsConst1, 1 );

		// Set Pixel Shader Combos
		DECLARE_DYNAMIC_PIXEL_SHADER( portal_refract_ps11 );
		SET_DYNAMIC_PIXEL_SHADER( portal_refract_ps11 );

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nPortalColorTexture );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nPortalMaskTexture );

		// Set Pixel Shader Constants 
		pShaderAPI->SetPixelShaderFogParams( 6 );

		// Set c0-c3 to contain four rows of ViewProj matrix
		VMatrix mView, mProj;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, mView.m[0] );
		pShaderAPI->GetMatrix( MATERIAL_PROJECTION, mProj.m[0] );
		VMatrix mViewProj = mView * mProj;
		mViewProj = mViewProj.Transpose();
		pShaderAPI->SetPixelShaderConstant( 0, mViewProj.m[0], 4 );

		// Portal open amount
		float vPackedConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst1[0] = ( IS_PARAM_DEFINED( info.m_nPortalOpenAmount ) ? params[info.m_nPortalOpenAmount]->GetFloatValue() : kDefaultPortalOpenAmount );
		vPackedConst1[1] = 1.0f - ( IS_PARAM_DEFINED( info.m_nPortalStatic ) ? params[info.m_nPortalStatic]->GetFloatValue() : kDefaultPortalStatic );
		vPackedConst1[2] = ( IS_PARAM_DEFINED( info.m_nPortalColorScale ) ? params[info.m_nPortalColorScale]->GetFloatValue() : kDefaultPortalColorScale ) / 4.0f; // Will scale by 4 in ps
		
		//vPackedConst4[0] = 0.6f;
		//DevMsg( "Refract Time: %f\n", vPackedConst4[0] );

		pShaderAPI->SetPixelShaderConstant( 1, vPackedConst1, 1 );
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vPackedConst1, 1 );

		// Camera position
		float vEyePos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
		pShaderAPI->SetPixelShaderConstant( 5, vEyePos, 1 );
	}
	pShader->Draw();
}
