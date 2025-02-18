//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "screenspaceeffect_vs20.inc"
#include "floatcombine_autoexpose_ps20.inc"
#include "floatcombine_autoexpose_ps20b.inc"

BEGIN_VS_SHADER( floatcombine_autoexpose, "Help for floatcombine_autoexpose" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BLOOMTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( SHARPNESS, SHADER_PARAM_TYPE_FLOAT, "1", "" )
	    SHADER_PARAM( WOODCUT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( VIGNETTE_MIN_BRIGHT,SHADER_PARAM_TYPE_FLOAT,"1","")
		SHADER_PARAM( VIGNETTE_POWER,SHADER_PARAM_TYPE_FLOAT,"4","")
		SHADER_PARAM( EDGE_SOFTNESS,SHADER_PARAM_TYPE_FLOAT,"0","")
	    SHADER_PARAM( BLOOMAMOUNT, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
		SHADER_PARAM( BLOOMEXPONENT, SHADER_PARAM_TYPE_FLOAT, "2.0", "" )
		SHADER_PARAM( ALPHASHARPENFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( EXPOSURE_TEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	    SHADER_PARAM( AUTOEXPOSE_MIN, SHADER_PARAM_TYPE_FLOAT, ".5", "" )
	    SHADER_PARAM( AUTOEXPOSE_MAX, SHADER_PARAM_TYPE_FLOAT, "2", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
		if( params[BLOOMTEXTURE]->IsDefined() )
		{
			LoadTexture( BLOOMTEXTURE );
		}
		if( params[EXPOSURE_TEXTURE]->IsDefined() )
		{
			LoadTexture( EXPOSURE_TEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
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

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// convert from linear to gamma on write.
			pShaderShadow->EnableSRGBWrite( true );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( floatcombine_autoexpose_ps20b );
				SET_STATIC_PIXEL_SHADER( floatcombine_autoexpose_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( floatcombine_autoexpose_ps20 );
				SET_STATIC_PIXEL_SHADER( floatcombine_autoexpose_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			float c0[4]={params[SHARPNESS]->GetFloatValue(),
						 params[WOODCUT]->GetFloatValue(),
						 params[BLOOMAMOUNT]->GetFloatValue(),
						 params[ALPHASHARPENFACTOR]->GetFloatValue()};
			float c1[4]={params[BLOOMEXPONENT]->GetFloatValue(),
						 params[VIGNETTE_MIN_BRIGHT]->GetFloatValue(),
						 params[VIGNETTE_POWER]->GetFloatValue(),
						 params[EDGE_SOFTNESS]->GetFloatValue()};
			float c2[4]={params[AUTOEXPOSE_MIN]->GetFloatValue(),
						 params[AUTOEXPOSE_MAX]->GetFloatValue(),
						 0,0};
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			pShaderAPI->SetPixelShaderConstant( 1, c1, 1 );
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BLOOMTEXTURE, -1 );
			BindTexture( SHADER_SAMPLER2, EXPOSURE_TEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( floatcombine_autoexpose_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( floatcombine_autoexpose_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( floatcombine_autoexpose_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( floatcombine_autoexpose_ps20 );
			}
		}
		Draw();
	}
END_SHADER
