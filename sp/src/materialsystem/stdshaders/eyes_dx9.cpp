//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: eye renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "eyes_dx8_dx9_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( eyes, Eyes_dx9 )

BEGIN_VS_SHADER( Eyes_dx9, "Help for Eyes" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( IRIS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "iris texture" )
		SHADER_PARAM( IRISFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame for the iris texture" )
		SHADER_PARAM( GLINT, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "glint texture" )
		SHADER_PARAM( EYEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "origin for the eyes" )
		SHADER_PARAM( EYEUP, SHADER_PARAM_TYPE_VEC3, "[0 0 1]", "up vector for the eyes" )
		SHADER_PARAM( IRISU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0 ]", "U projection vector for the iris" )
		SHADER_PARAM( IRISV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the iris" )
		SHADER_PARAM( GLINTU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "U projection vector for the glint" )
		SHADER_PARAM( GLINTV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the glint" )
		SHADER_PARAM( DILATION, SHADER_PARAM_TYPE_FLOAT, "0", "Pupil dilation (0 is none, 1 is maximal)" )
		SHADER_PARAM( INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is eyes in the ep1 intro" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )
	END_SHADER_PARAMS

	void SetupVars( Eyes_DX8_DX9_Vars_t &info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nFrame = FRAME;
		info.m_nIris = IRIS;
		info.m_nIrisFrame = IRISFRAME;
		info.m_nGlint = GLINT;
		info.m_nEyeOrigin = EYEORIGIN;
		info.m_nEyeUp = EYEUP;
		info.m_nIrisU = IRISU;
		info.m_nIrisV = IRISV;
		info.m_nGlintU = GLINTU;
		info.m_nGlintV = GLINTV;
		info.m_nDilation = DILATION;
		info.m_nIntro = INTRO;
		info.m_nEntityOrigin = ENTITYORIGIN;
		info.m_nWarpParam = WARPPARAM;
	}

	SHADER_INIT_PARAMS()
	{
		Eyes_DX8_DX9_Vars_t info;
		SetupVars( info );
		InitParamsEyes_DX8_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Eyes_dx8";

		return 0;
	}

	SHADER_INIT
	{
		Eyes_DX8_DX9_Vars_t info;
		SetupVars( info );
		InitEyes_DX8_DX9( this, params, info );
	}


	SHADER_DRAW
	{
		Eyes_DX8_DX9_Vars_t info;
		SetupVars( info );
		DrawEyes_DX8_DX9( true, this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
	}
END_SHADER

