// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler TexSampler0	: register( s0 );
sampler TexSampler1	: register( s1 );
sampler TexSampler2	: register( s2 );
sampler TexSampler3	: register( s3 );

struct PS_INPUT
{
	float2 texCoord	: TEXCOORD0;
};

const float4 weights : register( c0 );

float4 main( PS_INPUT i ) : COLOR
{
	// Just sample the four input textures
	float4 sample0 = tex2D( TexSampler0, i.texCoord );
	float4 sample1 = tex2D( TexSampler1, i.texCoord );
	float4 sample2 = tex2D( TexSampler2, i.texCoord );
	float4 sample3 = tex2D( TexSampler3, i.texCoord );

	// Compute weighted average and return
	return FinalOutput( weights.x * sample0 +
						   weights.y * sample1 +
						   weights.z * sample2 +
						   weights.w * sample3, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
