#include "common_vs_fxc.h"

struct VS_INPUT
{
	float2 vSrcCoord		: TEXCOORD0;
	float4 vMorphWeights	: TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 vDestCoord			: POSITION;
	float4 vMorphWeights		: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	// FIXME: Want these to be in pixel centers!
	float2 projSpace = v.vSrcCoord.xy;
	projSpace *= 2.0f;
	projSpace -= 1.0f;
	projSpace.y *= -1.0f;
	
	o.vDestCoord = float4( projSpace.x, projSpace.y, 0.0f, 1.0f );
	o.vMorphWeights	= v.vMorphWeights;

	return o;
}