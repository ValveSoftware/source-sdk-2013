#include "common_vs_fxc.h"


struct VS_INPUT
{
	float4 vPos							: POSITION;
	float2 vBaseTexCoord				: TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 vProjPos					: POSITION;	
	float2 vBaseTexCoord			: TEXCOORD0;
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT	o;

	o.vProjPos = v.vPos;
	o.vBaseTexCoord = v.vBaseTexCoord;

	return o;
}
