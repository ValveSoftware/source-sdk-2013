//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps30][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]
// STATIC: "BUMPMAP" "0..1"

// Includes =======================================================================================
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tRefractionSampler		: register( s0 );
#if BUMPMAP
	sampler g_tBumpSampler			: register( s1 );
#endif

// Shaders Constants and Globals ==================================================================
const float4 g_mViewProj0			: register( c0 );	// 1st row of matrix
const float4 g_mViewProj1			: register( c1 );	// 2nd row of matrix

const float4 g_vCameraPosition		: register( c5 );
const float4 g_vPackedConst6		: register( c6 );
#define g_flCloakFactor   g_vPackedConst6.x // Default = 1.0f
#define g_flRefractAmount g_vPackedConst6.y // Default = 1.0f

const float4 g_cCloakColorTint		: register( c7 );

// 8 2D Poisson offsets (designed to use .xy and .wz swizzles (not .zw)
static const float4 g_vPoissonOffset[4] = {	float4 (-0.0876f,  0.9703f,  0.5651f,  0.4802f ),
											float4 ( 0.1851f,  0.1580f, -0.0617f, -0.2616f ),
											float4 (-0.5477f, -0.6603f,  0.0711f, -0.5325f ),
											float4 (-0.0751f, -0.8954f,  0.4054f,  0.6384f ) };

// Interpolated values ============================================================================
struct PS_INPUT
{
	float3 vWorldNormal			: TEXCOORD0; // World-space normal
	float3 vProjPosForRefract	: TEXCOORD1;
	float3 vWorldViewVector		: TEXCOORD2;
	#if BUMPMAP
		float3x3 mTangentSpaceTranspose : TEXCOORD3;
		//	     second row				: TEXCOORD4;
		//	     third row				: TEXCOORD5;
		float2 vTexCoord0				: TEXCOORD6;
	#endif
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	float3 vWorldNormal = normalize( i.vWorldNormal.xyz );

	#if BUMPMAP
		float4 vBumpTexel = tex2D( g_tBumpSampler, i.vTexCoord0.xy );
		float3 vTangentNormal = ( 2.0f * vBumpTexel ) - 1.0f;
		vWorldNormal.xyz = mul( i.mTangentSpaceTranspose, vTangentNormal.xyz );
	#endif

	// Transform world space normal into clip space and project
	float3 vProjNormal;
	vProjNormal.x = dot( vWorldNormal.xyz, g_mViewProj0.xyz );	// 1st row
	vProjNormal.y = dot( vWorldNormal.xyz, g_mViewProj1.xyz );	// 2nd row

	// Compute coordinates for sampling refraction
	float2 vRefractTexCoordNoWarp = i.vProjPosForRefract.xy / i.vProjPosForRefract.z;
	float2 vRefractTexCoord = vProjNormal.xy;
	float scale = lerp( g_flRefractAmount, 0.0f, saturate( g_flCloakFactor ) );
	vRefractTexCoord.xy *= scale;
	vRefractTexCoord.xy += vRefractTexCoordNoWarp.xy;

	// Blur by scalable Poisson filter
	float flBlurAmount = lerp( 0.05f, 0.0f, saturate( g_flCloakFactor ) );
	float3 cRefract = tex2D( g_tRefractionSampler, vRefractTexCoord.xy );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[0].xy * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[0].wz * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[1].xy * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[1].wz * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[2].xy * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[2].wz * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[3].xy * flBlurAmount ) );
	cRefract += tex2D( g_tRefractionSampler, vRefractTexCoord.xy + ( g_vPoissonOffset[3].wz * flBlurAmount ) );
	cRefract /= 9.0f;

	// 1-(N.V) for Fresnel term (NOTE: If this math changes, you need to update the C code that mimics this on the CPU)
	float flFresnel = 1.0f - saturate( dot( i.vWorldNormal.xyz, normalize( -i.vWorldViewVector.xyz ) ) );
	float flCloakLerpFactor = saturate( lerp( 1.0f, flFresnel - 1.35f, saturate( g_flCloakFactor ) ) );
	flCloakLerpFactor = 1.0f - smoothstep( 0.4f, 0.425f, flCloakLerpFactor );

	// Slightly dim the facing pixels and brighten the silhouette pixels
	cRefract.rgb *= lerp( flFresnel * 0.4 + 0.8, 1.0f, saturate( g_flCloakFactor ) * saturate( g_flCloakFactor ) ); // This gives a scalar in the range [0.8 1.2]

	// Refract color tint
	float fColorTintStrength = saturate( ( saturate( g_flCloakFactor ) - 0.75f ) * 4.0f );
	cRefract.rgb *= lerp( g_cCloakColorTint, 1.0f, fColorTintStrength );

	//===============//
	// Combine terms //
	//===============//
	float4 result;
	result.rgb = cRefract.rgb;

	// Set alpha to cloak mask
	result.a = flCloakLerpFactor;

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
