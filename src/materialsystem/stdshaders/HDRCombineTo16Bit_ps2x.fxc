// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler LowSampler	: register( s0 );
sampler HiSampler	: register( s1 );

struct PS_INPUT
{
	float2 texCoord : TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 lowColor = tex2D( LowSampler, i.texCoord );
	float3 hiColor = tex2D( HiSampler, i.texCoord );

	lowColor.rgb = GammaToLinear( lowColor.rgb );
	hiColor.rgb = GammaToLinear( hiColor.rgb );

	float4 result = float4( ( 1.0f / MAX_HDR_OVERBRIGHT ) * max( lowColor.xyz, hiColor.xyz * MAX_HDR_OVERBRIGHT ), lowColor.a );
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
