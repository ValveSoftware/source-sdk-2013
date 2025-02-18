//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Core, Core_dx70 )

// This is here so that you can use $fallbackmaterial for Core_dx7
DEFINE_FALLBACK_SHADER( Core_dx70, Wireframe )

#if 0
BEGIN_VS_SHADER( Core_DX70, 
			  "Help for Core_DX70" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
	}
END_SHADER
#endif
