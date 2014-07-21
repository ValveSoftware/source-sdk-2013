#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD3;
};

sampler BaseTextureSampler	: register( s0 );
sampler DetailTextureSampler : register( s3 );

float4 main( PS_INPUT i ) : COLOR
{
	// Sample frames from texture 0
	float4 base= tex2D( BaseTextureSampler, i.texCoord0 );
  	float4 detail=tex2D( DetailTextureSampler, i.texCoord1 );

	return float4(base.rgb, base.a * detail.a);
}
