#include "common_vs_fxc.h"

struct VS_INPUT
{
	float3 vPos							: POSITION;
	float2 vBaseTexCoord				: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
	float2 baseTexCoord				: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float4 projPos;
	float3 worldPos;

	projPos = mul( float4( v.vPos, 1 ), cModelViewProj );
	o.projPos = projPos;

	o.baseTexCoord = v.vBaseTexCoord;
	return o;
}


