#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler FilmDustSampler   : register( s0 );

const float4 vChannelSelect : register( c0 );

struct PS_INPUT
{
	float2 grainTexCoord	: TEXCOORD0;
};
 
float4 main( PS_INPUT i ) : COLOR
{
   float4 noise = dot( tex2D( FilmDustSampler, i.grainTexCoord ), vChannelSelect );

   return float4( noise.x, noise.y, noise.z, 1.0 );
}
