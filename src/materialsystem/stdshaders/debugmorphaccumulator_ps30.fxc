#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 vTexCoord		: TEXCOORD0;
};

sampler MorphAccumulator		: register( s0 );

HALF4 main( PS_INPUT i ) : COLOR
{
	float4 vDeltas = tex2D( MorphAccumulator, i.vTexCoord );
	return float4( abs( vDeltas.x ), abs( vDeltas.z ), abs( vDeltas.y ) + abs( vDeltas.w ), 1.0f );
}

