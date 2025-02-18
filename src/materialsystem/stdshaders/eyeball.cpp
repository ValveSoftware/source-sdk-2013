//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Eyeball shader
//
//=============================================================================//

#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Eyeball, "Help for EyeBall" )
			   
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "models/alyx/pupil_l", "iris texture", 0 )
		SHADER_PARAM_OVERRIDE( BASETEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "unused", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		// This should be a dead shader...
		return "Wireframe";
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
	}

END_SHADER
