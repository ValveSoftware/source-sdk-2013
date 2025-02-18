//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Clears color/depth, but obeys stencil while doing so
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( BufferClearObeyStencil, BufferClearObeyStencil_DX6 )

BEGIN_SHADER_FLAGS( BufferClearObeyStencil_DX6, "Help for BufferClearObeyStencil_DX6", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( CLEARCOLOR, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of color" )
		SHADER_PARAM( CLEARDEPTH, SHADER_PARAM_TYPE_INTEGER, "1", "activates clearing of depth" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
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

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );

			DisableFog();
		}
		DYNAMIC_STATE
		{
		}
		Draw( );
	}
END_SHADER
