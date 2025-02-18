//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

// STATIC: "CONVERT_TO_SRGB"		"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"		"0..0"	[= 0] [XBOX]
// STATIC: "CUBEMAP"				"0..1"
// STATIC: "FLOWMAP"				"0..1"
// STATIC: "CORECOLORTEXTURE"		"0..1"
// STATIC: "REFRACT"				"0..1"
// DYNAMIC: "PIXELFOGTYPE"			"0..1"

// SKIP: ( $REFRACT || $CORECOLORTEXTURE ) && $CUBEMAP

#include "common_ps_fxc.h"

sampler RefractSampler	: register( s2 );
sampler NormalSampler	: register( s3 );
#if CUBEMAP
sampler EnvmapSampler			: register( s4 );
#endif
#if FLOWMAP
sampler FlowmapSampler			: register( s6 );
#endif

#if CORECOLORTEXTURE
sampler CoreColorSampler			: register( s7 );
#endif

const HALF3 g_EnvmapTint			: register( c0 );
const HALF3 g_RefractTint : register( c1 );
const HALF3 g_EnvmapContrast		: register( c2 );
const HALF3 g_EnvmapSaturation		: register( c3 );
const HALF2  g_RefractScale : register( c5 );
#if FLOWMAP
const float g_Time								: register( c6 );
const float2 g_FlowScrollRate	: register( c7 );
//const float3 g_SphereCenter		: register( c9 );
//const float3 g_SphereRadius		: register( c10 );
const float g_CoreColorTexCoordOffset : register( c9 );
#endif

const float3 g_EyePos			: register( c8 );
const float4 g_FogParams		: register( c11 );

float LengthThroughSphere( float3 vecRayOrigin, float3 vecRayDelta, 
						  float3 vecSphereCenter, float flRadius, out float alpha )
{
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	float3 vecSphereToRay;
	vecSphereToRay = vecRayOrigin - vecSphereCenter;

	float a = dot( vecRayDelta, vecRayDelta );

	// This would occur in the case of a zero-length ray
	//	if ( a == 0.0f )
	//	{
	//		*pT1 = *pT2 = 0.0f;
	//		return vecSphereToRay.LengthSqr() <= flRadius * flRadius;
	//	}

	float b = 2 * dot( vecSphereToRay, vecRayDelta );
	float c = dot( vecSphereToRay, vecSphereToRay ) - flRadius * flRadius;
	float flDiscrim = b * b - 4 * a * c;
	//	if ( flDiscrim < 0.0f )
	//		return 0.0f;

	float hack = flDiscrim;
	flDiscrim = sqrt( flDiscrim );
	float oo2a = 0.5f / a;
	

	//if( hack < 0.0f )
	//{
	//	alpha = 0.0f;
	//	return 0.0f;
	//}
	//else
	//{
	//	alpha = 1.0f;
	//	return abs( flDiscrim ) * 2 * oo2a;
	//}
	
	//replacing the if's above because if's in hlsl are bad.....
	float fHackGreaterThanZero = step( 0.0f, hack );
	alpha = fHackGreaterThanZero;
	return (fHackGreaterThanZero * (abs( flDiscrim ) * 2 * oo2a));


	//	*pT1 = ( - b - flDiscrim ) * oo2a;
	//	*pT2 = ( - b + flDiscrim ) * oo2a;
	//	return true;
}


struct PS_INPUT
{
	float2 vBumpTexCoord			: TEXCOORD0; // dudvMapAndNormalMapTexCoord
	HALF3 vWorldVertToEyeVector		: TEXCOORD1;
	HALF3x3 tangentSpaceTranspose	: TEXCOORD2;
	float3 vRefractXYW				: TEXCOORD5; 
	float3 projNormal				: TEXCOORD6;
	float4 worldPos_projPosZ		: TEXCOORD7;
};

float4 main( PS_INPUT i ) : COLOR
{
	HALF3 result = 0.0f;

	HALF blend = 1.0f;

#if FLOWMAP
	// hack
	float3 g_SphereCenter = { 2688.0f, 12139.0f, 5170.0f };
	float g_SphereDiameter = 430.0f;
	float g_SphereRadius = g_SphereDiameter * 0.5f;

	float3 tmp = i.worldPos_projPosZ.xyz - g_SphereCenter;
	float hackRadius = 1.05f * sqrt( dot( tmp, tmp ) );

	float sphereAlpha;
	float lengthThroughSphere = LengthThroughSphere( g_EyePos, normalize( i.worldPos_projPosZ.xyz - g_EyePos ),
		g_SphereCenter, /*g_SphereRadius*/ hackRadius, sphereAlpha );

	float normalizedLengthThroughSphere = lengthThroughSphere / g_SphereDiameter;


	float3 hackWorldSpaceNormal = normalize( i.worldPos_projPosZ.xyz - g_SphereCenter );
	float3 realFuckingNormal = abs( hackWorldSpaceNormal );
	hackWorldSpaceNormal = 0.5f * ( hackWorldSpaceNormal + 1.0f );

	//	hackWorldSpaceNormal = abs( hackWorldSpaceNormal );

	//	return float4( hackWorldSpaceNormal.x, 0.0f, 0.0f, 1.0f );

	i.vBumpTexCoord.xy = 0.0f;
	i.vBumpTexCoord.xy = realFuckingNormal.z * tex2D( FlowmapSampler, hackWorldSpaceNormal.xy );
	i.vBumpTexCoord.xy += realFuckingNormal.y * tex2D( FlowmapSampler, hackWorldSpaceNormal.xz );
	i.vBumpTexCoord.xy += realFuckingNormal.x * tex2D( FlowmapSampler, hackWorldSpaceNormal.yz );
	i.vBumpTexCoord.xy += g_Time * g_FlowScrollRate;
	//	return float4( i.vBumpTexCoord.xy, 0.0f, 0.0f );
#endif

	// Load normal and expand range
	HALF4 vNormalSample = tex2D( NormalSampler, i.vBumpTexCoord );
	//	return vNormalSample;
	HALF3 tangentSpaceNormal = vNormalSample * 2.0 - 1.0;

	HALF3 refractTintColor = g_RefractTint;

	// Perform division by W only once
	float ooW = 1.0f / i.vRefractXYW.z;

	// Compute coordinates for sampling refraction
	float2 vRefractTexCoordNoWarp = i.vRefractXYW.xy * ooW;
	float2 vRefractTexCoord = tangentSpaceNormal.xy;
	HALF scale = vNormalSample.a * g_RefractScale.x;
#if FLOWMAP
	scale *= normalizedLengthThroughSphere;
#endif
	vRefractTexCoord *= scale;
#if FLOWMAP
	float2 hackOffset = vRefractTexCoord;
#endif
	vRefractTexCoord += vRefractTexCoordNoWarp;

	float3 colorWarp = tex2D( RefractSampler, vRefractTexCoord.xy );
	float3 colorNoWarp = tex2D( RefractSampler, vRefractTexCoordNoWarp.xy );

	colorWarp *= refractTintColor;
#if REFRACT
	result = lerp( colorNoWarp, colorWarp, blend );
	//	return float4( 1.0f, 0.0f, 0.0f, 1.0f );
#endif

#if CUBEMAP
	HALF specularFactor = vNormalSample.a;

	HALF3 worldSpaceNormal = mul( i.tangentSpaceTranspose, tangentSpaceNormal );

	HALF3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, i.vWorldVertToEyeVector );
	HALF3 specularLighting = texCUBE( EnvmapSampler, reflectVect );
	specularLighting *= specularFactor;
	specularLighting *= g_EnvmapTint;
	HALF3 specularLightingSquared = specularLighting * specularLighting;
	specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
	HALF3 greyScale = dot( specularLighting, HALF3( 0.299f, 0.587f, 0.114f ) );
	specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
	result += specularLighting;
#endif

#if CORECOLORTEXTURE && FLOWMAP
	float4 coreColorTexel = tex2D( CoreColorSampler, hackOffset + float2( normalizedLengthThroughSphere, g_CoreColorTexCoordOffset ) );
	HALF4 rgba = HALF4( lerp( result, coreColorTexel, coreColorTexel.a /*normalizedLengthThroughSphere*/ ), sphereAlpha );
#else
	HALF4 rgba = HALF4( result, vNormalSample.a );
#endif


	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( rgba, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE );
}

