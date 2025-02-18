// STATIC: "CONVERT_TO_SRGB"		"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"		"0..0"	[= 0] [XBOX]
// STATIC: "MULTITEXTURE"			"0..1"
// STATIC: "FRESNEL"				"0..1"
// STATIC: "BLEND"					"0..1"
// STATIC: "REFRACTALPHA"			"0..1"
// STATIC: "HDRTYPE"				"0..2"
// STATIC: "NORMAL_DECODE_MODE"		"0..0"	[XBOX]
// STATIC: "NORMAL_DECODE_MODE"		"0..0"	[PC]

// DYNAMIC: "HDRENABLED"			"0..1"
// DYNAMIC: "PIXELFOGTYPE"			"0..1"

#include "common_ps_fxc.h"

const HALF3 g_WaterFogColor			: register( c0 );
const HALF4 g_CheapWaterParams		: register( c1 );
const HALF4 g_ReflectTint			: register( c2 );
const float4 g_PixelFogParams		: register( c3 );

#define g_CheapWaterStart			g_CheapWaterParams.x
#define g_CheapWaterEnd				g_CheapWaterParams.y
#define g_CheapWaterDeltaRecip		g_CheapWaterParams.z
#define g_CheapWaterStartDivDelta	g_CheapWaterParams.w

sampler EnvmapSampler		: register( s0 );
sampler NormalMapSampler	: register( s1 );
#if REFRACTALPHA
sampler RefractSampler		: register( s2 );
#endif
sampler NormalizeSampler	: register( s6 );

struct PS_INPUT
{
	float2 normalMapTexCoord		: TEXCOORD0;
	HALF3 worldSpaceEyeVect			: TEXCOORD1;
	HALF3x3 tangentSpaceTranspose	: TEXCOORD2;
	float4 vRefract_W_ProjZ			: TEXCOORD5;

#if MULTITEXTURE
	float4 vExtraBumpTexCoord		: TEXCOORD6;
#endif
	float4 fogFactorW				: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bBlend = BLEND ? true : false;

#if MULTITEXTURE
	float3 vNormal  = tex2D( NormalMapSampler, i.normalMapTexCoord );
	float3 vNormal1 = tex2D( NormalMapSampler, i.vExtraBumpTexCoord.xy );
	float3 vNormal2 = tex2D( NormalMapSampler, i.vExtraBumpTexCoord.zw );
	vNormal = 0.33 * ( vNormal + vNormal1 + vNormal2 );

#if ( NORMAL_DECODE_MODE == NORM_DECODE_ATI2N )
	vNormal.xy = vNormal.xy * 2.0f - 1.0f;
	vNormal.z = sqrt( 1.0f - dot(vNormal.xy, vNormal.xy) );
#else
	vNormal = 2.0 * vNormal - 1.0;
#endif

#else
	float3 vNormal = DecompressNormal( NormalMapSampler, i.normalMapTexCoord, NORMAL_DECODE_MODE );
#endif
	
	HALF3 worldSpaceNormal = mul( vNormal, i.tangentSpaceTranspose );
	HALF3 worldSpaceEye;
	
	HALF flWorldSpaceDist = 1.0f;	
	
#ifdef NV3X
	// for some reason, fxc doesn't convert length( half3 v ) into all _pp opcodes.
	if (bBlend) 
	{
		worldSpaceEye = i.worldSpaceEyeVect;
		HALF  worldSpaceDistSqr = dot( worldSpaceEye, worldSpaceEye );
		HALF  rcpWorldSpaceDist = rsqrt( worldSpaceDistSqr );
		worldSpaceEye *= rcpWorldSpaceDist;
		flWorldSpaceDist = worldSpaceDistSqr * rcpWorldSpaceDist;
	}
	else
	{
		worldSpaceEye  = NormalizeWithCubemap( NormalizeSampler, i.worldSpaceEyeVect );
	}
#else  // !NV3X
	if (bBlend) 
	{
		worldSpaceEye = i.worldSpaceEyeVect;
		flWorldSpaceDist = length( worldSpaceEye );
		worldSpaceEye /= flWorldSpaceDist;
	}
	else
	{
		worldSpaceEye  = NormalizeWithCubemap( NormalizeSampler, i.worldSpaceEyeVect );
	}
#endif
	
	HALF3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, worldSpaceEye );
	HALF3 specularLighting = ENV_MAP_SCALE * texCUBE( EnvmapSampler, reflectVect );
	specularLighting *= g_ReflectTint;
	
#if FRESNEL
	// FIXME: It's unclear that we want to do this for cheap water
	// but the code did this previously and I didn't want to change it
	HALF flDotResult = dot( worldSpaceEye, worldSpaceNormal ); 
	flDotResult = 1.0f - max( 0.0f, flDotResult );

	HALF flFresnelFactor = flDotResult * flDotResult;
	flFresnelFactor *= flFresnelFactor;
	flFresnelFactor *= flDotResult;
#else
	HALF flFresnelFactor = g_ReflectTint.a;
#endif
	
	HALF flAlpha;
	if (bBlend)
	{
		HALF flReflectAmount = saturate( flWorldSpaceDist * g_CheapWaterDeltaRecip - g_CheapWaterStartDivDelta );
		flAlpha = saturate( flFresnelFactor + flReflectAmount );

#if REFRACTALPHA
		// Perform division by W only once
		float ooW = 1.0f / i.vRefract_W_ProjZ.z;
		float2 unwarpedRefractTexCoord = i.vRefract_W_ProjZ * ooW;
		float fogDepthValue = tex2D( RefractSampler, unwarpedRefractTexCoord ).a;
		// Fade on the border between the water and land.
		flAlpha *= saturate( ( fogDepthValue - .05f ) * 20.0f );
#endif
	}
	else
	{
		flAlpha = 1.0f;
#if HDRTYPE == 0 || HDRENABLED == 0
		specularLighting = lerp( g_WaterFogColor, specularLighting, flFresnelFactor );
#else
		specularLighting = lerp( GammaToLinear( g_WaterFogColor ), specularLighting, flFresnelFactor );
#endif
	}

	// multiply the color by alpha.since we are using alpha blending to blend against dest alpha for borders.
	
	
		
#if (PIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE)
	float fogFactor = CalcRangeFog( i.vRefract_W_ProjZ.w, g_PixelFogParams.x, g_PixelFogParams.z, g_PixelFogParams.w );
#else
	float fogFactor = 0;
#endif
	
	return FinalOutput( float4( specularLighting, flAlpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}
