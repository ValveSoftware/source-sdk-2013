//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "common_vs_fxc.h"

struct VS_INPUT
{
	// If this is float4, and the input is float3, the w component default to one.
	float4 vPos							: POSITION; 
	float4 vColor						: COLOR0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
    float4 vColor					: COLOR0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	float4 projPos;

	projPos = mul( v.vPos, cModelViewProj );
	o.projPos = projPos;

	o.vColor = v.vColor;	
	return o;
}
