//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"
#include "screenspaceeffect_vs20.inc"
#include "accumbuff5sample_ps20.inc"
#include "accumbuff5sample_ps20b.inc"
#include "convar.h"

BEGIN_VS_SHADER_FLAGS( accumbuff5sample, "Help for AccumBuff5Sample", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS

		// Four textures to sample
		SHADER_PARAM( TEXTURE0, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE1, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE4, SHADER_PARAM_TYPE_TEXTURE, "", "" )

		// Corresponding weights for the four input textures
		SHADER_PARAM( WEIGHTS, SHADER_PARAM_TYPE_VEC4, "", "Weight for Samples" )

	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( TEXTURE0 );
		LoadTexture( TEXTURE1 );
		LoadTexture( TEXTURE2 );
		LoadTexture( TEXTURE3 );
		LoadTexture( TEXTURE4 );
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableBlending( false );
			pShaderShadow->EnableCulling( false );
//			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_LINE );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( accumbuff5sample_ps20b );
				SET_STATIC_PIXEL_SHADER( accumbuff5sample_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( accumbuff5sample_ps20 );
				SET_STATIC_PIXEL_SHADER( accumbuff5sample_ps20 );
			}			
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, TEXTURE0, -1 );
			BindTexture( SHADER_SAMPLER1, TEXTURE1, -1 );
			BindTexture( SHADER_SAMPLER2, TEXTURE2, -1 );
			BindTexture( SHADER_SAMPLER3, TEXTURE3, -1 );
			BindTexture( SHADER_SAMPLER4, TEXTURE4, -1 );

			SetPixelShaderConstant( 0, WEIGHTS );

			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( accumbuff5sample_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( accumbuff5sample_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( accumbuff5sample_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( accumbuff5sample_ps20 );
			}
		}
		Draw();
	}
END_SHADER
