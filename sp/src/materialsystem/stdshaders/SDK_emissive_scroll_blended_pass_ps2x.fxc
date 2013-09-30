//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// Includes =======================================================================================
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps30][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tBaseSampler		: register( s0 );
sampler g_tFlowSampler		: register( s1 );
sampler g_tSelfIllumSampler	: register( s2 );

// Shaders Constants and Globals ==================================================================
const float4 g_vPackedConst0 : register( c0 );
#define g_flBlendStrength   g_vPackedConst0.x
#define g_flTime			g_vPackedConst0.y

const float2 g_vEmissiveScrollVector : register( c1 );
const float3 g_cSelfIllumTint : register( c2 );

// Interpolated values ============================================================================
struct PS_INPUT
{
	float2 vTexCoord0 : TEXCOORD0;
};

// Main ===========================================================================================
//float4 main( PS_INPUT i ) : COLOR // Non-HDR for debugging
float4 main( PS_INPUT i ) : COLOR
{
	// Color texture
	float4 cBaseColor = tex2D( g_tBaseSampler, i.vTexCoord0.xy );

	// Fetch from dudv map and then fetch from emissive texture with new uv's & scroll
	float4 vFlowValue = tex2D( g_tFlowSampler, i.vTexCoord0.xy );
	float2 vEmissiveTexCoord = vFlowValue.xy + ( g_vEmissiveScrollVector.xy * g_flTime );
	float4 cEmissiveColor = tex2D( g_tSelfIllumSampler, vEmissiveTexCoord.xy );

	//===============//
	// Combine terms //
	//===============//
	float4 result;
	result.rgb = cBaseColor.rgb * cEmissiveColor.rgb * g_cSelfIllumTint.rgb;
	result.rgb *= g_flBlendStrength;

	// Set alpha to 0.0f so it doesn't change dest alpha (I should probably disable dest alpha writes)
	result.a = 0.0f;

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR );
}
