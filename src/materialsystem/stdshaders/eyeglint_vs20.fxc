//===== Copyright © 1996-2007, Valve Corporation, All rights reserved. ======//
//
// Vertex shader to pass through texcoords needed to run the
// procedural glint generation inner loop in the pixel shader
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "common_vs_fxc.h"

struct VS_INPUT
{
	float3 vPos			: POSITION;
	float2 tc			: TEXCOORD0;	// Interpolated coordinate of current texel in 3x3 quad
	float2 glintCenter	: TEXCOORD1;	// Uniform value containing center of glint in local 3x3 quad
	float3 glintColor	: TEXCOORD2;	// Uniform value of color of glint
};

struct VS_OUTPUT
{
    float4 projPos		: POSITION;	
	float2 tc			: TEXCOORD0;	// Interpolated coordinate of current texel in 3x3 quad
	float2 glintCenter	: TEXCOORD1;	// Uniform value containing center of glint in local 3x3 quad
	float3 glintColor	: TEXCOORD2;	// Uniform value of color of glint
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;
	o.projPos = float4( v.vPos, 1.0f );
	o.tc = v.tc;
	o.glintCenter = v.glintCenter;
	o.glintColor = v.glintColor;
	return o;
}


