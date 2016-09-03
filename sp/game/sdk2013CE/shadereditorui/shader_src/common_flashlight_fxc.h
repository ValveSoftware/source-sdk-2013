//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======//
//
// Purpose: Common pixel shader code specific to flashlights
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_FLASHLIGHT_FXC_H_
#define COMMON_FLASHLIGHT_FXC_H_

#include "common_ps_fxc.h"


// JasonM - TODO: remove this simpleton version
float DoShadow( sampler DepthSampler, float4 texCoord )
{
	const float g_flShadowBias = 0.0005f;
	float2 uoffset = float2( 0.5f/512.f, 0.0f );
	float2 voffset = float2( 0.0f, 0.5f/512.f );
	float3 projTexCoord = texCoord.xyz / texCoord.w;
	float4 flashlightDepth = float4(	tex2D( DepthSampler, projTexCoord + uoffset + voffset ).x,
										tex2D( DepthSampler, projTexCoord + uoffset - voffset ).x,
										tex2D( DepthSampler, projTexCoord - uoffset + voffset ).x,
										tex2D( DepthSampler, projTexCoord - uoffset - voffset ).x	);

#	if ( defined( REVERSE_DEPTH_ON_X360 ) )
	{
		flashlightDepth = 1.0f - flashlightDepth;
	}
#	endif

	float shadowed = 0.0f;
	float z = texCoord.z/texCoord.w;
	float4 dz = float4(z,z,z,z) - (flashlightDepth + float4( g_flShadowBias, g_flShadowBias, g_flShadowBias, g_flShadowBias));
	float4 shadow = float4(0.25f,0.25f,0.25f,0.25f);

	if( dz.x <= 0.0f )
		shadowed += shadow.x;
	if( dz.y <= 0.0f )
		shadowed += shadow.y;
	if( dz.z <= 0.0f )
		shadowed += shadow.z;
	if( dz.w <= 0.0f )
		shadowed += shadow.w;

	return shadowed;
}


float DoShadowNvidiaRAWZOneTap( sampler DepthSampler, const float4 shadowMapPos )
{
	float ooW = 1.0f / shadowMapPos.w;								// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos.xyz * ooW;		// Do both projections at once

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float fDepth = dot(tex2D(DepthSampler, shadowMapCenter).arg, float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5));

	return fDepth > objDepth;
}


float DoShadowNvidiaRAWZ( sampler DepthSampler, const float4 shadowMapPos )
{
	float fE = 1.0f / 512.0f;	 // Epsilon

	float ooW = 1.0f / shadowMapPos.w;								// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos.xyz * ooW;		// Do both projections at once

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vDepths;
	vDepths.x = dot(tex2D(DepthSampler, shadowMapCenter + float2(  fE,  fE )).arg, float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5));
	vDepths.y = dot(tex2D(DepthSampler, shadowMapCenter + float2( -fE,  fE )).arg, float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5));
	vDepths.z = dot(tex2D(DepthSampler, shadowMapCenter + float2(  fE, -fE )).arg, float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5));
	vDepths.w = dot(tex2D(DepthSampler, shadowMapCenter + float2( -fE, -fE )).arg, float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5));

	return dot(vDepths > objDepth.xxxx, float4(0.25, 0.25, 0.25, 0.25));
}


float DoShadowNvidiaCheap( sampler DepthSampler, const float4 shadowMapPos )
{
	float fTexelEpsilon = 1.0f / 1024.0f;

	float ooW = 1.0f / shadowMapPos.w;								// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos.xyz * ooW;		// Do both projections at once

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vTaps;
	vTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTexelEpsilon,  fTexelEpsilon), objDepth, 1 ) ).x;
	vTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTexelEpsilon,  fTexelEpsilon), objDepth, 1 ) ).x;
	vTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTexelEpsilon, -fTexelEpsilon), objDepth, 1 ) ).x;
	vTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTexelEpsilon, -fTexelEpsilon), objDepth, 1 ) ).x;

	return dot(vTaps, float4(0.25, 0.25, 0.25, 0.25));
}

float DoShadowNvidiaPCF3x3Box( sampler DepthSampler, const float4 shadowMapPos )
{
	float fTexelEpsilon = 1.0f / 1024.0f;

	float ooW = 1.0f / shadowMapPos.w;								// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos.xyz * ooW;		// Do both projections at once

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vOneTaps;
	vOneTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTexelEpsilon,  fTexelEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTexelEpsilon,  fTexelEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTexelEpsilon, -fTexelEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTexelEpsilon, -fTexelEpsilon ), objDepth, 1 ) ).x;
	float flOneTaps = dot( vOneTaps, float4(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));

	float4 vTwoTaps;
	vTwoTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTexelEpsilon,  0 ), objDepth, 1 ) ).x;
	vTwoTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTexelEpsilon,  0 ), objDepth, 1 ) ).x;
	vTwoTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fTexelEpsilon ), objDepth, 1 ) ).x;
	vTwoTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fTexelEpsilon ), objDepth, 1 ) ).x;
	float flTwoTaps = dot( vTwoTaps, float4(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));

	float flCenterTap = tex2Dproj( DepthSampler, float4( shadowMapCenter, objDepth, 1 ) ).x * (1.0f / 9.0f);

	// Sum all 9 Taps
	return flOneTaps + flTwoTaps + flCenterTap;
}


//
//	1	4	7	4	1
//	4	20	33	20	4
//	7	33	55	33	7
//	4	20	33	20	4
//	1	4	7	4	1
//
float DoShadowNvidiaPCF5x5Gaussian( sampler DepthSampler, const float4 shadowMapPos )
{
	float fEpsilon    = 1.0f / 512.0f;
	float fTwoEpsilon = 2.0f * fEpsilon;

	float ooW = 1.0f / shadowMapPos.w;								// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos.xyz * ooW;		// Do both projections at once

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vOneTaps;
	vOneTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilon,  fTwoEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilon,  fTwoEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilon, -fTwoEpsilon ), objDepth, 1 ) ).x;
	vOneTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilon, -fTwoEpsilon ), objDepth, 1 ) ).x;
	float flOneTaps = dot( vOneTaps, float4(1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f));

	float4 vSevenTaps;
	vSevenTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilon,  0 ), objDepth, 1 ) ).x;
	vSevenTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilon,  0 ), objDepth, 1 ) ).x;
	vSevenTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fTwoEpsilon ), objDepth, 1 ) ).x;
	vSevenTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fTwoEpsilon ), objDepth, 1 ) ).x;
	float flSevenTaps = dot( vSevenTaps, float4( 7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f ) );

	float4 vFourTapsA, vFourTapsB;
	vFourTapsA.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilon,  fEpsilon    ), objDepth, 1 ) ).x;
	vFourTapsA.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilon,     fTwoEpsilon ), objDepth, 1 ) ).x;
	vFourTapsA.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilon,     fTwoEpsilon ), objDepth, 1 ) ).x;
	vFourTapsA.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilon,  fEpsilon    ), objDepth, 1 ) ).x;
	vFourTapsB.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilon, -fEpsilon    ), objDepth, 1 ) ).x;
	vFourTapsB.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilon,    -fTwoEpsilon ), objDepth, 1 ) ).x;
	vFourTapsB.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilon,    -fTwoEpsilon ), objDepth, 1 ) ).x;
	vFourTapsB.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilon, -fEpsilon    ), objDepth, 1 ) ).x;
	float flFourTapsA = dot( vFourTapsA, float4( 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f ) );
	float flFourTapsB = dot( vFourTapsB, float4( 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f ) );

	float4 v20Taps;
	v20Taps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilon,  fEpsilon ), objDepth, 1 ) ).x;
	v20Taps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilon,  fEpsilon ), objDepth, 1 ) ).x;
	v20Taps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilon, -fEpsilon ), objDepth, 1 ) ).x;
	v20Taps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilon, -fEpsilon ), objDepth, 1 ) ).x;
	float fl20Taps = dot( v20Taps, float4(20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f));

	float4 v33Taps;
	v33Taps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilon,  0 ), objDepth, 1 ) ).x;
	v33Taps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilon,  0 ), objDepth, 1 ) ).x;
	v33Taps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fEpsilon ), objDepth, 1 ) ).x;
	v33Taps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fEpsilon ), objDepth, 1 ) ).x;
	float fl33Taps = dot( v33Taps, float4(33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f));

	float flCenterTap = tex2Dproj( DepthSampler, float4( shadowMapCenter, objDepth, 1 ) ).x * (55.0f / 331.0f);

	// Sum all 25 Taps
	return flOneTaps + flSevenTaps + +flFourTapsA + flFourTapsB + fl20Taps + fl33Taps + flCenterTap;
}


float DoShadowATICheap( sampler DepthSampler, const float4 shadowMapPos )
{
    float2 shadowMapCenter = shadowMapPos.xy/shadowMapPos.w;
	float objDepth = shadowMapPos.z / shadowMapPos.w;
	float fSampleDepth = tex2D( DepthSampler, shadowMapCenter ).x;

	objDepth = min( objDepth, 0.99999 ); //HACKHACK: On 360, surfaces at or past the far flashlight plane have an abrupt cutoff. This is temp until a smooth falloff is implemented

	return fSampleDepth > objDepth;
}


// Poisson disc, randomly rotated at different UVs
float DoShadowPoisson16Sample( sampler DepthSampler, sampler RandomRotationSampler, const float3 vProjCoords, const float2 vScreenPos, const float4 vShadowTweaks, bool bNvidiaHardwarePCF, bool bFetch4 )
{
	float2 vPoissonOffset[8] = { float2(  0.3475f,  0.0042f ),
								 float2(  0.8806f,  0.3430f ),
								 float2( -0.0041f, -0.6197f ),
								 float2(  0.0472f,  0.4964f ),
								 float2( -0.3730f,  0.0874f ),
								 float2( -0.9217f, -0.3177f ),
								 float2( -0.6289f,  0.7388f ),
								 float2(  0.5744f, -0.7741f ) };

	float flScaleOverMapSize = vShadowTweaks.x * 2;		// Tweak parameters to shader
	float2 vNoiseOffset = vShadowTweaks.zw;
	float4 vLightDepths = 0, accum = 0.0f;
	float2 rotOffset = 0;

	float2 shadowMapCenter = vProjCoords.xy;			// Center of shadow filter
	float objDepth = min( vProjCoords.z, 0.99999 );		// Object depth in shadow space

	// 2D Rotation Matrix setup
	float3 RMatTop = 0, RMatBottom = 0;
#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
	RMatTop.xy = tex2D( RandomRotationSampler, cFlashlightScreenScale.xy * (vScreenPos * 0.5 + 0.5) + vNoiseOffset) * 2.0 - 1.0;
	RMatBottom.xy = float2(-1.0, 1.0) * RMatTop.yx;	// 2x2 rotation matrix in 4-tuple
#endif

	RMatTop *= flScaleOverMapSize;				// Scale up kernel while accounting for texture resolution
	RMatBottom *= flScaleOverMapSize;

	RMatTop.z = shadowMapCenter.x;				// To be added in d2adds generated below
	RMatBottom.z = shadowMapCenter.y;
	
	float fResult = 0.0f;

	if ( bNvidiaHardwarePCF )
	{
		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[0].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[0].xy) + RMatBottom.z;
		vLightDepths.x += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[1].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[1].xy) + RMatBottom.z;
		vLightDepths.y += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[2].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[2].xy) + RMatBottom.z;
		vLightDepths.z += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[3].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[3].xy) + RMatBottom.z;
		vLightDepths.w += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4].xy) + RMatBottom.z;
		vLightDepths.x += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[5].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[5].xy) + RMatBottom.z;
		vLightDepths.y += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[6].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[6].xy) + RMatBottom.z;
		vLightDepths.z += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[7].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[7].xy) + RMatBottom.z;
		vLightDepths.w += tex2Dproj( DepthSampler, float4(rotOffset, objDepth, 1) ).x;

		fResult = dot( vLightDepths, float4( 0.25, 0.25, 0.25, 0.25) );
	}
	else if ( bFetch4 )
	{
/*

TODO: Fix this contact hardening stuff

		float flNumCloserSamples = 1;
		float flAccumulatedCloserSamples = objDepth;
		float4 vBlockerDepths;

		// First, search for blockers
		for( int j=0; j<8; j++ )
		{
			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[j].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[j].xy) + RMatBottom.z;
			vBlockerDepths = tex2D( DepthSampler, rotOffset.xy );

			// Which samples are closer than the pixel we're rendering?
			float4 vCloserSamples = (vBlockerDepths < objDepth.xxxx );				// Binary comparison results
			flNumCloserSamples += dot( vCloserSamples, float4(1, 1, 1, 1) );		// How many samples are closer than receiver?
			flAccumulatedCloserSamples += dot (vCloserSamples, vBlockerDepths );	// Total depths from samples closer than receiver
		}

		float flBlockerDepth = flAccumulatedCloserSamples / flNumCloserSamples;
		float flContactHardeningScale = (objDepth - flBlockerDepth) / flBlockerDepth;

		// Scale the kernel
		RMatTop.xy    *= flContactHardeningScale;
		RMatBottom.xy *= flContactHardeningScale;
*/

		for( int i=0; i<8; i++ )
		{
			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[i].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[i].xy) + RMatBottom.z;
			vLightDepths = tex2D( DepthSampler, rotOffset.xy );
			accum += (vLightDepths > objDepth.xxxx);
		}

		fResult = dot( accum, float4( 1.0f/32.0f, 1.0f/32.0f, 1.0f/32.0f, 1.0f/32.0f) );
	}
	else	// ATI vanilla hardware shadow mapping
	{
		for( int i=0; i<2; i++ )
		{
			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4*i+0].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4*i+0].xy) + RMatBottom.z;
			vLightDepths.x = tex2D( DepthSampler, rotOffset.xy ).x;

			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4*i+1].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4*i+1].xy) + RMatBottom.z;
			vLightDepths.y = tex2D( DepthSampler, rotOffset.xy ).x;

			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4*i+2].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4*i+2].xy) + RMatBottom.z;
			vLightDepths.z = tex2D( DepthSampler, rotOffset.xy ).x;

			rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4*i+3].xy) + RMatTop.z;
			rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4*i+3].xy) + RMatBottom.z;
			vLightDepths.w = tex2D( DepthSampler, rotOffset.xy ).x;

			accum += (vLightDepths > objDepth.xxxx);
		}

		fResult = dot( accum, float4( 0.125, 0.125, 0.125, 0.125) );
	}

	return fResult;
}

#if defined( _X360 )

// Poisson disc, randomly rotated at different UVs
float DoShadow360Simple( sampler DepthSampler, const float3 vProjCoords )
{
	float fLOD;
	float2 shadowMapCenter = vProjCoords.xy;			// Center of shadow filter
	float objDepth = min( vProjCoords.z, 0.99999 );		// Object depth in shadow space

#if defined( REVERSE_DEPTH_ON_X360 )
	objDepth = 1.0f - objDepth;
#endif	

	float4 vSampledDepths, vWeights;

	asm {
		getCompTexLOD2D fLOD.x, shadowMapCenter.xy, DepthSampler, AnisoFilter=max16to1
			setTexLOD fLOD.x

			tfetch2D vSampledDepths.x___, shadowMapCenter, DepthSampler, OffsetX = -0.5, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths._x__, shadowMapCenter, DepthSampler, OffsetX =  0.5, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.__x_, shadowMapCenter, DepthSampler, OffsetX = -0.5, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.___x, shadowMapCenter, DepthSampler, OffsetX =  0.5, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point

			getWeights2D vWeights, shadowMapCenter.xy, DepthSampler, MagFilter=linear, MinFilter=linear, UseComputedLOD=false, UseRegisterLOD=true
	};

	vWeights = float4( (1-vWeights.x)*(1-vWeights.y), vWeights.x*(1-vWeights.y), (1-vWeights.x)*vWeights.y, vWeights.x*vWeights.y );

#if defined( REVERSE_DEPTH_ON_X360 )
	float4 vCompare = (vSampledDepths < objDepth.xxxx);
#else
	float4 vCompare = (vSampledDepths > objDepth.xxxx);
#endif

	return dot( vCompare, vWeights );
}


float Do360PCFFetch( sampler DepthSampler, float2 tc, float objDepth )
{
	float fLOD;
	float4 vSampledDepths, vWeights;

	asm {
			getCompTexLOD2D fLOD.x, tc.xy, DepthSampler, AnisoFilter=max16to1
			setTexLOD fLOD.x

			tfetch2D vSampledDepths.x___, tc, DepthSampler, OffsetX = -0.5, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths._x__, tc, DepthSampler, OffsetX =  0.5, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.__x_, tc, DepthSampler, OffsetX = -0.5, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.___x, tc, DepthSampler, OffsetX =  0.5, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point

			getWeights2D vWeights, tc.xy, DepthSampler, MagFilter=linear, MinFilter=linear, UseComputedLOD=false, UseRegisterLOD=true
	};

	vWeights = float4( (1-vWeights.x)*(1-vWeights.y), vWeights.x*(1-vWeights.y), (1-vWeights.x)*vWeights.y, vWeights.x*vWeights.y );

#if defined( REVERSE_DEPTH_ON_X360 )
	float4 vCompare = (vSampledDepths < objDepth.xxxx);
#else
	float4 vCompare = (vSampledDepths > objDepth.xxxx);
#endif

	return dot( vCompare, vWeights );
}



float Do360NearestFetch( sampler DepthSampler, float2 tc, float objDepth )
{
	float fLOD;
	float4 vSampledDepth;

	asm {
		getCompTexLOD2D fLOD.x, tc.xy, DepthSampler, AnisoFilter=max16to1
		setTexLOD fLOD.x

		tfetch2D vSampledDepth.x___, tc, DepthSampler, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
	};

#if defined( REVERSE_DEPTH_ON_X360 )
	return (vSampledDepth.x < objDepth.x);
#else
	return (vSampledDepth.x > objDepth.x);
#endif

}


float AmountShadowed_8Tap_360( sampler DepthSampler, float2 tc, float objDepth )
{
	float fLOD;
	float4 vSampledDepthsA, vSampledDepthsB;

	// Optimal 8 rooks pattern to get an idea about whether we're at a penumbra or not
	// From [Kallio07] "Scanline Edge-Flag Algorithm for Antialiasing" 
	//
	//        +---+---+---+---+---+---+---+---+
	//        |   |   |   |   |   | o |   |   |
	//        +---+---+---+---+---+---+---+---+
	//        | o |   |   |   |   |   |   |   |
	//        +---+---+---+---+---+---+---+---+
	//        |   |   |   | o |   |   |   |   |
	//        +---+---+---+---+---+---+---+---+
	//        |   |   |   |   |   |   | o |   |
	//        +---+---+---+---+---+---+---+---+
	//        |   | o |   |   |   |   |   |   |
	//        +---+---+---+---+---+---+---+---+
	//        |   |   |   |   | o |   |   |   |
	//        +---+---+---+---+---+---+---+---+
	//        |   |   |   |   |   |   |   | o |
	//        +---+---+---+---+---+---+---+---+
	//        |   |   | o |   |   |   |   |   |
	//        +---+---+---+---+---+---+---+---+
	//
	asm {
			getCompTexLOD2D fLOD.x, tc.xy, DepthSampler, AnisoFilter=max16to1
			setTexLOD fLOD.x

			tfetch2D vSampledDepthsA.x___, tc, DepthSampler, OffsetX = -2.0, OffsetY = -1.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsA._x__, tc, DepthSampler, OffsetX = -1.5, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsA.__x_, tc, DepthSampler, OffsetX = -1.0, OffsetY =  2.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsA.___x, tc, DepthSampler, OffsetX = -0.5, OffsetY = -1.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point

			tfetch2D vSampledDepthsB.x___, tc, DepthSampler, OffsetX =  0.5, OffsetY =  1.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsB._x__, tc, DepthSampler, OffsetX =  1.0, OffsetY = -2.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsB.__x_, tc, DepthSampler, OffsetX =  1.5, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepthsB.___x, tc, DepthSampler, OffsetX =  2.0, OffsetY =  1.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
	};

#if defined( REVERSE_DEPTH_ON_X360 )
	float4 vCompareA = (vSampledDepthsA < objDepth.xxxx);
	float4 vCompareB = (vSampledDepthsB < objDepth.xxxx);
#else
	float4 vCompareA = (vSampledDepthsA > objDepth.xxxx);
	float4 vCompareB = (vSampledDepthsB > objDepth.xxxx);
#endif

	return dot( vCompareA, float4(0.125,0.125,0.125,0.125) ) + dot( vCompareB, float4(0.125,0.125,0.125,0.125) );
}


float AmountShadowed_4Tap_360( sampler DepthSampler, float2 tc, float objDepth )
{
	float fLOD;
	float4 vSampledDepths;

	// Rotated grid pattern to get an idea about whether we're at a penumbra or not
	asm {
		getCompTexLOD2D fLOD.x, tc.xy, DepthSampler, AnisoFilter=max16to1
			setTexLOD fLOD.x

			tfetch2D vSampledDepths.x___, tc, DepthSampler, OffsetX = -1.0, OffsetY =  0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths._x__, tc, DepthSampler, OffsetX = -0.5, OffsetY = -1.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.__x_, tc, DepthSampler, OffsetX =  0.5, OffsetY =  1.0, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
			tfetch2D vSampledDepths.___x, tc, DepthSampler, OffsetX =  1.0, OffsetY = -0.5, UseComputedLOD=false, UseRegisterLOD=true, MagFilter = point, MinFilter = point
	};

#if defined( REVERSE_DEPTH_ON_X360 )
	float4 vCompare = (vSampledDepths < objDepth.xxxx);
#else
	float4 vCompare = (vSampledDepths > objDepth.xxxx);
#endif

	return dot( vCompare, float4(0.25,0.25,0.25,0.25) );
}

// Poisson disc, randomly rotated at different UVs
float DoShadowPoisson360( sampler DepthSampler, sampler RandomRotationSampler, const float3 vProjCoords, const float2 vScreenPos, const float4 vShadowTweaks )
{
	float2 vPoissonOffset[8] = { float2(  0.3475f,  0.0042f ), float2(  0.8806f,  0.3430f ),
								 float2( -0.0041f, -0.6197f ), float2(  0.0472f,  0.4964f ),
								 float2( -0.3730f,  0.0874f ), float2( -0.9217f, -0.3177f ),
								 float2( -0.6289f,  0.7388f ), float2(  0.5744f, -0.7741f ) };

	float2 shadowMapCenter = vProjCoords.xy;		// Center of shadow filter
	float objDepth = min( vProjCoords.z, 0.99999 );	// Object depth in shadow space

#if defined( REVERSE_DEPTH_ON_X360 )
	objDepth = 1.0f - objDepth;
#endif

	float fAmountShadowed = AmountShadowed_4Tap_360( DepthSampler, shadowMapCenter, objDepth );

	if ( fAmountShadowed >= 1.0f )			// Fully in light
	{
		return 1.0f;
	}
	else	// Do the expensive filtering since we're at least partially shadowed
	{
		float flScaleOverMapSize = 1.7f / 512.0f;		// Tweak parameters to shader

		// 2D Rotation Matrix setup
		float3 RMatTop = 0, RMatBottom = 0;
#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
		RMatTop.xy = tex2D( RandomRotationSampler, cFlashlightScreenScale.xy * (vScreenPos * 0.5 + 0.5)) * 2.0 - 1.0;
		RMatBottom.xy = float2(-1.0, 1.0) * RMatTop.yx;	// 2x2 rotation matrix in 4-tuple
#endif

		RMatTop *= flScaleOverMapSize;					// Scale up kernel while accounting for texture resolution
		RMatBottom *= flScaleOverMapSize;
		RMatTop.z = shadowMapCenter.x;					// To be added in d2adds generated below
		RMatBottom.z = shadowMapCenter.y;
		float2 rotOffset = float2(0,0);
		float4 vAccum = 0;

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[0].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[0].xy) + RMatBottom.z;
		vAccum.x  = Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[1].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[1].xy) + RMatBottom.z;
		vAccum.y  = Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[2].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[2].xy) + RMatBottom.z;
		vAccum.z  = Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[3].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[3].xy) + RMatBottom.z;
		vAccum.w  = Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[4].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[4].xy) + RMatBottom.z;
		vAccum.x += Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[5].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[5].xy) + RMatBottom.z;
		vAccum.y += Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[6].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[6].xy) + RMatBottom.z;
		vAccum.z += Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		rotOffset.x = dot (RMatTop.xy,    vPoissonOffset[7].xy) + RMatTop.z;
		rotOffset.y = dot (RMatBottom.xy, vPoissonOffset[7].xy) + RMatBottom.z;
		vAccum.w += Do360NearestFetch( DepthSampler, rotOffset, objDepth );

		return dot( vAccum, float4( 0.25, 0.25, 0.25, 0.25) );
	}
}

#endif // _X360


float DoFlashlightShadow( sampler DepthSampler, sampler RandomRotationSampler, float3 vProjCoords, float2 vScreenPos, int nShadowLevel, float4 vShadowTweaks, bool bAllowHighQuality )
{
	float flShadow = 1.0f;

#if !defined( _X360 ) //PC
	if( nShadowLevel == NVIDIA_PCF_POISSON )
		flShadow = DoShadowPoisson16Sample( DepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, vShadowTweaks, true, false );
	else if( nShadowLevel == ATI_NOPCF )
		flShadow = DoShadowPoisson16Sample( DepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, vShadowTweaks, false, false );
	else if( nShadowLevel == ATI_NO_PCF_FETCH4 )
		flShadow = DoShadowPoisson16Sample( DepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, vShadowTweaks, false, true );

	return flShadow;
#else

	// Compile-time switch for shaders which allow high quality modes on 360
	if ( bAllowHighQuality )
	{
		// Static control flow switch for shadow quality.  Some non-interactive sequences use the high quality path
		if ( g_bHighQualityShadows )
		{
			flShadow = DoShadowPoisson360( DepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, vShadowTweaks );
		}
		else
		{
			flShadow = DoShadow360Simple( DepthSampler, vProjCoords );
		}
	}
	else
	{
		flShadow = DoShadow360Simple( DepthSampler, vProjCoords );
	}

	return flShadow;

#endif
}

float3 SpecularLight( const float3 vWorldNormal, const float3 vLightDir, const float fSpecularExponent,
					  const float3 vEyeDir, /*const bool bDoSpecularWarp, in sampler specularWarpSampler,*/ float fFresnel )
{
	float3 result = float3(0.0f, 0.0f, 0.0f);

	float3 vReflect = reflect( -vEyeDir, vWorldNormal );			// Reflect view through normal
	float3 vSpecular = saturate(dot( vReflect, vLightDir ));		// L.R	(use half-angle instead?)
	vSpecular = pow( vSpecular.x, fSpecularExponent );				// Raise to specular power

	// Optionally warp as function of scalar specular and fresnel
	//if ( bDoSpecularWarp )
	//	vSpecular *= tex2D( specularWarpSampler, float2(vSpecular.x, fFresnel) ); // Sample at { (L.R)^k, fresnel }
	vSpecular *= fFresnel;

	return vSpecular;
}

void DoSpecularFlashlight( float3 flashlightPos, float3 worldPos, float4 flashlightSpacePosition, float3 worldNormal,  
					float3 attenuationFactors, float farZ, sampler FlashlightSampler, sampler FlashlightDepthSampler, sampler RandomRotationSampler,
					int nShadowLevel, bool bDoShadows, bool bAllowHighQuality, const float2 vScreenPos, const float fSpecularExponent, const float3 vEyeDir,
					/*const bool bDoSpecularWarp, sampler specularWarpSampler,*/ float fFresnel, float4 vShadowTweaks,

					// Outputs of this shader...separate shadowed diffuse and specular from the flashlight
					out float3 diffuseLighting, out float3 specularLighting )
{
	float3 vProjCoords = flashlightSpacePosition.xyz / flashlightSpacePosition.w;
	float3 flashlightColor = float3(1,1,1);

#if ( defined( _X360 ) )

	float3 ltz = vProjCoords.xyz < float3( 0.0f, 0.0f, 0.0f );
	float3 gto = vProjCoords.xyz > float3( 1.0f, 1.0f, 1.0f );

	[branch]
	if ( dot(ltz + gto, float3(1,1,1)) > 0 )
	{
		clip(-1);
		diffuseLighting = specularLighting = float3(0,0,0);
		return;
	}
	else
	{
		flashlightColor = tex2D( FlashlightSampler, vProjCoords );

		[branch]
		if ( dot(flashlightColor.xyz, float3(1,1,1)) <= 0 )
		{
			clip(-1);
			diffuseLighting = specularLighting = float3(0,0,0);
			return;
		}
	}
#else
	flashlightColor = tex2D( FlashlightSampler, vProjCoords );
#endif


#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
	flashlightColor *= cFlashlightColor.xyz;						// Flashlight color
#endif

	float3 delta = flashlightPos - worldPos;
	float3 L = normalize( delta );
	float distSquared = dot( delta, delta );
	float dist = sqrt( distSquared );

	float endFalloffFactor = RemapValClamped( dist, farZ, 0.6f * farZ, 0.0f, 1.0f );

	// Attenuation for light and to fade out shadow over distance
	float fAtten = saturate( dot( attenuationFactors, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) ) );

	// Shadowing and coloring terms
#if (defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0))
	if ( bDoShadows )
	{
		float flShadow = DoFlashlightShadow( FlashlightDepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, nShadowLevel, vShadowTweaks, bAllowHighQuality );
		float flAttenuated = lerp( flShadow, 1.0f, vShadowTweaks.y );	// Blend between fully attenuated and not attenuated
		flShadow = saturate( lerp( flAttenuated, flShadow, fAtten ) );	// Blend between shadow and above, according to light attenuation
		flashlightColor *= flShadow;									// Shadow term
	}
#endif

	diffuseLighting = fAtten;
#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
		diffuseLighting *= saturate( dot( L.xyz, worldNormal.xyz ) + flFlashlightNoLambertValue ); // Lambertian term
#else
		diffuseLighting *= saturate( dot( L.xyz, worldNormal.xyz ) ); // Lambertian (not Half-Lambert) term
#endif
	diffuseLighting *= flashlightColor;
	diffuseLighting *= endFalloffFactor;

	// Specular term (masked by diffuse)
	specularLighting = diffuseLighting * SpecularLight ( worldNormal, L, fSpecularExponent, vEyeDir, /*bDoSpecularWarp, specularWarpSampler,*/ fFresnel );
}

// Diffuse only version
float3 DoFlashlight( float3 flashlightPos, float3 worldPos, float4 flashlightSpacePosition, float3 worldNormal, 
					float3 attenuationFactors, float farZ, sampler FlashlightSampler, sampler FlashlightDepthSampler,
					sampler RandomRotationSampler, int nShadowLevel, bool bDoShadows, bool bAllowHighQuality,
					const float2 vScreenPos, bool bClip, float4 vShadowTweaks = float4(3/1024.0f, 0.0005f, 0.0f, 0.0f), bool bHasNormal = true )
{
	float3 vProjCoords = flashlightSpacePosition.xyz / flashlightSpacePosition.w;
	float3 flashlightColor = float3(1,1,1);

#if ( defined( _X360 ) )

	float3 ltz = vProjCoords.xyz < float3( 0.0f, 0.0f, 0.0f );
	float3 gto = vProjCoords.xyz > float3( 1.0f, 1.0f, 1.0f );

	[branch]
	if ( dot(ltz + gto, float3(1,1,1)) > 0 )
	{
		if ( bClip )
		{
			clip(-1);
		}
		return float3(0,0,0);
	}
	else
	{
		flashlightColor = tex2D( FlashlightSampler, vProjCoords );

		[branch]
		if ( dot(flashlightColor.xyz, float3(1,1,1)) <= 0 )
		{
			if ( bClip )
			{
				clip(-1);
			}
			return float3(0,0,0);
		}
	}
#else
	flashlightColor = tex2D( FlashlightSampler, vProjCoords );
#endif

#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
	flashlightColor *= cFlashlightColor.xyz;						// Flashlight color
#endif

	float3 delta = flashlightPos - worldPos;
	float3 L = normalize( delta );
	float distSquared = dot( delta, delta );
	float dist = sqrt( distSquared );

	float endFalloffFactor = RemapValClamped( dist, farZ, 0.6f * farZ, 0.0f, 1.0f );

	// Attenuation for light and to fade out shadow over distance
	float fAtten = saturate( dot( attenuationFactors, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) ) );

	// Shadowing and coloring terms
#if (defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0))
	if ( bDoShadows )
	{
		float flShadow = DoFlashlightShadow( FlashlightDepthSampler, RandomRotationSampler, vProjCoords, vScreenPos, nShadowLevel, vShadowTweaks, bAllowHighQuality );
		float flAttenuated = lerp( flShadow, 1.0f, vShadowTweaks.y );	// Blend between fully attenuated and not attenuated
		flShadow = saturate( lerp( flAttenuated, flShadow, fAtten ) );	// Blend between shadow and above, according to light attenuation
		flashlightColor *= flShadow;									// Shadow term
	}
#endif

	float3 diffuseLighting = fAtten;

	float flLDotWorldNormal;
	if ( bHasNormal )
	{
		flLDotWorldNormal = dot( L.xyz, worldNormal.xyz );
	}
	else
	{
		flLDotWorldNormal = 1.0f;
	}

#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
	diffuseLighting *= saturate( flLDotWorldNormal + flFlashlightNoLambertValue ); // Lambertian term
#else
	diffuseLighting *= saturate( flLDotWorldNormal ); // Lambertian (not Half-Lambert) term
#endif

	diffuseLighting *= flashlightColor;
	diffuseLighting *= endFalloffFactor;

	return diffuseLighting;
}

#endif //#ifndef COMMON_FLASHLIGHT_FXC_H_
