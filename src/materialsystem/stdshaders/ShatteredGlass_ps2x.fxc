// STATIC: "CONVERT_TO_SRGB"		"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"		"0..0"	[= 0] [XBOX]
// STATIC: "CUBEMAP"				"0..1"
// STATIC: "VERTEXCOLOR"			"0..1"
// STATIC: "ENVMAPMASK"				"0..1"
// STATIC: "BASEALPHAENVMAPMASK"	"0..1"
// STATIC: "HDRTYPE"				"0..2"
				   
// DYNAMIC: "HDRENABLED" "0..1"
// DYNAMIC: "PIXELFOGTYPE" "0..1"

// HDRFIXME: Need to make this work.

#define USE_32BIT_LIGHTMAPS_ON_360 //uncomment to use 32bit lightmaps, be sure to keep this in sync with the same #define in materialsystem/cmatlightmaps.cpp

#include "common_ps_fxc.h"
#include "common_lightmappedgeneric_fxc.h"

const HALF4 g_EnvmapTint					: register( c0 );
const HALF3 g_DiffuseModulation				: register( c1 );
const HALF3 g_EnvmapContrast				: register( c2 );
const HALF3 g_EnvmapSaturation				: register( c3 );
const HALF4 g_FresnelReflection				: register( c4 );
const HALF3 g_EyePos						: register( c5 );
const HALF3 g_OverbrightFactor				: register( c6 );

const HALF4 g_FogParams						: register( c12 );

// CENTROID: TEXCOORD2
	    
sampler BaseTextureSampler	: register( s0 );
sampler LightmapSampler		: register( s1 );
sampler EnvmapSampler		: register( s2 );
sampler DetailSampler		: register( s3 );
sampler EnvmapMaskSampler	: register( s5 );

sampler NormalizeSampler : register( s6 );

struct PS_INPUT
{
	HALF2 baseTexCoord				: TEXCOORD0;
	HALF2 detailTexCoord			: TEXCOORD1;
	HALF2 lightmapTexCoord			: TEXCOORD2;
	HALF2 envmapMaskTexCoord		: TEXCOORD3;
	HALF4 worldPos_projPosZ			: TEXCOORD4;
	HALF3 worldSpaceNormal			: TEXCOORD5;
	HALF4 vertexColor				: COLOR;
	HALF4 fogFactorW				: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bCubemap = CUBEMAP ? true : false;
	bool bVertexColor = VERTEXCOLOR ? true : false;
	bool bEnvmapMask = ENVMAPMASK ? true : false;
	bool bBaseAlphaEnvmapMask = BASEALPHAENVMAPMASK ? true : false;

	HALF4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );
	HALF4 detailColor = tex2D( DetailSampler, i.detailTexCoord );

	HALF2 lightmapCoordinates = i.lightmapTexCoord;
	HALF3 lightmapColor = LightMapSample( LightmapSampler, lightmapCoordinates );

	HALF3 specularFactor = 1.0f;
	if( bEnvmapMask )
	{
		specularFactor = tex2D( EnvmapMaskSampler, i.detailTexCoord ).xyz;	
	}

	if( bBaseAlphaEnvmapMask )
	{
		specularFactor *= 1.0 - baseColor.a; // this blows!
	}

	HALF3 diffuseLighting = lightmapColor;
	diffuseLighting *= g_DiffuseModulation;
	diffuseLighting *= LIGHT_MAP_SCALE;

	HALF3 albedo = baseColor;
	HALF alpha = 1.0f;

	if( !bBaseAlphaEnvmapMask )
	{
		alpha *= baseColor.a;
	}

	albedo *= detailColor;
	alpha *= detailColor.a;

	// FIXME: seperate vertexcolor and vertexalpha?
	// vertex alpha is ignored if vertexcolor isn't set. . need to check other version.
	if( bVertexColor )
	{
		albedo *= i.vertexColor;
		alpha *= i.vertexColor.a; // not sure about this one
	}

	HALF3 specularLighting = HALF3( 0.0f, 0.0f, 0.0f );
	if( bCubemap )
	{
		float3 worldVertToEyeVector = g_EyePos - i.worldPos_projPosZ.xyz;
		worldVertToEyeVector = NormalizeWithCubemap( NormalizeSampler, worldVertToEyeVector );
		HALF3 reflectVect = CalcReflectionVectorUnnormalized( i.worldSpaceNormal, worldVertToEyeVector );

		// Calc Fresnel factor
		HALF3 worldSpaceNormal = NormalizeWithCubemap( NormalizeSampler, i.worldSpaceNormal );
		HALF fresnel = 1.0 - dot( worldSpaceNormal, worldVertToEyeVector );
		fresnel = pow( fresnel, 5.0 );
		fresnel = fresnel * g_FresnelReflection.b + g_FresnelReflection.a;
			 
		specularLighting = texCUBE( EnvmapSampler, reflectVect );
		specularLighting *= specularFactor;

		specularLighting *= g_EnvmapTint;
#if HDRTYPE == HDR_TYPE_NONE
		HALF3 specularLightingSquared = specularLighting * specularLighting;
		specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
		HALF3 greyScale = dot( specularLighting, HALF3( 0.299f, 0.587f, 0.114f ) );
		specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
#endif
		specularLighting *= fresnel;
	}

	// Do it somewhat unlit
	HALF3 result = albedo*(g_OverbrightFactor.z*diffuseLighting + g_OverbrightFactor.y) + specularLighting;
	
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( HALF4( result, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}

