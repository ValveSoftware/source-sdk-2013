// ======= Copyright © 1996-2007, Valve Corporation, All rights reserved. ======
//
// Run procedural glint generation inner loop in pixel shader (ps_2_0)
//
// =============================================================================

struct PS_INPUT
{
	float2 tc			: TEXCOORD0;	// Interpolated coordinate of current texel
	float2 glintCenter	: TEXCOORD1;	// Uniform value containing center of glint
	float3 glintColor	: TEXCOORD2;	// Uniform value of color of glint
};

float GlintGaussSpotCoefficient( float2 d )
{
	return saturate( exp( -25.0f * dot(d, d) ) );
}

float4 main( PS_INPUT i ) : COLOR
{
	float2 uv = i.tc - i.glintCenter;	// This texel relative to glint center

	float intensity =	GlintGaussSpotCoefficient( uv + float2(-0.25f, -0.25f) ) + 
						GlintGaussSpotCoefficient( uv + float2( 0.25f, -0.25f) ) + 
					5 * GlintGaussSpotCoefficient( uv ) + 
						GlintGaussSpotCoefficient( uv + float2(-0.25f,  0.25f) ) + 
						GlintGaussSpotCoefficient( uv + float2( 0.25f,  0.25f) );
			
	intensity *= 4.0f/9.0f;

	return float4( intensity * i.glintColor, 1.0f );
}