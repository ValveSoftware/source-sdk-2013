#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler FilmGrainSampler   : register( s0 );

const float4 vNoiseScale : register( c0 );

struct PS_INPUT
{
	float2 grainTexCoord	: TEXCOORD0;
};
 
HALF4 main( PS_INPUT i ) : COLOR
{
   HALF4 noise = tex2D( FilmGrainSampler, i.grainTexCoord );

   noise.w = noise.x*(1.0-vNoiseScale.w) + vNoiseScale.w;
   noise.xyz *= vNoiseScale.xyz;

   return HALF4( noise.x, noise.y, noise.z, noise.w );
}
