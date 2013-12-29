// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "MRTINDEX"			"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 baseTexCoord			: TEXCOORD0;
};

sampler BaseTextureSampler1	: register( s0 );
sampler BaseTextureSampler2	: register( s1 );

float4 main( PS_INPUT i ) : COLOR
{
#if MRTINDEX == 0
	float4 result = tex2D( BaseTextureSampler1, i.baseTexCoord );
#else
	float4 result = tex2D( BaseTextureSampler2, i.baseTexCoord );
#endif

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

