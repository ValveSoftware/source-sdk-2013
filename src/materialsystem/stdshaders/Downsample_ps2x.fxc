// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler TexSampler	: register( s0 );

struct PS_INPUT
{
	float2 coordTap0				: TEXCOORD0;
	float2 coordTap1				: TEXCOORD1;
	float2 coordTap2				: TEXCOORD2;
	float2 coordTap3				: TEXCOORD3;
};

float4 main( PS_INPUT i ) : COLOR
{
	float3 s0, s1, s2, s3;

	// Sample 4 taps
	s0 = tex2D( TexSampler, i.coordTap0);
	s1 = tex2D( TexSampler, i.coordTap1);
	s2 = tex2D( TexSampler, i.coordTap2);
	s3 = tex2D( TexSampler, i.coordTap3);

	float3 avgColor = ( s0 + s1 + s2 + s3 ) * 0.25f;
	return FinalOutput( float4( avgColor, 1 ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

