// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
};
	   
HALF4 main( PS_INPUT i ) : COLOR
{
	HALF4 result;
	HALF4 baseColor; 
	HALF maxValue;

	baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );
	maxValue = max( baseColor.r, baseColor.g );
	maxValue = max( baseColor.b, maxValue );
	result = maxValue;
	result.a = 1.0f;
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

