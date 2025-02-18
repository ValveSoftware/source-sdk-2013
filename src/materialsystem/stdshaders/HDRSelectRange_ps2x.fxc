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

struct MYHDR_PS_OUTPUT
{
    float4 color[2] : COLOR0;
};

MYHDR_PS_OUTPUT main( PS_INPUT i ) : COLOR
{
	float3 lowColor = GammaToLinear( tex2D( LowSampler, i.texCoord ) );
	float3 hiColor = GammaToLinear( tex2D( HiSampler, i.texCoord ) );

	float4 lowOut;
	lowOut.a = 1.0f;
	float4 hiOut;
	hiOut.a = 1.0f;

	float3 hdrColor = max( lowColor, hiColor * MAX_HDR_OVERBRIGHT );
	
	float fMax = max( hdrColor.b, max( hdrColor.r, hdrColor.g ) );

	float blendFactor = saturate( ( fMax - 0.9f ) * 10.0f );

	blendFactor = 1.0f;

	lowOut.rgb = LinearToGamma( lowColor * ( 1.0f - blendFactor ) );
	hiOut.rgb = LinearToGamma( hiColor * ( blendFactor ) );
	MYHDR_PS_OUTPUT output;
	output.color[0] = lowOut;
	output.color[1] = hiOut;
	return output;
}

