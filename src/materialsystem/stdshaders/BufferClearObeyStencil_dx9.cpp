//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Clears color/depth, but obeys stencil while doing so
//
//=============================================================================//

#include "BaseVSShader.h"

#include "bufferclearobeystencil_vs20.inc"
#include "bufferclearobeystencil_ps20.inc"
#include "bufferclearobeystencil_ps20b.inc"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( BufferClearObeyStencil, BufferClearObeyStencil_DX9 )

BEGIN_VS_SHADER_FLAGS( BufferClearObeyStencil_DX9, "", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( CLEARCOLOR, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of color" )
		SHADER_PARAM( CLEARALPHA, SHADER_PARAM_TYPE_INTEGER, "-1", "activates clearing of alpha. -1 == copy CLEARCOLOR setting" )
		SHADER_PARAM( CLEARDEPTH, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of depth" )		
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "BufferClearObeyStencil_DX8";
		}
		return 0;
	}

	SHADER_INIT
	{
		if ( !params[CLEARALPHA]->IsDefined() )
			params[CLEARALPHA]->SetIntValue( -1 );
	}

	SHADER_DRAW
	{
		bool bEnableColorWrites = params[CLEARCOLOR]->GetIntValue() != 0;
		bool bEnableAlphaWrites = (params[CLEARALPHA]->GetIntValue() >= 0) ? (params[CLEARALPHA]->GetIntValue() != 0) : bEnableColorWrites;

		bool bUsesColor = bEnableColorWrites || bEnableAlphaWrites;
		SHADOW_STATE
		{
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			bool bEnableDepthWrites = params[CLEARDEPTH]->GetIntValue() != 0;
			pShaderShadow->EnableDepthWrites( bEnableDepthWrites );

			pShaderShadow->EnableColorWrites( bEnableColorWrites );
			pShaderShadow->EnableAlphaWrites( bEnableAlphaWrites );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0 );

			DECLARE_STATIC_VERTEX_SHADER( bufferclearobeystencil_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( USESCOLOR, bUsesColor || g_pHardwareConfig->PlatformRequiresNonNullPixelShaders() );
			SET_STATIC_VERTEX_SHADER( bufferclearobeystencil_vs20 );

			//avoid setting a pixel shader when only doing depth/stencil operations, as recommended by PIX
			if( bUsesColor || g_pHardwareConfig->PlatformRequiresNonNullPixelShaders() )
			{
				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( bufferclearobeystencil_ps20b );
					SET_STATIC_PIXEL_SHADER( bufferclearobeystencil_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( bufferclearobeystencil_ps20 );
					SET_STATIC_PIXEL_SHADER( bufferclearobeystencil_ps20 );
				}
			}

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ZERO );
			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_ALWAYS, 0 );
		}

		DYNAMIC_STATE
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( bufferclearobeystencil_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( bufferclearobeystencil_vs20 );

			//avoid setting a pixel shader when only doing depth/stencil operations, as recommended by PIX
			if( bUsesColor || g_pHardwareConfig->PlatformRequiresNonNullPixelShaders() )
			{
				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( bufferclearobeystencil_ps20b );
					SET_DYNAMIC_PIXEL_SHADER( bufferclearobeystencil_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( bufferclearobeystencil_ps20 );
					SET_DYNAMIC_PIXEL_SHADER( bufferclearobeystencil_ps20 );
				}
			}
		}

		Draw( );
	}

END_SHADER
