//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "volume_clouds_helper.h"
#include "convar.h"

// Auto generated inc files
#include "volume_clouds_vs20.inc"
#include "volume_clouds_ps20.inc"
#include "volume_clouds_ps20b.inc"


void InitParamsVolumeClouds( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, VolumeCloudsVars_t &info )
{
	// Set material flags
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );

	// Set material parameter default values
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nRefractAmount, kDefaultRefractAmount );
}

void InitVolumeClouds( CBaseVSShader *pShader, IMaterialVar** params, VolumeCloudsVars_t &info )
{
	// Load textures
	if ( (info.m_nTexture1 != -1) && params[info.m_nTexture1]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nTexture1, TEXTUREFLAGS_SRGB );
	}

	if ( (info.m_nTexture2 != -1) && params[info.m_nTexture2]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nTexture2, TEXTUREFLAGS_SRGB );
	}

	if ( (info.m_nTexture3 != -1) && params[info.m_nTexture3]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nTexture3, TEXTUREFLAGS_SRGB );
	}
}

void DrawVolumeClouds( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					  IShaderShadow* pShaderShadow, VolumeCloudsVars_t &info, VertexCompressionType_t vertexCompression )
{
	SHADOW_STATE
	{
		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		// Vertex Shader
		DECLARE_STATIC_VERTEX_SHADER( volume_clouds_vs20 );
		SET_STATIC_VERTEX_SHADER( volume_clouds_vs20 );
	
		// Pixel Shader
		if( g_pHardwareConfig->SupportsPixelShaders_2_b() && !IsOpenGL() ) // Always send POSIX down the 20 path (rg - why?)
		{
			DECLARE_STATIC_PIXEL_SHADER( volume_clouds_ps20b );
			SET_STATIC_PIXEL_SHADER( volume_clouds_ps20b );
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER( volume_clouds_ps20 );
			SET_STATIC_PIXEL_SHADER( volume_clouds_ps20 );
		}

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
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
		DECLARE_DYNAMIC_VERTEX_SHADER( volume_clouds_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER( volume_clouds_vs20 );

		// Set Vertex Shader Constants 

		// Time
		float vPackedVsConst1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float flTime = IS_PARAM_DEFINED( info.m_nTime ) && params[info.m_nTime]->GetFloatValue() > 0.0f ? params[info.m_nTime]->GetFloatValue() : pShaderAPI->CurrentTime();
		float flRotateSpeed = 0.065f;
		vPackedVsConst1[0] = flTime * flRotateSpeed * 1.0f;
		vPackedVsConst1[1] = flTime * flRotateSpeed * 2.0f;
		vPackedVsConst1[2] = flTime * flRotateSpeed * 4.0f;
		vPackedVsConst1[0] -= (float)( (int)( vPackedVsConst1[0] / ( 2.0f * 3.14159f ) ) ) * 2.0f * 3.14159f;
		vPackedVsConst1[1] -= (float)( (int)( vPackedVsConst1[1] / ( 2.0f * 3.14159f ) ) ) * 2.0f * 3.14159f;
		vPackedVsConst1[2] -= (float)( (int)( vPackedVsConst1[2] / ( 2.0f * 3.14159f ) ) ) * 2.0f * 3.14159f;
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vPackedVsConst1, 1 );

		// Set Pixel Shader Combos
		if ( g_pHardwareConfig->SupportsPixelShaders_2_b() && !IsOpenGL() ) // Always send POSIX down the 20 path (rg - why?)
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( volume_clouds_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( volume_clouds_ps20b );
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( volume_clouds_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( volume_clouds_ps20 );
		}

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nTexture1 );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nTexture2 );
		pShader->BindTexture( SHADER_SAMPLER2, info.m_nTexture3 );

		// Set Pixel Shader Constants 
		float vEyePos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
		pShaderAPI->SetPixelShaderConstant( 5, vEyePos, 1 );

		float vPackedConst6[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		vPackedConst6[0] = IS_PARAM_DEFINED( info.m_nRefractAmount ) ? params[info.m_nRefractAmount]->GetFloatValue() : kDefaultRefractAmount;
		vPackedConst6[1] = vPackedVsConst1[0]; // Time % 1000
		pShaderAPI->SetPixelShaderConstant( 6, vPackedConst6, 1 );
	}
	pShader->Draw();
}
