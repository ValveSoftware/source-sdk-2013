//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Clears color/depth, but obeys stencil while doing so
//
//=============================================================================//

#include "BaseVSShader.h"
#include "BufferClearObeyStencil_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( BufferClearObeyStencil, BufferClearObeyStencil_DX8 )

BEGIN_VS_SHADER_FLAGS( BufferClearObeyStencil_DX8, "", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( CLEARCOLOR, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of color" )
		SHADER_PARAM( CLEARDEPTH, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of depth" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "BufferClearObeyStencil_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			bool bEnableDepthWrites = params[CLEARDEPTH]->GetIntValue() != 0;
			pShaderShadow->EnableDepthWrites( bEnableDepthWrites );

			bool bEnableColorWrites = params[CLEARCOLOR]->GetIntValue() != 0;
			pShaderShadow->EnableColorWrites( bEnableColorWrites );
			pShaderShadow->EnableAlphaWrites( bEnableColorWrites );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION|VERTEX_COLOR, 1, 0, 0 );

			bufferclearobeystencil_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "BufferClearObeyStencil_vs11", vshIndex.GetIndex() );
			pShaderShadow->SetPixelShader( "BufferClearObeyStencil_ps11", 0 );
		}

		DYNAMIC_STATE
		{
			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );
		}

		Draw( );
	}

END_SHADER
