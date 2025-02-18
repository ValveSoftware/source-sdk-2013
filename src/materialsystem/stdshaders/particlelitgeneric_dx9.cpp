//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "particlelitgeneric_dx9_helper.h"

BEGIN_VS_SHADER( ParticleLitGeneric_DX9, 
				"Help for ParticleLitGeneric_DX9" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
	END_SHADER_PARAMS

	void SetupVars( ParticleLitGeneric_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nBumpmap = BUMPMAP;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpTransform = BUMPTRANSFORM;
		info.m_nAlphaTestReference = -1;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
	}

	SHADER_INIT_PARAMS()
	{
		ParticleLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );
//		InitParamsParticleLitGeneric_DX9( this, params, pMaterialName, vars );
	}

	SHADER_FALLBACK
	{	
		if (g_pHardwareConfig->GetDXSupportLevel() < 70)
			return "ParticleLitGeneric_DX6";

		if (g_pHardwareConfig->GetDXSupportLevel() < 80)
			return "ParticleLitGeneric_DX7";

		return "ParticleLitGeneric_DX8";

		return 0;
	}

	SHADER_INIT
	{
		ParticleLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );
//		InitParticleLitGeneric_DX9( this, params, vars );
	}

	SHADER_DRAW
	{
		ParticleLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );
//		DrawParticleLitGeneric_DX9( this, params, pShaderAPI, pShaderShadow, vars );
	}
END_SHADER
