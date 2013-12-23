// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#include "common_ps_fxc.h"

sampler TextureSampler	: register( s0 );

struct PS_INPUT
{
	float4 vColor0		: COLOR0;
	float2 vTexCoord0	: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 result = i.vColor0 * tex2D( TextureSampler, i.vTexCoord0 );
	
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}