//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader that can be applied to models
//
//==================================================================================================

#include "BaseVSShader.h"
#include "convar.h"
#include "example_model_dx9_helper.h"

#ifdef GAME_SHADER_DLL
DEFINE_FALLBACK_SHADER( Mod_Example_Model, Mod_Example_Model_DX9 )
BEGIN_VS_SHADER( Mod_Example_Model_DX9, "Help for Example Model Shader" )
#else
DEFINE_FALLBACK_SHADER( Example_Model, Example_Model_DX9 )
BEGIN_VS_SHADER( Example_Model_DX9, "Help for Example Model Shader" )
#endif

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )	
	END_SHADER_PARAMS

	void SetupVars( ExampleModel_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nAlphaTestReference = ALPHATESTREFERENCE;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
	}

	SHADER_INIT_PARAMS()
	{
		ExampleModel_DX9_Vars_t info;
		SetupVars( info );
		InitParamsExampleModel_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		ExampleModel_DX9_Vars_t info;
		SetupVars( info );
		InitExampleModel_DX9( this, params, info );
	}

	SHADER_DRAW
	{
		ExampleModel_DX9_Vars_t info;
		SetupVars( info );
		DrawExampleModel_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
	}

END_SHADER

