//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

#include "common_ps_fxc.h"

sampler TexSampler : register( s0 );
sampler TexRed : register( s1 );
sampler TexGreen : register( s2 );
sampler TexBlue : register( s3 );

float2 g_vPixelSize : register( c4 );

struct PS_INPUT
{
	float2 baseTexCoord : TEXCOORD0; // Base texture coordinate
};

float4 main( PS_INPUT i ) : COLOR
{
	// Make outline 2 pixels wide $C0_X in a vmt doesn't seem to work in staging so I can't parameterize this right now
	float2 vOffset = 2.0f * g_vPixelSize.xy;

	// Take the max of 4 offset texture samples
	float4 result;
	result.rgba = tex2D( TexSampler, i.baseTexCoord.xy + float2( vOffset.x, vOffset.y ) );
	result.rgba = max( result.rgba, tex2D( TexSampler, i.baseTexCoord.xy + float2( vOffset.x, -vOffset.y ) ) );
	result.rgba = max( result.rgba, tex2D( TexSampler, i.baseTexCoord.xy + float2( -vOffset.x, vOffset.y ) ) );
	result.rgba = max( result.rgba, tex2D( TexSampler, i.baseTexCoord.xy + float2( -vOffset.x, -vOffset.y ) ) );

	// Store max color component in alpha for alpha blend of one/invSrcAlpha
	float flLuminance = max( result.r, max( result.g, result.b ) );
	result.a = flLuminance;

	return result.rgba;
}
