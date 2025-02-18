//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Wireframe, Wireframe_DX8 )
DEFINE_FALLBACK_SHADER( Wireframe_DX9, Wireframe_DX8 )

BEGIN_VS_SHADER( Wireframe_DX8,
			  "Help for Wireframe_DX8" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		InitParamsUnlitGeneric_DX8( -1, -1, -1, -1, -1, -1, -1 );

		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_NOFOG );
		SET_FLAGS( MATERIAL_VAR_WIREFRAME );
	}

	SHADER_INIT
	{
		InitUnlitGeneric_DX8( -1, -1, -1, -1 );
	}

	SHADER_DRAW
	{
		VertexShaderUnlitGenericPass( 
			-1, -1, -1,
			-1, -1, true, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 );
	}
END_SHADER

