//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Naively sets the depth buffer values without testing the old values and without writing to alpha or color
//
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( SetZ, SetZ_DX6 )

BEGIN_SHADER_FLAGS( SetZ_DX6, "Help for SetZ_DX6", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
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
			pShaderShadow->EnableColorWrites( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
		}
		Draw();
	}
END_SHADER

