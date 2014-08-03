//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
// Includes =======================================================================================
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tBaseSampler		: register( s0 );
sampler g_tNoiseSampler		: register( s1 );
sampler g_tBorder1DSampler	: register( s2 );
sampler g_tNormalSampler	: register( s3 );
sampler g_tSubsurfaceSampler: register( s4 );
sampler g_tCubeSampler		: register( s5 );

// Shaders Constants and Globals ==================================================================
const float3 g_cSubsurfaceTint  : register( c0 );
const float2 g_flBorderWidth    : register( c1 ); //{ 1.0f / g_flBorderWidthFromVmt, ( 1.0f / g_flBorderWidthFromVmt ) - 1.0f };
const float g_flBorderSoftness  : register( c2 );
const float3 g_cBorderTint      : register( c3 );
const float g_flGlobalOpacity   : register( c4 );
const float g_flGlossBrightness : register( c5 );

// Interpolated values ============================================================================
struct PS_INPUT
{
	float2 vTexCoord0 : TEXCOORD0;
	float2 flDistanceToEffectCenter_flFresnelEffect	: TEXCOORD1;
	float4 vNoiseTexCoord		: TEXCOORD2;
	float3 vTangentViewVector	: TEXCOORD3;
	float3 cVertexLight         : TEXCOORD4;
	float3x3 mTangentSpaceTranspose : TEXCOORD5;
	//	     second row				: TEXCOORD6;
	//	     third row				: TEXCOORD7;
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	// Color texture
    float4 cBaseColor = tex2D( g_tBaseSampler, i.vTexCoord0.xy );
	float flFleshMaskFromTexture = cBaseColor.a;

	// Subsurface colors
    float4 cSubsurfaceColor = tex2D( g_tSubsurfaceSampler, i.vTexCoord0.xy );
	cBaseColor.rgb += cBaseColor.rgb * cSubsurfaceColor.rgb * g_cSubsurfaceTint.rgb;

	// Scroll noise textures to ripple border of opening
	float flNoise0 = tex2D( g_tNoiseSampler, i.vNoiseTexCoord.xy ).g; // Use green so we can DXT1 if we want
	float flNoise1 = tex2D( g_tNoiseSampler, i.vNoiseTexCoord.wz ).g; // Use green so we can DXT1 if we want
	float flNoise = ( flNoise0 + flNoise1 ) * 0.5f;

	// Generate 0-1 mask from distance computed in the VS
	float flClampedInputMask = 0.0f;
	flClampedInputMask = 1.0f - saturate( i.flDistanceToEffectCenter_flFresnelEffect.x );
	flClampedInputMask *= i.flDistanceToEffectCenter_flFresnelEffect.y;
	flClampedInputMask *= flFleshMaskFromTexture;

	// Noise mask - Only apply noise around border of sphere
	float flBorderMask = saturate( ( 1.0f - flClampedInputMask ) * g_flBorderWidth.x - g_flBorderWidth.y );
	float flNoiseMask = 1.0f - abs( ( flBorderMask * 2.0f ) - 1.0f );

	// This is used to lerp in the 1D border texture over the flesh color
	float flBorderMaskWithNoise = ( 1.0f - smoothstep( flNoiseMask - g_flBorderSoftness, flNoiseMask + g_flBorderSoftness, flNoise.r ) ) * flNoiseMask;

	// Border color
	float vBorderUv = ( sign( flBorderMask - 0.5 ) * (1.0f - pow( flBorderMaskWithNoise, 4.0f )) * 0.5f ) + 0.5f;
	float4 cBorderColor = 2.0f * tex2D( g_tBorder1DSampler, vBorderUv );
	cBorderColor.rgb *= g_cBorderTint;
	cBorderColor.rgb *= flNoise;

	// Normal map
	float4 vNormalMapValue = tex2D( g_tNormalSampler, i.vTexCoord0.xy );
	float3 vTangentNormal = ( vNormalMapValue.xyz * 2.0f ) - 1.0f;
	vTangentNormal.xy += ( flNoise * 1.5f ) - 0.75f; // NOTE: This will denormalize the normal.
	//float3 vWorldNormal = mul( i.mTangentSpaceTranspose, vTangentNormal.xyz );

	// Specular gloss layer
	float3 vTangentReflectionVector = reflect( i.vTangentViewVector.xyz, vTangentNormal.xyz );
	//vTangentReflectionVector.xy += ( flNoise * 1.5f ) - 0.75f;
	float3 vWorldReflectionVector = mul( i.mTangentSpaceTranspose, vTangentReflectionVector.xyz );
	float3 cGlossLayer = ENV_MAP_SCALE * texCUBE( g_tCubeSampler, vWorldReflectionVector.xyz ).rgb;
	cGlossLayer.rgb *= g_flGlossBrightness;

	// Gloss mask is just hard-coded fresnel for now
	float flGlossMask = pow( saturate( dot( vTangentNormal.xyz, -i.vTangentViewVector.xyz ) ), 8.0f );

	// Opacity
	float flOpacity = 1.0f;
	flOpacity = max( flBorderMaskWithNoise, step( flBorderMask, 0.5f ) );

	// Apply global opacity
	flOpacity *= g_flGlobalOpacity;

	//===============//
	// Combine terms //
	//===============//
	float4 result;
	result.rgb = cBaseColor.rgb * i.cVertexLight.rgb;
	result.rgb += cGlossLayer.rgb * flGlossMask;
	result.rgb *= pow( 1.0f - flBorderMaskWithNoise, 2.0f ); // Darken near border
	result.rgb = lerp( result.rgb, cBorderColor.rgb, saturate( vBorderUv * 2.0f ) ); // bring in transition 1D texture

	//result.rgb = flClampedInputMask;
	//result.rgb = flBorderMask;
	//result.rgb = saturate( flClampedInputMask * 2.0f );
	//result.rgb = i.flDistanceToEffectCenter_flFresnelEffect.x;// * i.flDistanceToEffectCenter_flFresnelEffect.y;
	//result.rgb = i.flDistanceToEffectCenter_flFresnelEffect.y * g_flBorderWidth.x - g_flBorderWidth.y;
	//result.rgb = flNoiseMask;
	//result.rgb = flBorderMaskWithNoise;
	//result.rgb = flOpacity;
	//result.rgb = flBorderUv;
	//result.rgb = cBorderColor;
	//result.rgb = -i.vTangentViewVector.z;
	//result.rgb = vNormalMapValue.xyz;
	//result.rgb = vTangentNormal.xyz;
	//result.rgb = flGlossLayer;
	//result.rgb = i.cVertexLight.rgb;
	//result.rgb = texCUBE( g_tCubeSampler, vTangentNormal.xyz ).rgb;
	//result.rgb = i.vTangentViewVector.x;
	//result.rgb = cGlossLayer.rgb;

	// Set alpha for blending
	result.a = flOpacity;
	//result.a = 1.0f;
	
	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR );
}
