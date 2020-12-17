//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

sampler g_texSampler	: register( s0 );

struct PS_INPUT
{
	float2 uv	: TEXCOORD0;
};

float2 g_vPsTapOffsets[2] : register( c0 );

float4 main( PS_INPUT i ) : COLOR
{
	float4 cOut;

	cOut  = 0.25 * tex2D( g_texSampler, i.uv + g_vPsTapOffsets[0] );
	cOut += 0.25 * tex2D( g_texSampler, i.uv - g_vPsTapOffsets[0] );
	cOut += 0.25 * tex2D( g_texSampler, i.uv + g_vPsTapOffsets[1] );
	cOut += 0.25 * tex2D( g_texSampler, i.uv - g_vPsTapOffsets[1] );

	return cOut;
}
