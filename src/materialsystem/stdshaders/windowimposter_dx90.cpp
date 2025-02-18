//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

#include "windowimposter_vs20.inc"
#include "windowimposter_ps20.inc"
#include "windowimposter_ps20b.inc"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WindowImposter, WindowImposter_DX90 )

BEGIN_VS_SHADER( WindowImposter_DX90,
			  "Help for WindowImposter_DX90" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90)
			return "WindowImposter_DX80";

		return NULL;
	}

	SHADER_INIT
	{
		LoadCubeMap( ENVMAP );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
				pShaderShadow->EnableSRGBWrite( true );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			DECLARE_STATIC_VERTEX_SHADER( windowimposter_vs20 );
			SET_STATIC_VERTEX_SHADER( windowimposter_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( windowimposter_ps20b );
				SET_STATIC_PIXEL_SHADER( windowimposter_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( windowimposter_ps20 );
				SET_STATIC_PIXEL_SHADER( windowimposter_ps20 );
			}

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableDepthWrites( false );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( windowimposter_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER( windowimposter_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( windowimposter_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo1( true ) );
				SET_DYNAMIC_PIXEL_SHADER( windowimposter_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( windowimposter_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( windowimposter_ps20 );
			}

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			BindTexture( SHADER_SAMPLER0, ENVMAP, -1 );
			SetModulationVertexShaderDynamicState();
		}
		Draw();
	}

END_SHADER
