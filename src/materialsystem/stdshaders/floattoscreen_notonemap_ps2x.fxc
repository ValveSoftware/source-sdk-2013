// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_FLOAT
#define HDRENABLED 1
#include "common_ps_fxc.h"

sampler FBSampler	: register( s0 );

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
};

#define MAX_LUM 5
#define MIN_LUM .1

float4 main( PS_INPUT i ) : COLOR
{
	float4 fbSample = tex2D( FBSampler, i.texCoord );

	float lum=0.33*(fbSample.x+fbSample.y+fbSample.z);
	// derive a luminance-based scale factor for tone mapping
	float factor=1.0;
	if (lum<MIN_LUM)
	{
	}
	else if (lum>MAX_LUM)
	{
	}


	// linear scale for tone mapping
//	fbSample *= lum * LINEAR_LIGHT_SCALE;

	// assume that sRGB write is enabled.	
	return FinalOutput( fbSample, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
