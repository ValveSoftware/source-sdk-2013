//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "DIFFUSELIGHTING"	"0..1"
// STATIC: "HALFLAMBERT"		"0..1"

// DYNAMIC: "AMBIENT_LIGHT"		"0..1"
// DYNAMIC: "NUM_LIGHTS"		"0..2"	[ps20]
// DYNAMIC: "NUM_LIGHTS"		"0..4"	[ps20b]

#define HDRTYPE HDR_TYPE_NONE
#include "common_vertexlitgeneric_dx9.h"

const float4 g_OverbrightFactor		: register( c4 );
const float3 cAmbientCube[6]		: register( c6 );

PixelShaderLightInfo cLightInfo[3]	: register(c13);

sampler BumpmapSampler				: register( s0 );
sampler NormalizeSampler			: register( s1 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
	// detail textures and bumpmaps are mutually exclusive so that we have enough texcoords.
	float2 detailOrBumpTexCoord		: TEXCOORD1;
	// bump mapping and a separate envmap mask texture are mutually exclusive.
	float2 envmapMaskTexCoord		: TEXCOORD2;
	float3 worldVertToEyeVector		: TEXCOORD3;
	float3x3 tangentSpaceTranspose	: TEXCOORD4;
	float4 worldPos_projPosZ		: TEXCOORD5;
	float2 lightAtten01				: TEXCOORD6;
	float2 lightAtten23				: TEXCOORD7;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bDiffuseLighting = DIFFUSELIGHTING ? true : false;
	bool bHalfLambert = HALFLAMBERT ? true : false;
	bool bAmbientLight = AMBIENT_LIGHT ? true : false;
	int nNumLights = NUM_LIGHTS;

	float4 vLightAtten = float4( i.lightAtten01, i.lightAtten23 );

	float3 tangentSpaceNormal = float3( 0.0f, 0.0f, 1.0f );
	float4 normalTexel = 1.0f;
	float4 baseColor = float4( 1.0f, 1.0f, 1.0f, 1.0f );

	normalTexel = tex2D( BumpmapSampler, i.detailOrBumpTexCoord );
	tangentSpaceNormal = 2.0f * normalTexel - 1.0f;

	float3 diffuseLighting = float3( 1.0f, 1.0f, 1.0f );
	if( bDiffuseLighting )
	{
		float3 worldSpaceNormal = mul( i.tangentSpaceTranspose, tangentSpaceNormal );
		float3 staticLightingColor = float3( 0.0f, 0.0f, 0.0f );
		diffuseLighting = PixelShaderDoLighting( i.worldPos_projPosZ.xyz, worldSpaceNormal,
			   float3( 0.0f, 0.0f, 0.0f ), false, bAmbientLight,
			   vLightAtten, cAmbientCube, NormalizeSampler, nNumLights, cLightInfo, bHalfLambert,
			   false, 0, false, NormalizeSampler );
		// multiply by .5 since we want a 50% (in gamma space) reflective surface)
		diffuseLighting *= pow( 0.5f, 2.2f );
	}
	
	return FinalOutput( float4( diffuseLighting, 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

