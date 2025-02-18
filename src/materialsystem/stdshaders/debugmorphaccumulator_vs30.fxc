#include "common_vs_fxc.h"

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float2 vTexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 vProjPos			: POSITION;
	float2 vTexCoord		: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.vProjPos = mul( v.vPos, cModelViewProj );
	o.vTexCoord = v.vTexCoord;

	return o;
}