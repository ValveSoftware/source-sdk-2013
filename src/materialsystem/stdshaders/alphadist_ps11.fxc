#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD3;
};

sampler BaseTextureSampler	: register( s0 );
sampler DetailTextureSampler : register( s3 );

const float4 g_OutlineColor	: register( c0 );
const float4 g_OutlineEnd : register( c1 );			// a comp
const float4 g_OutlineStart : register( c2 );			// a comp

float4 main( PS_INPUT i ) : COLOR
{
	// Sample frames from texture 0
	float4 base= tex2D( BaseTextureSampler, i.texCoord0 );
  	float4 detail=tex2D( DetailTextureSampler, i.texCoord1 );
	if ( detail.a < g_OutlineEnd.a )
		base = g_OutlineColor;
  	base.a *= ( detail.a > g_OutlineStart.a );
	return base;
}

