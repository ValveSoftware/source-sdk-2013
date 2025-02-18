//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "common_flashlight_fxc.h"
#include "shader_constant_register_map.h"


const float4 g_vShadowTweaks	: register( PSREG_ENVMAP_TINT__SHADOW_TWEAKS );

sampler SpotSampler				: register( s0 );
sampler BaseTextureSampler		: register( s1 );
sampler IrisSampler				: register( s3 );

#if FLASHLIGHTSHADOWS && (!SHADER_MODEL_PS_1_1) && (!SHADER_MODEL_PS_1_4) && (!SHADER_MODEL_PS_2_0)
sampler FlashlightDepthSampler	: register( s4 );
sampler RandomRotationSampler	: register( s5 );
#endif

#if defined( SHADER_MODEL_PS_1_1 ) || defined ( SHADER_MODEL_PS_1_4 )

#else
	const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
	const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );
#endif

struct PS_INPUT
{
	float4 spotTexCoord			: TEXCOORD0;
	float2 baseTexCoord			: TEXCOORD1;
	float2 irisTexCoord			: TEXCOORD3;
#if defined( SHADER_MODEL_PS_1_1 ) || defined ( SHADER_MODEL_PS_1_4 )
	float3 vertAtten			: COLOR0;
#else
	float3 vertAtten			: TEXCOORD4;
	float3 worldPos				: TEXCOORD5;
	float3 projPos				: TEXCOORD7;
#endif
};

float4 main( PS_INPUT i ) : COLOR
{
#if defined(SHADER_MODEL_PS_2_0)
	float3 spotColor = tex2Dproj( SpotSampler, i.spotTexCoord.xyzw ) * cFlashlightColor;
#elif ( defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0) )
	float3 vProjCoords = i.spotTexCoord.xyz / i.spotTexCoord.w;
	float3 spotColor = tex2D( SpotSampler, vProjCoords ) * cFlashlightColor;
#else
	float3 spotColor = tex2D( SpotSampler, i.spotTexCoord );
#endif

	float4 baseSample = tex2D( BaseTextureSampler, i.baseTexCoord );
	float4 irisSample = tex2D( IrisSampler, i.irisTexCoord );

	float3 outcolor = float3(1,1,1);

#if !defined( SHADER_MODEL_PS_1_1 ) && !defined( SHADER_MODEL_PS_1_4 )
	if( i.spotTexCoord.w <= 0.0f )
	{
		outcolor = float3(0,0,0);
	}
#endif

	// Composite the iris and sclera together
#if defined( SHADER_MODEL_PS_1_1 ) || defined ( SHADER_MODEL_PS_1_4 )
	float3 albedo = lerp( baseSample.xyz, irisSample.xyz, irisSample.a );
#else
	float3 albedo = lerp( baseSample.xyz, irisSample.xyz * 0.5f, irisSample.a );	// dim down the iris in HDR
#endif

	// Do shadow depth mapping...
#if FLASHLIGHTSHADOWS && ( defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0) )
	float flShadow = DoFlashlightShadow( FlashlightDepthSampler, RandomRotationSampler, vProjCoords, i.projPos.xy / i.projPos.z, FLASHLIGHTDEPTHFILTERMODE, g_vShadowTweaks, true );
	float flAttenuated = lerp( flShadow, 1.0f, g_vShadowTweaks.y );								// Blend between fully attenuated and not attenuated
	flShadow = lerp( flAttenuated, flShadow, dot(i.vertAtten, float3(0.30f, 0.59f, 0.11f) ) );	// Blend between shadow and above, according to light attenuation
	outcolor *= flShadow * spotColor * albedo;
#else
	outcolor *= spotColor * albedo;
#endif

	// NOTE!!  This has to be last to avoid loss of range.
	outcolor *= i.vertAtten;
#if defined( SHADER_MODEL_PS_1_1 ) || defined ( SHADER_MODEL_PS_1_4 )
	return float4( outcolor, baseSample.a );
#else
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos.z, i.projPos.z );
	return FinalOutput( float4( outcolor, 1.0f ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
#endif

}
