//  DYNAMIC: "COMPRESSED_VERTS"			"0..1"

#include "common_vs_fxc.h"

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float4 vTexCoord0	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float2 vUv0			: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT i )
{
	VS_OUTPUT o;
	o.vProjPos.xyzw = mul( i.vPos.xyzw, cModelViewProj );
	o.vUv0.xy = i.vTexCoord0.xy;
 	return o;
}
