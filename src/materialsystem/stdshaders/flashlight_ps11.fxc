//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

//	STATIC: "NORMALMAP"				"0..1"
//	STATIC: "NOCULL"				"0..1"

#include "common_ps_fxc.h"

sampler SpotSampler					: register( s0 );
sampler BaseTextureSampler			: register( s1 );
sampler NormalizingCubemapSampler	: register( s2 );
// use a normalizing cube map here if we aren't normal mapping
#if NORMALMAP
sampler NormalMapSampler			: register( s3 );
#else
sampler NormalizingCubemapSampler2	: register( s3 );
#endif

static const HALF g_OverbrightFactor = 2.0f;

struct PS_INPUT
{
	float4 spotTexCoord				: TEXCOORD0;
	float2 baseTexCoord				: TEXCOORD1;
#if NORMALMAP
	float3 tangentPosToLightVector	: TEXCOORD2;
	float2 normalMapTexCoord		: TEXCOORD3;
#else
	float3 worldPosToLightVector	: TEXCOORD2;
	float3 normal					: TEXCOORD3;
#endif
	float4 vertAtten				: COLOR0;
};

float4 main( PS_INPUT i ) : COLOR
{
#if NORMALMAP
	float3 normal = tex2D( NormalMapSampler, i.normalMapTexCoord ) * 2.0f - 1.0f;
#else
	float3 normal = texCUBE( NormalizingCubemapSampler2, i.normal ) * 2.0f - 1.0f;
#endif
	
	float3 spotColor = tex2D( SpotSampler, i.spotTexCoord );
	float4 baseSample = tex2D( BaseTextureSampler, i.baseTexCoord );
	float3 baseColor = baseSample.xyz;
#if NORMALMAP
	// wrap this!
	float3 tangentPosToLightVector = texCUBE( NormalizingCubemapSampler, i.tangentPosToLightVector ) * 2.0f - 1.0f;
	float nDotL = saturate( dot( tangentPosToLightVector, normal ) );
#else
	float3 worldPosToLightVector = texCUBE( NormalizingCubemapSampler, i.worldPosToLightVector ) * 2.0f - 1.0f;
	float nDotL = saturate( dot( worldPosToLightVector, normal ) );
#endif	
	float3 outcolor;

	outcolor = spotColor * baseColor * g_OverbrightFactor;

#if !NOCULL
	outcolor *= nDotL;
#endif

	// NOTE!!  This has to be last to avoid loss of range.
	outcolor *= i.vertAtten;

	return float4( outcolor.xyz, baseSample.a * i.vertAtten.a );
}
