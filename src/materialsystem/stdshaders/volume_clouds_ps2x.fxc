//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

// Includes =======================================================================================
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tInnerSampler		: register( s0 );
sampler g_tMiddleSampler	: register( s1 );
sampler g_tOuterSampler		: register( s2 );

// Shaders Constants and Globals ==================================================================
//const float4 g_vPackedConst6		: register( c6 );
//#define g_flTime          g_vPackedConst6.w

// Interpolated values ============================================================================
struct PS_INPUT
{
	float4 v2DTangentViewVector01		: TEXCOORD0;
	float4 vUv01						: TEXCOORD1;
	float4 v2DTangentViewVector2_vUv2	: TEXCOORD2;
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	float4 vFinalColor = float4( 0.0f, 0.0f, 0.0f, 1.0f );

#if defined(SHADER_MODEL_PS_2_0)
	float flNumLayers = 2.0f;
#else
	float flNumLayers = 10.0f;
#endif

	//float flColorDim = 1.0f;
	for ( float j=flNumLayers-1.0f; j>=0.0f; j-=1.0f ) // From hightest to lowest layer
	{
		float4 vInnerTexel = tex2D( g_tInnerSampler, saturate( i.vUv01.xy + i.v2DTangentViewVector01.xy * 0.005 * j ) );
		float4 vMiddleTexel = tex2D( g_tMiddleSampler, saturate( i.vUv01.wz + i.v2DTangentViewVector01.wz * 0.005 * j ) );
		float4 vOuterTexel = tex2D( g_tOuterSampler, saturate( i.v2DTangentViewVector2_vUv2.wz + i.v2DTangentViewVector2_vUv2.xy * 0.005 * j ) );

		float4 vThisTexel;
		vThisTexel.rgb = ( vInnerTexel.rgb * vInnerTexel.a ) + ( vMiddleTexel.rgb * vMiddleTexel.a ) + ( vOuterTexel.rgb * vOuterTexel.a );
		vThisTexel.a = 1.0f - ( ( 1.0f - vOuterTexel.a ) * ( 1.0f - vMiddleTexel.a ) * ( 1.0f - vInnerTexel.a ) );

		//vThisTexel.rgb *= flColorDim;
		//flColorDim *= 0.95f;

		// 5.0 and 0.8625 are magic numbers that look good with the current textures
		float flBlendValue = saturate( pow( vThisTexel.a, lerp( 5.0f, 0.8625f, saturate( j/(flNumLayers-1.0f) ) ) ) );

		vFinalColor.rgb = vThisTexel.rgb + ( vFinalColor.rgb * ( 1.0f - flBlendValue ) );
		vFinalColor.a *= 1.0f - flBlendValue; // Dest alpha scalar
	}

	//===============//
	// Combine terms //
	//===============//
	float4 result;
	result.rgb = vFinalColor.rgb;
	result.a = 1.0f - vFinalColor.a;

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR ); //go back to final output when it'll fit.
}
