#include "common_vs_fxc.h"

struct VS_INPUT
{
	float3 vPos						: POSITION;
	float2 vScreenCoord				: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 vProjPos					: POSITION;	
	float2 vScreenUV				: TEXCOORD0;
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.vProjPos = float4( v.vPos, 1.0f );
	o.vScreenUV = v.vScreenCoord;

	return o;
}
