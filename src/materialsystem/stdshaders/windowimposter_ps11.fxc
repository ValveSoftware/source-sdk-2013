//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "common_ps_fxc.h"

sampler EnvmapSampler		: register( s0 );

struct PS_INPUT
{
	float3 eyeToVertVector			: TEXCOORD0;
	float4 vertexColor				: COLOR;
};

HALF4 main( PS_INPUT i ) : COLOR
{
	HALF4 color;
	color.xyz = texCUBE( EnvmapSampler, i.eyeToVertVector );
	color.a = 1.0f;
	color *= i.vertexColor;
	return color;
}
