// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler FBSampler	: register( s0 );
sampler BlurSampler	: register( s1 );

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 fbSample = tex2D( FBSampler, i.texCoord );
	float4 blurSample = tex2D( BlurSampler, i.texCoord );

	return FinalOutput( float4( fbSample + blurSample.rgb * blurSample.a * MAX_HDR_OVERBRIGHT, 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}