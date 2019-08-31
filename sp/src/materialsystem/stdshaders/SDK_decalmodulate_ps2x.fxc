//========== Copyright (c) Valve Corporation, All rights reserved. ==========//
// paired with "vertexlit_and_unlit_generic_vs##"

// STATIC: "VERTEXALPHA"				"0..1"
// STATIC: "FLASHLIGHT"					"0..1"
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"						[ps20b] [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"						[ps30] [PC]

// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"						[ps20b]
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"						[ps30] [PC]

// We don't care about flashlight depth unless the flashlight is on
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )		[ps20b]
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )		[ps30]

// Flashlight shadow filter mode is irrelevant if there is no flashlight
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 ) [ps20b]
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 ) [ps30]

#include "common_ps_fxc.h"
#include "common_flashlight_fxc.h"
#include "shader_constant_register_map.h"

sampler TexSampler	: register( s0 );

sampler RandRotSampler						: register( s6 );	// RandomRotation sampler
sampler FlashlightSampler					: register( s7 );
sampler ShadowDepthSampler					: register( s8 );	// Flashlight shadow depth map sampler

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );
const float4 g_FogTweakParams			: register( c0 );
#define g_fFogExponentTweak	g_FogTweakParams.x
#define g_fFogScaleTweak	g_FogTweakParams.y

const float4 g_FlashlightAttenuationFactors	: register( c22 );
const float3 g_FlashlightPos				: register( c23 );
const float4x4 g_FlashlightWorldToTexture	: register( c24 ); // through c27

struct PS_INPUT
{
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	
	float4 worldPos_projPosZ		: TEXCOORD1;		// Necessary for pixel fog

    float4 color					: COLOR1;
	
#if FLASHLIGHT
	float4 vProjPos					: TEXCOORD2;
#endif
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 result = tex2D( TexSampler, i.baseTexCoord );
	
	// Blend towards grey based on alpha
	float flFactor = 1.0;
#if VERTEXALPHA
	flFactor *= i.color.w;
#endif

#if FLASHLIGHT
	//if( bFlashlight )
	{
		int nShadowSampleLevel = 0;
		bool bDoShadows = false;
		float2 vProjPos = float2(0, 0);
// On ps_2_b, we can do shadow mapping
#if ( FLASHLIGHTSHADOWS && (defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0) ) )
		nShadowSampleLevel = FLASHLIGHTDEPTHFILTERMODE;
		bDoShadows = FLASHLIGHTSHADOWS;
		vProjPos = i.vProjPos.xy / i.vProjPos.w;	// Screen-space position for shadow map noise
#endif

		float4 flashlightSpacePosition = mul( float4( i.worldPos_projPosZ.xyz, 1.0f ), g_FlashlightWorldToTexture );

		float3 flashlightColor = DoFlashlight( g_FlashlightPos, i.worldPos_projPosZ.xyz, flashlightSpacePosition,
			float3( 0.0f, 0.0f, 1.0f ), g_FlashlightAttenuationFactors.xyz, 
			g_FlashlightAttenuationFactors.w, FlashlightSampler, ShadowDepthSampler,
			RandRotSampler, nShadowSampleLevel, bDoShadows, false, vProjPos, false, float4(3/1024.0f, 0.0005f, 0.0f, 0.0f), false );
			
		flFactor *= (flashlightColor.x + flashlightColor.y + flashlightColor.z);

		//result.xyz *= flashlightColor.xyz;
		
		//result.a *= (flashlightColor.x * flashlightColor.y * flashlightColor.z);
	}
#endif

	result.xyz = lerp( float3( 0.5, 0.5, 0.5 ), result.xyz, flFactor );

	// Since we're blending with a mod2x, we need to compensate with this hack
	// NOTE: If the fog color (not fog density) is extremely dark, this can makes some decals seem
	//       a little transparent, but it's better than not doing this
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.xyz, i.worldPos_projPosZ.xyz, i.worldPos_projPosZ.w );
	fogFactor = pow( saturate( g_fFogScaleTweak * fogFactor ), g_fFogExponentTweak );

	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE );
}
