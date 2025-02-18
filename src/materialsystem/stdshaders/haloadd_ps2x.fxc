//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

#include "common_ps_fxc.h"

sampler TexSampler : register( s0 );

float g_flDimValue : register( c0 );

struct PS_INPUT
{
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 result = tex2D( TexSampler, i.baseTexCoord );

	// Scale by dim value before computing luminance below
	result.rgb *= saturate( g_flDimValue );

	// Store max color component in alpha for alpha blend of one/invSrcAlpha
	float flLuminance = max( result.r, max( result.g, result.b ) );
	result.a = pow( flLuminance, 0.8f );

	return result.rgba;
}
