// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler InputSampler	: register( s0 );
sampler FilmGrain		: register( s1 );

const float4 vNoiseScale : register( c0 );

struct PS_INPUT
{
	float2 inputImageCoords	: TEXCOORD0;	// Input image having grain added
	float2 filmGrainCoords	: TEXCOORD1;	// Tiling film grain texture
};

float4 main( PS_INPUT i ) : COLOR
{
	float3 hsl = RGBtoHSL( tex2D( InputSampler, i.inputImageCoords ) );
	float3 hslNoise = tex2D( FilmGrain, i.filmGrainCoords ) * 2.0f - 1.0f;

	hsl += hslNoise * vNoiseScale * float3( 0.5f, 1.0f, 1.0f );

	return FinalOutput( float4 ( hsl, 1.0f  ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
