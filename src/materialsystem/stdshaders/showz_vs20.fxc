//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "common_vs_fxc.h"

struct VS_INPUT
{
	// If this is float4, and the input is float3, the w component defaults to one.
	float4 vPos				: POSITION; 
	float2 vBaseTexCoords	: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	
    float2 vBaseTexCoords	: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	float4 projPos;

	projPos = mul( v.vPos, cModelViewProj );
	o.projPos = projPos;

	o.vBaseTexCoords = v.vBaseTexCoords;	
	return o;
}
