// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 texCoord0	:	TEXCOORD0;
	float3 color		:	COLOR;
};

sampler BaseTextureSampler	: register( s0 );

HALF4 main( PS_INPUT i ) : COLOR
{
	float4 baseTex = tex2D( BaseTextureSampler, i.texCoord0 );	
	return FinalOutput( baseTex * float4( i.color, 1 ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

