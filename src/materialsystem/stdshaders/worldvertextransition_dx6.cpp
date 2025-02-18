//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

#include "worldvertextransition_dx6_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WorldVertexTransition, WorldVertexTransition_DX6 )

BEGIN_SHADER( WorldVertexTransition_DX6,
			  "Help for WorldVertexTransition_dx6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture2", "base texture2 help" )
	END_SHADER_PARAMS

	void SetupVars( WorldVertexTransition_DX6_Vars_t& info )
	{
		info.m_nBaseTextureVar = BASETEXTURE;
		info.m_nBaseTextureFrameVar = FRAME;
		info.m_nBaseTexture2Var = BASETEXTURE2;
		info.m_nFlashlightTextureVar = FLASHLIGHTTEXTURE;
	}

	SHADER_INIT_PARAMS()
	{
		WorldVertexTransition_DX6_Vars_t info;
		SetupVars( info );
		InitParamsWorldVertexTransition_DX6( params, info );
	}
	
	SHADER_INIT
	{
		WorldVertexTransition_DX6_Vars_t info;
		SetupVars( info );
		InitWorldVertexTransition_DX6( this, params, info );
	}

	SHADER_DRAW
	{
		WorldVertexTransition_DX6_Vars_t info;
		SetupVars( info );
		DrawWorldVertexTransition_DX6( this, params, pShaderAPI, pShaderShadow, info );
	}
END_SHADER
