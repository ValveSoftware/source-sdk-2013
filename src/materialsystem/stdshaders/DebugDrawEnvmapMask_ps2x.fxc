// DYNAMIC: "SHOWALPHA"			"0..1"

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );

struct PS_INPUT
{
    float4 projPos					: POSITION;	
	float2 baseTexCoord				: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );
#if SHOWALPHA
	float4 result = float4( baseColor.a, baseColor.a, baseColor.a, 1.0f );
#else
	float4 result = float4( baseColor.rgb, 1.0f );
#endif
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
