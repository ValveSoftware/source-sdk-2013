//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Cloaking shader for Spy in TF2 (and probably many other things to come)
//
// $NoKeywords: $
//=====================================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "cloak_dx9_helper.h"

DEFINE_FALLBACK_SHADER( Cloak, Cloak_DX90 )

BEGIN_VS_SHADER( Cloak_DX90, "Help for Cloak" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0f", "" )
		SHADER_PARAM( REFRACTTINTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shield", "" )
		SHADER_PARAM( REFRACTTINTTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "1.0", "1.0 == mirror, 0.0 == water" )
		SHADER_PARAM( NOWRITEZ, SHADER_PARAM_TYPE_INTEGER, "0", "0 == write z, 1 = no write z" )
		SHADER_PARAM( MASKED, SHADER_PARAM_TYPE_BOOL, "0", "mask using dest alpha" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0", "How cloaked?  Zero is not cloaked, 1 is fully cloaked." )
		SHADER_PARAM( LIGHTWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "1D ramp texture for tinting scalar diffuse term" )
		SHADER_PARAM( PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "5.0", "Phong exponent for local specular lights" )
		SHADER_PARAM( PHONGTINT, SHADER_PARAM_TYPE_VEC3, "5.0", "Phong tint for local specular lights" )
		SHADER_PARAM( PHONGALBEDOTINT, SHADER_PARAM_TYPE_BOOL, "1.0", "Apply tint by albedo (controlled by spec exponent texture" )
		SHADER_PARAM( PHONGFRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "[0  0.5  1]", "Parameters for remapping fresnel output" )
		SHADER_PARAM( PHONGBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Phong overbrightening factor (specular mask channel should be authored to account for this)" )
		SHADER_PARAM( PHONGEXPONENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Phong Exponent map" )

		// Rim lighting terms
		SHADER_PARAM( RIMLIGHT, SHADER_PARAM_TYPE_BOOL, "0", "enables rim lighting" )
		SHADER_PARAM( RIMLIGHTEXPONENT, SHADER_PARAM_TYPE_FLOAT, "4.0", "Exponent for rim lights" )
		SHADER_PARAM( RIMLIGHTBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Boost for rim lights" )
		SHADER_PARAM( RIMMASK, SHADER_PARAM_TYPE_BOOL, "0", "Indicates whether or not to use alpha channel of exponent texture to mask the rim term" )
	END_SHADER_PARAMS
// FIXME: doesn't support Fresnel!

	void SetupVars( Cloak_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nRefractAmount = REFRACTAMOUNT;
		info.m_nRefractTint = REFRACTTINT;
		info.m_nNormalMap = NORMALMAP;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpTransform = BUMPTRANSFORM;
		info.m_nRefractTintTexture = REFRACTTINTTEXTURE;
		info.m_nRefractTintTextureFrame = REFRACTTINTTEXTUREFRAME;
		info.m_nFresnelReflection = FRESNELREFLECTION;
		info.m_nMasked = MASKED;
		info.m_nCloakFactor = CLOAKFACTOR;
		info.m_nDiffuseWarpTexture = LIGHTWARPTEXTURE;
		info.m_nPhongExponent = PHONGEXPONENT;
		info.m_nPhongTint = PHONGTINT;
		info.m_nPhongAlbedoTint = PHONGALBEDOTINT;
		info.m_nPhongExponentTexture = PHONGEXPONENTTEXTURE;
		info.m_nPhongBoost = PHONGBOOST;
		info.m_nPhongFresnelRanges = PHONGFRESNELRANGES;

		// Rim lighting parameters
		info.m_nRimLight = RIMLIGHT;
		info.m_nRimLightPower = RIMLIGHTEXPONENT;
		info.m_nRimLightBoost = RIMLIGHTBOOST;
		info.m_nRimMask = RIMMASK;
	}

	bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const 
	{ 
		if ( bCheckSpecificToThisFrame == false ) // For setting model flag at load time
			return true;
		else if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
			return true;
		else
			return false;
	}

	bool IsTranslucent( IMaterialVar **params ) const
	{
		if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
			return true;
		else
			return false;
	}

	SHADER_INIT_PARAMS()
	{
		Cloak_DX9_Vars_t info;
		SetupVars( info );
		InitParamsCloak_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 82 )
			return "Refract_DX80";

		return 0;
	}

	SHADER_INIT
	{
		Cloak_DX9_Vars_t info;
		SetupVars( info );
		InitCloak_DX9( this, params, info );
	}

	SHADER_DRAW
	{
		Cloak_DX9_Vars_t info;
		SetupVars( info );
		DrawCloak_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
	}
END_SHADER

