//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "aftershock_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Aftershock, Aftershock_dx9 )
BEGIN_VS_SHADER( Aftershock_dx9, "Aftershock" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( COLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )

		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )

		SHADER_PARAM( SILHOUETTETHICKNESS, SHADER_PARAM_TYPE_FLOAT, "1", "" )
		SHADER_PARAM( SILHOUETTECOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Silhouette color tint" )
		SHADER_PARAM( GROUNDMIN, SHADER_PARAM_TYPE_FLOAT, "1", "" )
		SHADER_PARAM( GROUNDMAX, SHADER_PARAM_TYPE_FLOAT, "1", "" )
		SHADER_PARAM( BLURAMOUNT, SHADER_PARAM_TYPE_FLOAT, "1", "" )

		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "Needs CurrentTime Proxy" )
	END_SHADER_PARAMS

	void SetupVarsAftershock( AftershockVars_t &info )
	{
		info.m_nColorTint = COLORTINT;
		info.m_nRefractAmount = REFRACTAMOUNT;

		info.m_nBumpmap = NORMALMAP;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpTransform = BUMPTRANSFORM;

		info.m_nSilhouetteThickness = SILHOUETTETHICKNESS;
		info.m_nSilhouetteColor = SILHOUETTECOLOR;
		info.m_nGroundMin = GROUNDMIN;
		info.m_nGroundMax = GROUNDMAX;
		info.m_nBlurAmount = BLURAMOUNT;

		info.m_nTime = TIME;
	}

	SHADER_INIT_PARAMS()
	{
		AftershockVars_t info;
		SetupVarsAftershock( info );
		InitParamsAftershock( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Aftershock_dx8";
		}

		return 0;
	}

	SHADER_INIT
	{
		AftershockVars_t info;
		SetupVarsAftershock( info );
		InitAftershock( this, params, info );
	}

	SHADER_DRAW
	{
		AftershockVars_t info;
		SetupVarsAftershock( info );
		DrawAftershock( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
	}
END_SHADER
