#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );
sampler CloudAlphaSampler	: register( s1 );

struct PS_INPUT
{
	float2 baseCoords		: TEXCOORD0;
	float2 cloudAlphaCoords	: TEXCOORD1;
	float  fogFactor		: TEXCOORD2;
};
 
float4 main( PS_INPUT i ) : COLOR
{
	float4 vBase = tex2D( BaseTextureSampler, i.baseCoords );
	float4 vCloudAlpha = tex2D( CloudAlphaSampler, i.cloudAlphaCoords );

	float fogFactor = 2.0f * smoothstep( 0.3f, 0.6f, i.fogFactor );

	float4 result = vBase * vCloudAlpha;
	result.a *= fogFactor;

	// No actual fog.  Use the "fog factor" to modulate alpha (after some smoothstep mojo)
	return FinalOutput( result, 1.0f, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR );
}
