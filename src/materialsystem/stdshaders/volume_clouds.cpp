//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "volume_clouds_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( VolumeClouds, VolumeClouds_dx9 )
BEGIN_VS_SHADER( VolumeClouds_dx9, "VolumeClouds" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Texture 1" )
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "Texture 2" )
		SHADER_PARAM( BASETEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "Texture 3" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "Needs CurrentTime Proxy" )
	END_SHADER_PARAMS

	void SetupVarsVolumeClouds( VolumeCloudsVars_t &info )
	{
		info.m_nRefractAmount = REFRACTAMOUNT;
		info.m_nTexture1 = BASETEXTURE;
		info.m_nTexture2 = BASETEXTURE2;
		info.m_nTexture3 = BASETEXTURE3;
		info.m_nTime = TIME;
	}

	SHADER_INIT_PARAMS()
	{
		VolumeCloudsVars_t info;
		SetupVarsVolumeClouds( info );
		InitParamsVolumeClouds( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			// Fallback to unlit generic
			return "UnlitGeneric_DX8";
		}

		return 0;
	}

	SHADER_INIT
	{
		VolumeCloudsVars_t info;
		SetupVarsVolumeClouds( info );
		InitVolumeClouds( this, params, info );
	}

	SHADER_DRAW
	{
		VolumeCloudsVars_t info;
		SetupVarsVolumeClouds( info );
		DrawVolumeClouds( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
	}
END_SHADER
