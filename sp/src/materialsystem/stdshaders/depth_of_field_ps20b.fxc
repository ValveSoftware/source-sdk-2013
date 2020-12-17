//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

// DYNAMIC: "QUALITY"	"0..3"

// Includes =======================================================================================
#include "common_ps_fxc.h"

// Texture Samplers ===============================================================================
sampler g_tFullFB	: register( s0 );
sampler g_tSmallFB	: register( s1 );

// Shaders Constants and Globals ==================================================================
float4 g_vDists			: register( c0 );
#define g_flNearBlurDist		g_vDists.x
#define g_flNearFocusDist		g_vDists.y
#define g_flFarFocusDist		g_vDists.z
#define g_flFarBlurDist			g_vDists.w

float3 g_vBlurAmounts	: register( c1 );
#define g_flMaxBlurRadius		g_vBlurAmounts.x
#define g_flNearBlurStrength	g_vBlurAmounts.y
#define g_flFarBlurStrength		g_vBlurAmounts.z

float3 g_vNearFarDists	: register( c2 );
#define g_flNearPlaneDist		g_vNearFarDists.x
#define g_flFarPlaneDist		g_vNearFarDists.y
#define g_flDepthConv			g_vNearFarDists.z

float4 g_vMagicConsts	: register( c3 );

#if ( QUALITY == 0 )
	#define NUM_SAMPLES 8	// These must match the C code
#elif ( QUALITY == 1 )
	#define NUM_SAMPLES 16
#elif ( QUALITY == 2 )
	#define NUM_SAMPLES 16
#elif ( QUALITY == 3 )
	#define NUM_SAMPLES 32
#endif

float4 g_vPoisson[ NUM_SAMPLES/2 ] : register( c4 );

// Interpolated values ============================================================================
struct PS_INPUT
{
	float2 vUv0 : TEXCOORD0;
};

float DestAlphaDepthToViewSpaceDepth( float flDepth )
{
	return g_flDepthConv * flDepth + g_flNearPlaneDist;
}

// returns blur radius from depth as a fraction of max_blur.
float BlurAmountFromDepth( float flDestAlphaDepth )
{
	/*
	dist = DestAlphaDepthToViewSpaceDepth( flDestAlphaDepth );
	float flBlur = max( g_flNearBlurStrength * saturate( (flDestAlphaDepth - g_flNearFocusDist) / ( g_flNearBlurDist - g_flNearFocusDist ) ),
						g_flFarBlurStrength * saturate( (flDestAlphaDepth - g_flFarFocusDist) / ( g_flFarBlurDist - g_flFarFocusDist ) ) );
	*/

	// A more optimized version that concatenates the math above and the one in DestAlphaDepthToViewSpaceDepth to a single muladd
	float flBlur = max( g_flNearBlurStrength * saturate( g_vMagicConsts.x * flDestAlphaDepth + g_vMagicConsts.y ),
						g_flFarBlurStrength * saturate( g_vMagicConsts.z * flDestAlphaDepth + g_vMagicConsts.w ) );
	return flBlur;
}

float BlurRadiusFromDepth( float flDepth )
{
	return g_flMaxBlurRadius * BlurAmountFromDepth( flDepth );
}

float4 ComputeTap( float flCenterDepth, float flCenterBlurRadius, float2 vUV, float2 vPoisson )
{
	float4 cTapSmall;
	float4 cTap;
	float2 vPoissonUV = vUV.xy + flCenterBlurRadius * vPoisson.xy;

	cTapSmall = tex2D( g_tSmallFB, vPoissonUV.xy );
	cTap = tex2D( g_tFullFB, vPoissonUV.xy );

	float flTapBlur = BlurAmountFromDepth( cTap.a );		// Maybe 50/50 mix between low and high here?

	cTap = lerp( cTap, cTapSmall, saturate( 2.2 * flTapBlur ) );		// TODO: tweak blur amount.
	float flLerpedTapBlur = BlurAmountFromDepth( cTap.a );

	float weight = ( cTap.a >= flCenterDepth ) ? 1.0 : ( flLerpedTapBlur*flLerpedTapBlur );
	return float4( cTap.rgb, 1 ) * weight;
}

float4 ComputeTapHQ( float flCenterDepth, float flCenterBlurRadius, float2 vUV, float2 vPoisson )
{
	float4 cTap;

	cTap = tex2D( g_tFullFB, vUV.xy + flCenterBlurRadius * vPoisson.xy );
	float flTapBlur = BlurAmountFromDepth( cTap.a );
	float weight = ( cTap.a >= flCenterDepth ) ? 1.0 : ( flTapBlur * flTapBlur );
	return float4( cTap.rgb, 1 ) * weight;
}

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	// TODO: BETTER DOWNSAMPLE THAT TAKES DEPTH INTO ACCOUNT?

	float4 cOut = { 0, 0, 0, 0 };
	float4 cCenterTap = tex2D( g_tFullFB, i.vUv0 );
	float flCenterBlurRadius = BlurRadiusFromDepth( cCenterTap.a );	// circle of confusion radius for current pixel
	cCenterTap.a -= 0.001;		// z-bias to avoid strange banding artifact on almost orthogonal walls

#if ( QUALITY < 2 )
	// ATI's Ruby1-style algorithm
	for ( int t = 0; t < NUM_SAMPLES/2; t++ )
	{
		cOut.rgba += ComputeTap( cCenterTap.a, flCenterBlurRadius, i.vUv0, g_vPoisson[t].xy );
		cOut.rgba += ComputeTap( cCenterTap.a, flCenterBlurRadius, i.vUv0, g_vPoisson[t].wz );
	}
#else
	// Less fancy, with less fetches per tap and less math. Needs more samples to look smooth.
	cOut = cCenterTap;
	cOut.a = 1.0;		// Use the center sample we just fetched
	for ( int t = 0; t < NUM_SAMPLES/2; t++ )
	{
		cOut.rgba += ComputeTapHQ( cCenterTap.a, flCenterBlurRadius, i.vUv0, g_vPoisson[t].xy );
		cOut.rgba += ComputeTapHQ( cCenterTap.a, flCenterBlurRadius, i.vUv0, g_vPoisson[t].wz );
	}
#endif
	//cOut.rgb = cOut.a / float(NUM_SAMPLES+1);
	//cOut = lerp( tex2D( g_tFullFB, i.vUv0 ), tex2D( g_tSmallFB, i.vUv0 ).aaaa, 0.5 );
	if ( cOut.a > 0.0 )
		cOut.rgba /= cOut.a;
	else
		cOut.rgba = cCenterTap.rgba;

	return cOut;
}
