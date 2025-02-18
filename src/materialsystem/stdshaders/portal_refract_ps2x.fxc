//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "STAGE"				"0..2"
// STATIC: "SHADER_SRGB_READ"	"0..1"	[ps20b]

// DYNAMIC: "PIXELFOGTYPE"		"0..1"	[ps20b]

#if defined(SHADER_MODEL_PS_2_0)
	#define PIXELFOGTYPE PIXEL_FOG_TYPE_NONE
#endif

// Includes =======================================================================================
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tRefractionSampler		: register( s0 );
sampler g_tPortalNoiseSampler		: register( s1 );
sampler g_tPortalColorSampler		: register( s2 );

// Shaders Constants and Globals ==================================================================
const float4 g_mViewProj0			: register( c0 );	// 1st row of matrix
const float4 g_mViewProj1			: register( c1 );	// 2nd row of matrix
const float4 g_mViewProj2			: register( c2 );	// 3rd row of matrix
const float4 g_mViewProj3			: register( c3 );	// 4th row of matrix
const float3 g_vConst4				: register( c4 );
#define g_flPortalOpenAmount   g_vConst4.x
#define g_flPortalActive       g_vConst4.y
#define g_flPortalColorScale   g_vConst4.z
const float4 g_vCameraPosition		: register( c5 );
const float4 g_vFogParams			: register( c6 );

// Interpolated values ============================================================================
struct PS_INPUT
{
	float2 vUv0					: TEXCOORD0;
	float3 vWorldTangent		: TEXCOORD1;
	float3 vWorldBinormal		: TEXCOORD2;
	float4 vWorldPosition		: TEXCOORD3; // Proj pos z in w
	float3 vProjPosForRefract	: TEXCOORD4;
	float4 vNoiseTexCoord		: TEXCOORD5;
};

// This is the equilavent of smoothstep built into HLSL but linear
float linearstep( float iMin, float iMax, float iValue )
{
	return saturate( ( iValue - iMin ) / ( iMax - iMin ) );
}

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	float kFlPortalOuterBorder = 0.075f; // Must match VS!
	float kFlPortalInnerBorder = kFlPortalOuterBorder * 4.0f;

	// Add a slight border around the portal opening (Do this in the VS?)
	//i.vUv0.xy = i.vUv0.xy * ( 1.0f + kFlPortalOuterBorder ) - ( kFlPortalOuterBorder * 0.5f );

	// Portal open time
	float flPortalOpenAmount = smoothstep( 0.0f, 1.0f, saturate( g_flPortalOpenAmount ) );
	//float flPortalOpenAmount = saturate( g_flPortalOpenAmount );
	float flPortalOpenAmountSquared = flPortalOpenAmount * flPortalOpenAmount;

	// Stretch values
	float2 vStretchVector = ( i.vUv0.xy * 2.0f ) - 1.0f;
	float flDistFromCenter = length( vStretchVector );
	float2 vStretchVectorNormalized = normalize( vStretchVector );

	// Stencil cutout (1.0 in hole)
	float flStencilCutout = step( flDistFromCenter, flPortalOpenAmountSquared );

	//==================================//
	// Stage 0: Warp pixels around hole //
	//==================================//
	#if ( STAGE == 0 )
	{
		// Unrefracted tex coords
		float2 vRefractTexCoordNoWarp = i.vProjPosForRefract.xy / i.vProjPosForRefract.z;

		// Projected screen-space tangent
		float3 vProjTangent;
		vProjTangent.x = dot( float4( i.vWorldTangent.xyz, 1.0f ), g_mViewProj0.xyzw ); // 1st row
		vProjTangent.y = -dot( float4( i.vWorldTangent.xyz, 1.0f ), g_mViewProj1.xyzw ); // 2nd row
		vProjTangent.z = dot( float4( i.vWorldTangent.xyz, 1.0f ), g_mViewProj3.xyzw ); // 4th row
		vProjTangent.xy += vProjTangent.z;
		vProjTangent.xy *= 0.5f;
		vProjTangent.xy /= vProjTangent.z;
		vProjTangent.xy -= vRefractTexCoordNoWarp.xy;

		// Projected screen-space binormal
		float3 vProjBinormal;
		vProjBinormal.x = dot( float4( i.vWorldBinormal.xyz, 1.0f ), g_mViewProj0.xyzw ); // 1st row
		vProjBinormal.y = -dot( float4( i.vWorldBinormal.xyz, 1.0f ), g_mViewProj1.xyzw ); // 2nd row
		vProjBinormal.z = dot( float4( i.vWorldBinormal.xyz, 1.0f ), g_mViewProj3.xyzw ); // 4th row
		vProjBinormal.xy += vProjBinormal.z;
		vProjBinormal.xy *= 0.5f;
		vProjBinormal.xy /= vProjBinormal.z;
		vProjBinormal.xy -= vRefractTexCoordNoWarp.xy;

		// Tangent-space uv offset
		float2 vTangentRefract = -vStretchVectorNormalized * flPortalOpenAmountSquared * ( 1.0f - pow( saturate( flDistFromCenter ), 64.0f ) );
		vTangentRefract.xy *= smoothstep( ( flPortalOpenAmount * 1.5f ), flPortalOpenAmount, flDistFromCenter );

		// Note: This works well perpendicular to the surface, but because the projection is non-linear, it's refracty very edge on
		float2 kPortalRadius = { 32.0f, 32.0f }; // Should be 32, 54 but this reduces the artifacts from the comment above
		vTangentRefract.xy *= kPortalRadius.xy;

		// Generate refracteds screen-space uv
		float2 vRefractTexCoord = vRefractTexCoordNoWarp.xy;
		vRefractTexCoord.xy += vTangentRefract.x * vProjTangent.xy;
		vRefractTexCoord.xy -= vTangentRefract.y * vProjBinormal.xy;

		// Fetch color from texture
		float3 cRefract = tex2D( g_tRefractionSampler, vRefractTexCoord.xy );

		// In some cases, we have to convert this render target from sRGB to Linear ourselves here
		#if ( SHADER_SRGB_READ == 1 )
		{
			cRefract = GammaToLinear( cRefract );
		}
		#endif

		// Darken the ring around the portal as it's opening to help it stand out on plain walls
		float flHoleEdge = flPortalOpenAmountSquared;
		float flDimEdge = saturate( flPortalOpenAmount * 2.0f );
		float flDarkeningRing = linearstep( flHoleEdge - 0.01f, flDimEdge, flDistFromCenter );
		flDarkeningRing = ( abs( flDarkeningRing * 2.0f - 1.0f ) * 0.15f ) + 0.85f;

		//===============//
		// Combine terms //
		//===============//
		float4 result;
		result.rgb = cRefract.rgb;
		result.rgb *= flDarkeningRing;

		// Alpha test away outside the portal oval
		result.a = step( flDistFromCenter, 1.0f );
		
		return FinalOutput( result, 0.0f, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
	#endif

	//============================================================================//
	// Stage 1: Cut a hole in the stencil buffer (only render pixels in the hole) //
	//============================================================================//
	#if ( STAGE == 1 )
	{
		float4 result;
		result.rgb = 0.0f;
		result.a = flStencilCutout;
		//result = 0.0f; // Disable the hole for debugging
		return result;
	}
	#endif

	//============================================//
	// Stage 2: Fire effect around rim of opening //
	//============================================//
	#if ( STAGE == 2 )
	{
		// Outer effect mask
		float flOuterEffectMask = ( 1.0f - linearstep( flPortalOpenAmountSquared, flPortalOpenAmountSquared + kFlPortalOuterBorder, flDistFromCenter ) ) * ( 1.0f - flStencilCutout );

		// Inner effect mask
		float flInnerEffectMask = ( linearstep( flPortalOpenAmountSquared - kFlPortalInnerBorder, flPortalOpenAmountSquared, flDistFromCenter ) ) * ( flStencilCutout );

		// Fade it in as the portal is opening
		//float flPortalActive = smoothstep( 0.0f, 1.0f, saturate( g_flPortalActive ) );
		float flPortalActive = saturate( g_flPortalActive ); // This is good enough...smoothstep above is not necessary
		//flPortalActive = linearstep( 0.0f, saturate( flDistFromCenter ), saturate( flPortalActive ) ); // Experiment to fade from center out
		float flEffectFadeIn = max( saturate( flPortalOpenAmount * 2.5f ), ( 1.0f - flPortalActive ) );

		// Combine mask terms
		float flEffectMask = ( flInnerEffectMask + flOuterEffectMask ) * flEffectFadeIn;
		//flEffectMask = pow( flEffectMask, 0.75f ); // This will thicken the border but also darken the alpha blend in ugly ways. Leaving this here for experiments later.

		float4 cNoiseTexel1 = tex2D( g_tPortalNoiseSampler, i.vNoiseTexCoord.xy );
		float4 cNoiseTexel2 = tex2D( g_tPortalNoiseSampler, i.vNoiseTexCoord.wz - cNoiseTexel1.rg*0.02 );
		cNoiseTexel1.rgba = tex2D( g_tPortalNoiseSampler, i.vNoiseTexCoord.xy - cNoiseTexel2.rg*0.02 );

		//float flNoise = ( ( cNoiseTexel1.g * cNoiseTexel2.g ) * 2.0f ); // More broken up flames and crazier
		float flNoise = ( ( cNoiseTexel1.g + cNoiseTexel2.g ) * 0.5f ); // More solid flames and calmer
		float flPortalActiveWithNoise = smoothstep( 0.0f, flNoise, flPortalActive );

		float kFlBorderSoftness = 0.875f; // Larger numbers give more color in the middle when portal is inactive
		float flBorderMaskWithNoise = ( 1.0f - smoothstep( flEffectMask - kFlBorderSoftness, flEffectMask + kFlBorderSoftness, flNoise ) );
		flNoise = flBorderMaskWithNoise;
		flEffectMask *= flBorderMaskWithNoise;

		// This will get stuffed in alpha
		float flTransparancy = saturate( flEffectMask + ( flStencilCutout * ( 1.0f - flPortalActiveWithNoise ) ) ) * 1.5f; // Magic number at the end will make the flames thicker with larger numbers

		// This will make the portals shift in color from bottom to top (Set to 1.0f to disable)
		//float flBottomToTopBrightnessShift = ( pow( abs(i.vUv0.y), 1.5f ) * 0.9f ) + 0.1f; // More extreme
		//float flBottomToTopBrightnessShift = ( pow( abs(i.vUv0.y), 1.5f ) * 0.85f ) + 0.15f;
		float flBottomToTopBrightnessShift = ( pow( abs(i.vUv0.y), 1.5f ) * 0.8f ) + 0.2f;
		//float flBottomToTopBrightnessShift = ( pow( abs(i.vUv0.y), 1.5f ) * 0.75f ) + 0.25f; // More subtle (needs higher color scale below)
		//float flBottomToTopBrightnessShift = 1.0f; // Disabled

		// Fetch color from 1D texture
		float4 cFlameColor = tex1D( g_tPortalColorSampler, pow( flNoise, 0.5f ) * flBottomToTopBrightnessShift * flTransparancy );
		cFlameColor.rgb *= g_flPortalColorScale; // Brighten colors to make it look more emissive

		// Generate final color result
		float4 result;
		result.rgb = cFlameColor.rgb;
		result.a = flTransparancy;
		//result.rgb *= result.a; // This will give better definition to the flames but also darkens the outer rim
		//result.rgb = pow( result.rgb, 1.5f );
		//result.rgb *= result.rgb; // Make it look hotter

		// Debugging
		//result.rgba = flBorderMaskWithNoise;
		//result.rgba = flEffectMask;
		//result.rgba = flTransparancy;
		//result.rgba = flPortalActive * flStencilCutout;

		// Apply fog and deal with HDR
		float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_vFogParams, g_vCameraPosition.z, i.vWorldPosition.z, i.vWorldPosition.w );
		return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
	}
	#endif
}
