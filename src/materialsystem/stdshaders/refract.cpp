//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "refract_dx9_helper.h"

DEFINE_FALLBACK_SHADER( Refract, Refract_DX90 )

BEGIN_VS_SHADER( Refract_DX90, "Help for Refract" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( NORMALMAP2, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $normalmap" )
		SHADER_PARAM( BUMPFRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $normalmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$normalmap texcoord transform" )
		SHADER_PARAM( BUMPTRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$normalmap texcoord transform" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0f", "" )
		SHADER_PARAM( BLURAMOUNT, SHADER_PARAM_TYPE_INTEGER, "1", "0, 1, or 2 for how much blur you want" )
		SHADER_PARAM( FADEOUTONSILHOUETTE, SHADER_PARAM_TYPE_BOOL, "1", "0 for no fade out on silhouette, 1 for fade out on sillhouette" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "envmap frame number" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( REFRACTTINTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shield", "" )
		SHADER_PARAM( REFRACTTINTTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "1.0", "1.0 == mirror, 0.0 == water" )
		SHADER_PARAM( NOWRITEZ, SHADER_PARAM_TYPE_INTEGER, "0", "0 == write z, 1 = no write z" )
		SHADER_PARAM( MASKED, SHADER_PARAM_TYPE_BOOL, "0", "mask using dest alpha" )
		SHADER_PARAM( VERTEXCOLORMODULATE, SHADER_PARAM_TYPE_BOOL, "0","Use the vertex color to effect refract color. alpha will adjust refract amount" )
		SHADER_PARAM( FORCEALPHAWRITE, SHADER_PARAM_TYPE_BOOL, "0","Force the material to write alpha to the dest buffer" )
	END_SHADER_PARAMS
// FIXME: doesn't support Fresnel!

	void SetupVars( Refract_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nFrame = FRAME;
		info.m_nRefractAmount = REFRACTAMOUNT;
		info.m_nRefractTint = REFRACTTINT;
		info.m_nNormalMap = NORMALMAP;
		info.m_nNormalMap2 = NORMALMAP2;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpFrame2 = BUMPFRAME2;
		info.m_nBumpTransform = BUMPTRANSFORM;
		info.m_nBumpTransform2 = BUMPTRANSFORM2;
		info.m_nBlurAmount = BLURAMOUNT;
		info.m_nFadeOutOnSilhouette = FADEOUTONSILHOUETTE;
		info.m_nEnvmap = ENVMAP;
		info.m_nEnvmapFrame = ENVMAPFRAME;
		info.m_nEnvmapTint = ENVMAPTINT;
		info.m_nEnvmapContrast = ENVMAPCONTRAST;
		info.m_nEnvmapSaturation = ENVMAPSATURATION;
		info.m_nRefractTintTexture = REFRACTTINTTEXTURE;
		info.m_nRefractTintTextureFrame = REFRACTTINTTEXTUREFRAME;
		info.m_nFresnelReflection = FRESNELREFLECTION;
		info.m_nNoWriteZ = NOWRITEZ;
		info.m_nMasked = MASKED;
		info.m_nVertexColorModulate = VERTEXCOLORMODULATE;
		info.m_nForceAlphaWrite = FORCEALPHAWRITE;
	}

	SHADER_INIT_PARAMS()
	{
		Refract_DX9_Vars_t info;
		SetupVars( info );
		InitParamsRefract_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 82 )
			return "Refract_DX80";

		return 0;
	}

	SHADER_INIT
	{
		Refract_DX9_Vars_t info;
		SetupVars( info );
		InitRefract_DX9( this, params, info );
	}

	SHADER_DRAW
	{
		Refract_DX9_Vars_t info;
		SetupVars( info );
		
		// If ( snapshotting ) or ( we need to draw this frame )
		bool bHasFlashlight = this->UsingFlashlight( params );
		if ( ( pShaderShadow != NULL ) || ( bHasFlashlight == false ) )
		{
			DrawRefract_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
		}
		else
		{
			Draw( false );
		}
	}
END_SHADER
