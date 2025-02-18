//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// Includes =======================================================================================
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tRefractionSampler		: register( s0 );
sampler g_tBumpSampler				: register( s1 );

// Shaders Constants and Globals ==================================================================
const float4 g_mViewProj0			: register( c0 );	// 1st row of matrix
const float4 g_mViewProj1			: register( c1 );	// 2nd row of matrix

const float4 g_vCameraPosition		: register( c5 );
const float4 g_vPackedConst6		: register( c6 );
#define g_flBlurAmount    g_vPackedConst6.x // 0.01f;
#define g_flRefractAmount g_vPackedConst6.y // Default = 1.0f
#define g_flTime          g_vPackedConst6.w

const float4 g_cColorTint			: register( c7 );

const float4 g_vPackedConst8		: register( c8 );
#define g_cSilhouetteColor      g_vPackedConst8 //= { 0.3, 0.3, 0.5 };
#define g_flSilhouetteThickness g_vPackedConst8.w //= 0.2f;

const float2 g_vGroundMinMax	    : register( c9 ); //= { -0.3, -0.1 };

// 8 2D Poisson offsets (designed to use .xy and .wz swizzles (not .zw)
static const float4 g_vPoissonOffset[4] = {	float4 (-0.0876f,  0.9703f,  0.5651f,  0.4802f ),
											float4 ( 0.1851f,  0.1580f, -0.0617f, -0.2616f ),
											float4 (-0.5477f, -0.6603f,  0.0711f, -0.5325f ),
											float4 (-0.0751f, -0.8954f,  0.4054f,  0.6384f ) };

// Interpolated values ============================================================================
struct PS_INPUT
{
	float3 vWorldNormal			: TEXCOORD0; // World-space normal
	float3 vWorldTangent		: TEXCOORD1;
	float3 vWorldBinormal		: TEXCOORD2;
	float3 vProjPosForRefract	: TEXCOORD3;
	float3 vWorldViewVector		: TEXCOORD4;
	float4 vUv0					: TEXCOORD5; // uv.xy, uvScroll.xy
	float4 vUv1					: TEXCOORD6; // uv.xy, uvScroll.xy
	float2 vUvGroundNoise		: TEXCOORD7;
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	/*
	// Bump layer 0
	float2 vUv0 = i.vUv0Uv1.xy;
	float2 vUv0Scroll = i.vUv0Uv1.xy * 3.0f;
	vUv0Scroll.y -= g_flTime * 0.1f;

	float4 vBumpTexel0 = tex2D( g_tBumpSampler, vUv0Scroll.xy );
	vBumpTexel0 = tex2D( g_tBumpSampler, vUv0.xy + (vBumpTexel0.xy*0.03) );

	// Bump layer 1
	float2 vUv1 = i.vUv0Uv1.xy * 10.0f;
	float2 vUv1Scroll = i.vUv0Uv1.xy * 32.0f;
	vUv1Scroll.y -= g_flTime * 0.1f;

	float4 vBumpTexel1 = tex2D( g_tBumpSampler, vUv1Scroll.xy );
	vBumpTexel1 = tex2D( g_tBumpSampler, vUv1.xy + (vBumpTexel1.xy*0.03) );
	//*/

	// Bump layer 0
	float4 vBumpTexel0 = tex2D( g_tBumpSampler, i.vUv0.wz );
	vBumpTexel0 = tex2D( g_tBumpSampler, i.vUv0.xy + ( vBumpTexel0.xy*0.03 ) );

	// Bump layer 1
	float4 vBumpTexel1 = tex2D( g_tBumpSampler, i.vUv1.wz );
	vBumpTexel1 = tex2D( g_tBumpSampler, i.vUv1.xy + ( vBumpTexel1.xy*0.03 ) );

	// Combine bump layers into tangetn normal
	float3 vTangentNormal = ( vBumpTexel0 * 2.0f ) - 1.0f;
	vTangentNormal.xyz += ( vBumpTexel1 * 2.0f - 1.0f ) * 0.5f; // * 0.5f;

	// Transform into world space
	float3 vWorldNormal = Vec3TangentToWorld( vTangentNormal.xyz, i.vWorldNormal, i.vWorldTangent, i.vWorldBinormal );

	// Effect mask
	//float flEffectMask = saturate( dot( -i.vWorldViewVector.xyz, i.vWorldNormal.xyz ) * lerp( 2.0f, 1.0f, g_flSilhouetteThickness ) );
	float flEffectMask = saturate( dot( -i.vWorldViewVector.xyz, i.vWorldNormal.xyz ) * ( (2.0f - g_flSilhouetteThickness) ) );

	// Simulate ground intersection
	flEffectMask *= smoothstep( g_vGroundMinMax.x, g_vGroundMinMax.y, i.vWorldNormal.z );

	// Soften mask by squaring term
	flEffectMask *= flEffectMask;

	// Silhouette mask
	float flSilhouetteHighlightMask = saturate( flEffectMask * ( 1.0f - flEffectMask ) * 4.0f );
	flSilhouetteHighlightMask *= flSilhouetteHighlightMask * flSilhouetteHighlightMask;

	// Transform world space normal into clip space and project
	float3 vProjNormal;
	vProjNormal.x = dot( vWorldNormal.xyz, g_mViewProj0.xyz );	// 1st row
	vProjNormal.y = dot( vWorldNormal.xyz, g_mViewProj1.xyz );	// 2nd row

	// Compute coordinates for sampling refraction
	float2 vRefractTexCoordNoWarp = i.vProjPosForRefract.xy / i.vProjPosForRefract.z;
	float2 vRefractTexCoord = vProjNormal.xy;
	float scale = lerp( 0.0f, g_flRefractAmount, flEffectMask );// * flEffectMask * flEffectMask ); // Using flEffectMask^3
	vRefractTexCoord.xy *= scale;
	vRefractTexCoord.xy += vRefractTexCoordNoWarp.xy;

	// Blur by scalable Poisson filter
	float flBlurAmount = g_flBlurAmount * flEffectMask;
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

	// Undo tone mapping
	cRefract /= LINEAR_LIGHT_SCALE;

	// Refract color tint
	float fColorTintStrength = 1.0f - flEffectMask;
	float3 cRefractColorTint = lerp( g_cColorTint, 1.0f, fColorTintStrength );

	// Ground noise
	//float flGroundNoise = tex2D( g_tBumpSampler, i.vUvGroundNoise.xy ).g;
	//flGroundNoise *= smoothstep( g_vGroundMinMax.y, g_vGroundMinMax.y+0.4, -i.vWorldNormal.z );
	//flGroundNoise = smoothstep( 0.2, 0.9, flGroundNoise );

	//===============//
	// Combine terms //
	//===============//
	float4 result;
	result.rgb = cRefract.rgb * cRefractColorTint.rgb;
	result.rgb += result.rgb * ( flSilhouetteHighlightMask * g_cSilhouetteColor.rgb );
	//result.rgb += flGroundNoise;

//result.rgb = float3( 0.0, 0.0, 0.0 );
//result.rg = vRefractTexCoord.xy;
//result.rg = i.vUv0Uv1.xy;
//result.rgb = vBumpTexel0.rgb;
//result.rgb = vTangentNormal;
//result.rgb = vWorldNormal;
//result = flEffectMask;
//result = flSilhouetteHighlightMask;

//result = tex2D( g_tBumpSampler, i.vUvGroundNoise.xy ).y;
//result.rgb = flGroundNoise;

	// Set alpha to...
	result.a = flEffectMask;


	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR ); //go back to final output when it'll fit.
}
