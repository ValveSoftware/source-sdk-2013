//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. ===========================

// Includes =======================================================================================
#include "common_vs_fxc.h"

// Input values ===================================================================================
struct VS_INPUT
{
	float3 vPos				: POSITION;
	float2 vBaseTexCoord	: TEXCOORD0;
};

// Interpolated values ============================================================================
struct VS_OUTPUT
{
    float4 projPos	: POSITION;	
	float2 vUv0		: TEXCOORD0;
};

// Main ===========================================================================================
VS_OUTPUT main( const VS_INPUT i )
{
	VS_OUTPUT o;

	o.projPos.xyzw = float4( i.vPos.xyz, 1.0f );
	o.vUv0.xy = i.vBaseTexCoord.xy;

	return o;
}
