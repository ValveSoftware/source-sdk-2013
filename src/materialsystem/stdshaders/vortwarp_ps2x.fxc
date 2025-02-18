//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps30][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// STATIC: "BASETEXTURE"				"0..1"
// STATIC: "CUBEMAP"					"0..1"
// STATIC: "DIFFUSELIGHTING"			"0..1"
// STATIC: "NORMALMAPALPHAENVMAPMASK"	"0..1"
// STATIC: "HALFLAMBERT"				"0..1"
// STATIC: "FLASHLIGHT"					"0..1"
// STATIC: "TRANSLUCENT"				"0..1"

// DYNAMIC: "WRITEWATERFOGTODESTALPHA"  "0..1"
// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WARPINGIN"					"0..1"
// DYNAMIC: "AMBIENT_LIGHT"				"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..2"	[ps20]
// DYNAMIC: "NUM_LIGHTS"				"0..4"	[ps20b]
// DYNAMIC: "NUM_LIGHTS"				"0..4"	[ps30]

// We don't use other lights when doing the flashlight, so just skip that
// SKIP: ( $FLASHLIGHT != 0 ) && ( $NUM_LIGHTS > 0 ) [PC]

#include "common_flashlight_fxc.h"
#include "common_vertexlitgeneric_dx9.h"

const HALF4 g_EnvmapTint						: register( c0 );
const HALF4 g_DiffuseModulation					: register( c1 );
#if !FLASHLIGHT
		const HALF3 g_EnvmapContrast			: register( c2 );
		const HALF3 g_EnvmapSaturation			: register( c3 );
#endif
const HALF4 g_SelfIllumTint						: register( c4 );
const float3 cAmbientCube[6]					: register( c5 );

// 2 registers each - 6 register total
PixelShaderLightInfo cLightInfo[3]				: register( c13 );  // through c18
const HALF3 g_EyePos							: register( c20 );
const HALF4 g_FogParams							: register( c21 );

#if FLASHLIGHT
const float4 g_FlashlightAttenuationFactors	    : register( c22 );
const HALF3 g_FlashlightPos						: register( c23 );
const float4x4 g_FlashlightWorldToTexture		: register( c24 ); // through c27
#else
const float g_Time : register( c22 );
#endif

sampler BaseTextureSampler		: register( s0 );
sampler EnvmapSampler			: register( s1 );
sampler FlowMapSampler			: register( s2 );
sampler BumpmapSampler			: register( s3 );
sampler EnvmapMaskSampler		: register( s4 );
sampler NormalizeSampler		: register( s5 );
sampler SelfIllumMapSampler		: register( s6 );
sampler FlashlightSampler		: register( s7 );

struct PS_INPUT
{
	HALF4 baseTexCoord2_tangentSpaceVertToEyeVectorXY			: TEXCOORD0;

	// bump mapping and a separate envmap mask texture are mutually exclusive.
	float4 lightAtten											: TEXCOORD1;
	float4 worldVertToEyeVectorXYZ_tangentSpaceVertToEyeVectorZ	: TEXCOORD2;
	float3x3 tangentSpaceTranspose								: TEXCOORD3;
	//	     second row											: TEXCOORD4;
	//	     third row											: TEXCOORD5;
	float4 worldPos_projPosZ									: TEXCOORD6;
	float4 fogFactorW											: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bBaseTexture = BASETEXTURE ? true : false;
	bool bCubemap = CUBEMAP ? true : false;
	bool bDiffuseLighting = DIFFUSELIGHTING ? true : false;
	bool bNormalMapAlphaEnvmapMask = NORMALMAPALPHAENVMAPMASK ? true : false;
	bool bHalfLambert = HALFLAMBERT ? true : false;
	bool bFlashlight = (FLASHLIGHT!=0) ? true : false;
	bool bAmbientLight = AMBIENT_LIGHT ? true : false;
	int nNumLights = NUM_LIGHTS;

	HALF4 baseColor = HALF4( 1.0f, 1.0f, 1.0f, 1.0f );
	if( bBaseTexture )
		baseColor = tex2D( BaseTextureSampler, i.baseTexCoord2_tangentSpaceVertToEyeVectorXY.xy );
	
	float specularFactor = 1.0f;
	HALF4 normalTexel = tex2D( BumpmapSampler, i.baseTexCoord2_tangentSpaceVertToEyeVectorXY.xy );
	HALF3 tangentSpaceNormal = 2.0f * normalTexel - 1.0f;

	if( bNormalMapAlphaEnvmapMask )
		specularFactor = normalTexel.a;

	HALF3 diffuseLighting = HALF3( 1.0f, 1.0f, 1.0f );
	if( bDiffuseLighting )
	{
		float3 worldSpaceNormal = mul( i.tangentSpaceTranspose, tangentSpaceNormal );
		worldSpaceNormal = NormalizeWithCubemap( NormalizeSampler, worldSpaceNormal );
		diffuseLighting = PixelShaderDoLighting( i.worldPos_projPosZ.xyz, worldSpaceNormal,
			   float3( 0.0f, 0.0f, 0.0f ), false, bAmbientLight, i.lightAtten,
			   cAmbientCube, NormalizeSampler, nNumLights, cLightInfo, bHalfLambert,
			   false, 0, false, NormalizeSampler );
	}

	HALF3 albedo = HALF3( 1.0f, 1.0f, 1.0f );
	HALF alpha = 1.0f;
	if( bBaseTexture )
	{
		albedo *= baseColor;
		alpha *= baseColor.a;
	}

	// If we only have specularity, assume that we want a black diffuse component, and
	// get alpha from the envmapmask
	if( !bBaseTexture && bCubemap )
	{
		diffuseLighting = HALF3( 0.0f, 0.0f, 0.0f );
		if( bNormalMapAlphaEnvmapMask )
		{
			alpha *= specularFactor;
		}
	}
	
#if FLASHLIGHT
	if( bFlashlight )
	{
		float3 worldSpaceNormal = mul( i.tangentSpaceTranspose, tangentSpaceNormal );
		float4 flashlightSpacePosition = mul( float4( i.worldPos_projPosZ.xyz, 1.0f ), g_FlashlightWorldToTexture );

		diffuseLighting = DoFlashlight( g_FlashlightPos, i.worldPos_projPosZ.xyz, flashlightSpacePosition,
										worldSpaceNormal, g_FlashlightAttenuationFactors.xyz,
										g_FlashlightAttenuationFactors.w, FlashlightSampler,
										EnvmapMaskSampler, EnvmapMaskSampler, 0, false, false, float2(0, 0), true );
	}
#endif
	
	diffuseLighting *= g_DiffuseModulation.rgb;
	alpha *= g_DiffuseModulation.a;
	
	HALF3 diffuseComponent = albedo * diffuseLighting;
	
	HALF4 flowmapSample = tex2D( FlowMapSampler, i.baseTexCoord2_tangentSpaceVertToEyeVectorXY.xy );
#if !FLASHLIGHT
	flowmapSample.xy += float2( .11, .124 ) * g_Time.xx;
#endif
	HALF4 selfIllumSample = tex2D( SelfIllumMapSampler, flowmapSample.xy );

//	float thing = ( 0.5f * ( cos( g_Time * 3 ) + 1.0f ) );
//	diffuseComponent.xyz += albedo * 10.0f * pow( selfIllumSample.xyz, thing );
	diffuseComponent.xyz += albedo * g_SelfIllumTint.xyz * selfIllumSample.xyz;

	HALF3 specularLighting = HALF3( 0.0f, 0.0f, 0.0f );
#if !FLASHLIGHT
	if( bCubemap )
	{
		// If we've *only* specified a cubemap, blow off the diffuse component
		if ( !bBaseTexture && !bDiffuseLighting && !bFlashlight )
		{
			diffuseComponent = HALF3( 0.0f, 0.0f, 0.0f );
		}

		HALF3 worldSpaceNormal = mul( i.tangentSpaceTranspose, tangentSpaceNormal );

		HALF3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, i.worldVertToEyeVectorXYZ_tangentSpaceVertToEyeVectorZ.xyz );

		specularLighting = ENV_MAP_SCALE * texCUBE( EnvmapSampler, reflectVect );
		specularLighting *= specularFactor;
		specularLighting *= g_EnvmapTint;
		HALF3 specularLightingSquared = specularLighting * specularLighting;
		specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
		HALF3 greyScale = dot( specularLighting, HALF3( 0.299f, 0.587f, 0.114f ) );
		specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
	}
#endif

	HALF3 result = diffuseComponent + specularLighting;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
#if WRITEWATERFOGTODESTALPHA && (PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	alpha = fogFactor;
#endif



//	result.xyz = float3( 1.0f, 1.0f, 1.0f );
#if TRANSLUCENT==0
#	if WARPINGIN
	alpha = 0.0f; // write alpha where the vortigaunt is so that we can selectivly draw pixels when refracting
	#else
	alpha = 1.0f; // write alpha where the vortigaunt is so that we can selectivly draw pixels when refracting
#	endif
#endif

	//FIXME: need to take dowaterfog into consideration
	return FinalOutput( float4( result, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}

