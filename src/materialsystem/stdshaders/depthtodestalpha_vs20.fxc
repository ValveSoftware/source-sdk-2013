#include "common_vs_fxc.h"

struct VS_INPUT
{
	float4 vPos		: POSITION;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float projPosZ		: TEXCOORD0;
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.vProjPos = mul( v.vPos, cModelViewProj );
	o.projPosZ = o.vProjPos.z;

	return o;
}
