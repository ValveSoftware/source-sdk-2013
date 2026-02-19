#include "common_ps_fxc.h"

sampler TexSampler : register( s0 );
sampler TexRed : register( s1 );
sampler TexGreen : register( s2 );
sampler TexBlue : register( s3 );
float2 g_vPixelSize : register( c4 );
float flOutlineRadius : register( c5 );

struct PS_INPUT
{
	float2 baseTexCoord : TEXCOORD0; // Base texture coordinate
};

float4 main( PS_INPUT i ) : COLOR
{
	// Get the size of one pixel in texture coordinates
	float2 vPixelOffset = g_vPixelSize.xy;
	int iRadius = (int)max(1.0f, flOutlineRadius); // Ensure radius is at least 1

	// Initialize result with the center pixel's value
	float4 result = tex2D( TexSampler, i.baseTexCoord.xy );

	// Sample the neighborhood defined by the radius and find the maximum
	for (int y = -iRadius; y <= iRadius; ++y)
	{
		for (int x = -iRadius; x <= iRadius; ++x)
		{
			// Skip the center pixel as it's already initialized
			if (x == 0 && y == 0)
				continue;

			float2 vOffset = float2(x, y) * vPixelOffset;
			result = max( result, tex2D( TexSampler, i.baseTexCoord.xy + vOffset ) );
		}
	}

	// Store max color component in alpha for alpha blend of one/invSrcAlpha
	float flLuminance = max( result.r, max( result.g, result.b ) );
	result.a = flLuminance;

	return result.rgba;
}
