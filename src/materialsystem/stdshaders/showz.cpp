//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Visualize shadow z buffers.  Designed to be used when drawing a screen-aligned
//          quad with a floating-point z-buffer so that the large z-range is divided down
//          into visual range of grayscale colors.
//
// $NoKeywords: $
//=============================================================================//

#include "convar.h"
#include "BaseVSShader.h"

#include "showz_vs20.inc"
#include "showz_ps20.inc"
#include "showz_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar r_showz_power( "r_showz_power", "1.0f", FCVAR_CHEAT );

BEGIN_VS_SHADER_FLAGS( showz, "Help for ShowZ", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALPHADEPTH, SHADER_PARAM_TYPE_INTEGER, "0", "Depth is stored in alpha channel" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if ( !params[ALPHADEPTH]->IsDefined() )
		{
			params[ALPHADEPTH]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
//		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
//			return "Wireframe";
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			DECLARE_STATIC_VERTEX_SHADER( showz_vs20 );
			SET_STATIC_VERTEX_SHADER( showz_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( showz_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( DEPTH_IN_ALPHA, params[ALPHADEPTH]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER( showz_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( showz_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( DEPTH_IN_ALPHA, params[ALPHADEPTH]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER( showz_ps20 );
			}

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			pShaderShadow->EnableSRGBWrite( true );  // The back buffer is sRGB, we should always set this true!
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );	// Bind shadow depth map

			DECLARE_DYNAMIC_VERTEX_SHADER( showz_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( showz_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( showz_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( showz_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( showz_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( showz_ps20 );
			}

			Vector4D C0;
			C0.x = r_showz_power.GetFloat();

			pShaderAPI->SetPixelShaderConstant( 0, C0.Base(), 1 );
		}
		Draw();
	}
END_SHADER

