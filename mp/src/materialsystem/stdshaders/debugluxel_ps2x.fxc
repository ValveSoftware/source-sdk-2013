#include "common_ps_fxc.h"

sampler TextureSampler		: register( s0 );

struct PS_INPUT
{
	float2 baseTexCoord		: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 result = tex2D( TextureSampler, i.baseTexCoord );
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
 
