// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

// This pixel shader compares the luminance against a constant value and retruns all 1's when greater

sampler TexSampler	: register( s0 );

struct PS_INPUT
{
	float2 uv0						: TEXCOORD0;

#if defined( _X360 ) //matching pixel shader inputs to vertex shader outputs to avoid shader patches	
	float2 ZeroTexCoord				: TEXCOORD1;
	float2 bloomTexCoord			: TEXCOORD2;
#endif
};

float3 g_vComparisonMinMaxScale : register( c0 );
#define g_flComparisonMin   g_vComparisonMinMaxScale.x
#define g_flComparisonMax   g_vComparisonMinMaxScale.y
#define g_flComparisonScale g_vComparisonMinMaxScale.z

struct PS_OUTPUT
{
    float4 color : COLOR0;
    float  depth : DEPTH;
};

PS_OUTPUT main( PS_INPUT i )
{
	float3 color = tex2D( TexSampler, i.uv0 ) * g_flComparisonScale;

	// Formula for calculating luminance based on NTSC standard
	float3 tmpv = { 0.2125f, 0.7154f, 0.0721f };
	float flLuminance = dot( color.rgb, tmpv.rgb );

	// Alternate formula for calculating luminance for linear RGB space (Widely used in color hue and saturation computations)
	//float3 tmpv = { 0.3086f, 0.6094f, 0.0820f };
	//float flLuminance = dot( color.rgb, tmpv.rgb );

	// Simple average
	//float flLuminance = ( color.r + color.g + color.b ) * 0.33333f;

	PS_OUTPUT o;
	o.color.rgba = step( g_flComparisonMin, flLuminance ) * step( flLuminance, g_flComparisonMax );
	o.depth = 0.0f;
	return o;
}
