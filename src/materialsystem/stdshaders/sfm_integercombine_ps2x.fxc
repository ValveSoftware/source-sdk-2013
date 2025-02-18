// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler TexSampler0	: register( s0 );
sampler TexSampler1	: register( s1 );

struct PS_INPUT
{
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;
};

const float4 bloomamount : register( c0 );

float4 main( PS_INPUT i ) : COLOR
{
	// Just sample the input textures
	float4 c0 = tex2D( TexSampler0, i.tc0 );
	float4 c1 = tex2D( TexSampler1, i.tc1 );

	// Add in bloom and return
	return FinalOutput( c0 + bloomamount.xxxx * c1, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

