#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler FilmDustSampler   : register( s0 );

const float4 vChannelSelect : register( c0 );

struct PS_INPUT
{
	float2 grainTexCoord	: TEXCOORD0;
};
 
HALF4 main( PS_INPUT i ) : COLOR
{
   HALF4 noise = dot( tex2D( FilmDustSampler, i.grainTexCoord ), vChannelSelect );

   return HALF4( noise.x, noise.y, noise.z, 1.0 );
}
