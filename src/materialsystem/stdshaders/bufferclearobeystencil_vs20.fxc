#include "common_vs_fxc.h"
// STATIC: "USESCOLOR" "0..1"

struct VS_INPUT
{
	float4 vPos		: POSITION;
#	if (USESCOLOR == 1)
		float4 vColor	: COLOR0;
#	endif
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
#	if (USESCOLOR == 1)
		float4 vColor	: COLOR0;
#	endif
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.vProjPos.xyz = v.vPos.xyz;
	o.vProjPos.w = 1.0f;
	
#	if (USESCOLOR == 1)
	{
		o.vColor = v.vColor;
	}
#	endif	

	return o;
}