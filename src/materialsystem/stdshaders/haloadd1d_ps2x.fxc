//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

#include "common_ps_fxc.h"

sampler TexSampler : register( s0 );
sampler TexRed : register( s1 );
sampler TexGreen : register( s2 );
sampler TexBlue : register( s3 );

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

	// Fetch into 1D textures based on the intensity of each color channel and sum
	float4 vRed = tex2D( TexRed, pow( result.r, 0.45 ) );
	float4 vGreen = tex2D( TexGreen, pow( result.g, 0.45 ) );
	float4 vBlue = tex2D( TexBlue, pow( result.b, 0.45 ) );
	result.rgb = vRed.rgb + vGreen.rgb + vBlue.rgb;

	// Store max color component in alpha for alpha blend of one/invSrcAlpha
	float flLuminance = max( result.r, max( result.g, result.b ) );
	result.a = flLuminance;

	return result.rgba;
}
