//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "vertexlitgeneric_dx9_helper.h"

extern ConVar r_flashlight_version2;

BEGIN_VS_SHADER( UnlitGeneric, "Help for UnlitGeneric" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALBEDO, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "albedo (Base texture with no baked lighting)" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.7", "" )	
		SHADER_PARAM( VERTEXALPHATEST, SHADER_PARAM_TYPE_INTEGER, "0", "" )	
		SHADER_PARAM( HDRCOLORSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "hdr color scale" )
		SHADER_PARAM( PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "5.0", "Phong exponent for local specular lights" )
		SHADER_PARAM( PHONGTINT, SHADER_PARAM_TYPE_VEC3, "5.0", "Phong tint for local specular lights" )
		SHADER_PARAM( PHONGALBEDOTINT, SHADER_PARAM_TYPE_BOOL, "1.0", "Apply tint by albedo (controlled by spec exponent texture" )
		SHADER_PARAM( LIGHTWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "1D ramp texture for tinting scalar diffuse term" )
		SHADER_PARAM( PHONGWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "2D map for warping specular" )
		SHADER_PARAM( PHONGFRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "[0  0.5  1]", "Parameters for remapping fresnel output" )
		SHADER_PARAM( PHONGBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Phong overbrightening factor (specular mask channel should be authored to account for this)" )
		SHADER_PARAM( PHONGEXPONENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Phong Exponent map" )
		SHADER_PARAM( PHONG, SHADER_PARAM_TYPE_BOOL, "0", "enables phong lighting" )
		SHADER_PARAM( DETAILBLENDMODE, SHADER_PARAM_TYPE_INTEGER, "0", "mode for combining detail texture with base. 0=normal, 1= additive, 2=alpha blend detail over base, 3=crossfade" )
		SHADER_PARAM( DETAILBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "blend amount for detail texture." )
		SHADER_PARAM( DETAILTEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$detail texcoord transform" )

		SHADER_PARAM( SELFILLUMMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "If we bind a texture here, it overrides base alpha (if any) for self illum" )

		SHADER_PARAM( DISTANCEALPHA, SHADER_PARAM_TYPE_BOOL, "0", "Use distance-coded alpha generated from hi-res texture by vtex.")
		SHADER_PARAM( DISTANCEALPHAFROMDETAIL, SHADER_PARAM_TYPE_BOOL, "0", "Take the distance-coded alpha mask from the detail texture.")

		SHADER_PARAM( SOFTEDGES, SHADER_PARAM_TYPE_BOOL, "0", "Enable soft edges to distance coded textures.")
		SHADER_PARAM( SCALEEDGESOFTNESSBASEDONSCREENRES, SHADER_PARAM_TYPE_BOOL, "0", "Scale the size of the soft edges based upon resolution. 1024x768 = nominal.")
	    SHADER_PARAM( EDGESOFTNESSSTART, SHADER_PARAM_TYPE_FLOAT, "0.6", "Start value for soft edges for distancealpha.");
		SHADER_PARAM( EDGESOFTNESSEND, SHADER_PARAM_TYPE_FLOAT, "0.5", "End value for soft edges for distancealpha.");

		SHADER_PARAM( GLOW, SHADER_PARAM_TYPE_BOOL, "0", "Enable glow/shadow for distance coded textures.")
		SHADER_PARAM( GLOWCOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "color of outter glow for distance coded line art." )
		SHADER_PARAM( GLOWALPHA, SHADER_PARAM_TYPE_FLOAT, "1", "Base glow alpha amount for glows/shadows with distance alpha." )
		SHADER_PARAM( GLOWSTART, SHADER_PARAM_TYPE_FLOAT, "0.7", "start value for glow/shadow")
		SHADER_PARAM( GLOWEND, SHADER_PARAM_TYPE_FLOAT, "0.5", "end value for glow/shadow")
		SHADER_PARAM( GLOWX, SHADER_PARAM_TYPE_FLOAT, "0", "texture offset x for glow mask.")
		SHADER_PARAM( GLOWY, SHADER_PARAM_TYPE_FLOAT, "0", "texture offset y for glow mask.")

		SHADER_PARAM( OUTLINE, SHADER_PARAM_TYPE_BOOL, "0", "Enable outline for distance coded textures.")
		SHADER_PARAM( OUTLINECOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "color of outline for distance coded images." )
		SHADER_PARAM( OUTLINEALPHA, SHADER_PARAM_TYPE_FLOAT, "0.0", "alpha value for outline")
		SHADER_PARAM( OUTLINESTART0, SHADER_PARAM_TYPE_FLOAT, "0.0", "outer start value for outline")
		SHADER_PARAM( OUTLINESTART1, SHADER_PARAM_TYPE_FLOAT, "0.0", "inner start value for outline")
		SHADER_PARAM( OUTLINEEND0, SHADER_PARAM_TYPE_FLOAT, "0.0", "inner end value for outline")
		SHADER_PARAM( OUTLINEEND1, SHADER_PARAM_TYPE_FLOAT, "0.0", "outer end value for outline")
		SHADER_PARAM( SCALEOUTLINESOFTNESSBASEDONSCREENRES, SHADER_PARAM_TYPE_BOOL, "0", "Scale the size of the soft part of the outline based upon resolution. 1024x768 = nominal.")

		SHADER_PARAM( SEPARATEDETAILUVS, SHADER_PARAM_TYPE_BOOL, "0", "Use texcoord1 for detail texture" )

		SHADER_PARAM( GAMMACOLORREAD, SHADER_PARAM_TYPE_INTEGER, "0", "Disables SRGB conversion of color texture read." )
		SHADER_PARAM( LINEARWRITE, SHADER_PARAM_TYPE_INTEGER, "0", "Disables SRGB conversion of shader results." )

		SHADER_PARAM( DEPTHBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "fade at intersection boundaries" )
		SHADER_PARAM( DEPTHBLENDSCALE, SHADER_PARAM_TYPE_FLOAT, "50.0", "Amplify or reduce DEPTHBLEND fading. Lower values make harder edges." )
		SHADER_PARAM( RECEIVEFLASHLIGHT, SHADER_PARAM_TYPE_INTEGER, "0", "Forces this material to receive flashlights." )

	END_SHADER_PARAMS

	void SetupVars( VertexLitGeneric_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nAlbedo = ALBEDO;
		info.m_nSelfIllumTint = -1;
		info.m_nDetail = DETAIL;
		info.m_nDetailFrame = DETAILFRAME;
		info.m_nDetailScale = DETAILSCALE;
		info.m_nDetailTextureCombineMode = DETAILBLENDMODE;
		info.m_nDetailTextureBlendFactor = DETAILBLENDFACTOR;
		info.m_nDetailTextureTransform = DETAILTEXTURETRANSFORM;

		info.m_nEnvmap = ENVMAP;
		info.m_nEnvmapFrame = ENVMAPFRAME;
		info.m_nEnvmapMask = ENVMAPMASK;
		info.m_nEnvmapMaskFrame = ENVMAPMASKFRAME;
		info.m_nEnvmapMaskTransform = ENVMAPMASKTRANSFORM;
		info.m_nEnvmapTint = ENVMAPTINT;
		info.m_nBumpmap = -1;
		info.m_nBumpFrame = -1;
		info.m_nBumpTransform = -1;
		info.m_nEnvmapContrast = ENVMAPCONTRAST;
		info.m_nEnvmapSaturation = ENVMAPSATURATION;
		info.m_nAlphaTestReference = ALPHATESTREFERENCE;
		info.m_nVertexAlphaTest = VERTEXALPHATEST;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
		info.m_nHDRColorScale = HDRCOLORSCALE;
		info.m_nPhongExponent = -1;
		info.m_nPhongExponentTexture = -1;
		info.m_nDiffuseWarpTexture = -1;
		info.m_nPhongWarpTexture = -1;
		info.m_nPhongBoost = -1;
		info.m_nPhongFresnelRanges = -1;
		info.m_nPhong = -1;
		info.m_nPhongTint = -1;
		info.m_nPhongAlbedoTint = -1;
		info.m_nSelfIllumEnvMapMask_Alpha = -1;
		info.m_nAmbientOnly = -1;
		info.m_nBaseMapAlphaPhongMask = -1;
		info.m_nEnvmapFresnel = -1;
		info.m_nSelfIllumMask = -1;

		info.m_nDistanceAlpha = DISTANCEALPHA;
		info.m_nDistanceAlphaFromDetail = DISTANCEALPHAFROMDETAIL;
		info.m_nSoftEdges = SOFTEDGES;
		info.m_nEdgeSoftnessStart = EDGESOFTNESSSTART;
		info.m_nEdgeSoftnessEnd = EDGESOFTNESSEND;
		info.m_nScaleEdgeSoftnessBasedOnScreenRes = SCALEEDGESOFTNESSBASEDONSCREENRES;

		info.m_nGlow = GLOW;
		info.m_nGlowColor = GLOWCOLOR;
		info.m_nGlowAlpha = GLOWALPHA;
		info.m_nGlowStart = GLOWSTART;
		info.m_nGlowEnd = GLOWEND;
		info.m_nGlowX = GLOWX;
		info.m_nGlowY = GLOWY;

		info.m_nOutline = OUTLINE;
		info.m_nOutlineColor = OUTLINECOLOR;
		info.m_nOutlineAlpha = OUTLINEALPHA;
		info.m_nOutlineStart0 = OUTLINESTART0;
		info.m_nOutlineStart1 = OUTLINESTART1;
		info.m_nOutlineEnd0 = OUTLINEEND0;
		info.m_nOutlineEnd1 = OUTLINEEND1;
		info.m_nScaleOutlineSoftnessBasedOnScreenRes = SCALEOUTLINESOFTNESSBASEDONSCREENRES;

		info.m_nSeparateDetailUVs = SEPARATEDETAILUVS;

		info.m_nLinearWrite = LINEARWRITE;
		info.m_nGammaColorRead = GAMMACOLORREAD;

		info.m_nDepthBlend = DEPTHBLEND;
		info.m_nDepthBlendScale = DEPTHBLENDSCALE;
		info.m_nReceiveFlashlight = RECEIVEFLASHLIGHT;
	}

	SHADER_INIT_PARAMS()
	{
		VertexLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );
		InitParamsVertexLitGeneric_DX9( this, params, pMaterialName, false, vars );
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "UnlitGeneric_DX8";
		}
		return 0;
	}

	SHADER_INIT
	{
		VertexLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );
		InitVertexLitGeneric_DX9( this, params, false, vars );
	}

	SHADER_DRAW
	{
		VertexLitGeneric_DX9_Vars_t vars;
		SetupVars( vars );

		bool bNewFlashlightPath = IsX360() || ( r_flashlight_version2.GetInt() != 0 );
		if ( ( pShaderShadow == NULL ) && ( pShaderAPI != NULL ) && !bNewFlashlightPath && pShaderAPI->InFlashlightMode() ) // Not snapshotting && flashlight pass
		{
			Draw( false );
		}
		else
		{
			DrawVertexLitGeneric_DX9( this, params, pShaderAPI, pShaderShadow, false, vars, vertexCompression, pContextDataPtr );
		}
	}
END_SHADER
