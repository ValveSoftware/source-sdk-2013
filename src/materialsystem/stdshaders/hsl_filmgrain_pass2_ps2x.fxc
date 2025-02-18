// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler InputSampler	: register( s0 );

struct PS_INPUT
{
	float2 inputImageCoords	: TEXCOORD0;	// Input image
	float2 filmGrainCoords	: TEXCOORD1;	// Tiling film grain texture
};

float4 main( PS_INPUT i ) : COLOR
{
	return FinalOutput( HSLtoRGB( tex2D( InputSampler, i.inputImageCoords ) ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

