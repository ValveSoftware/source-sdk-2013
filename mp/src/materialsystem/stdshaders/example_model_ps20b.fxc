//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example pixel shader that can be applied to models
//
//==================================================================================================

// STATIC: "CONVERT_TO_SRGB"			"0..0"
// STATIC: "FLASHLIGHT"					"0..1"
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"	[ps20b]

// DYNAMIC: "WRITEWATERFOGTODESTALPHA"  "0..1"
// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..4"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b]
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"	[ps20b]

// SKIP: ($PIXELFOGTYPE == 0) && ($WRITEWATERFOGTODESTALPHA != 0)

// We don't care about flashlight depth unless the flashlight is on
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )

// Flashlight shadow filter mode is irrelevant if there is no flashlight
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 ) [ps20b]

#include "common_flashlight_fxc.h"
#include "shader_constant_register_map.h"

const float4 g_DiffuseModulation			: register( PSREG_DIFFUSE_MODULATION );
const float4 g_ShadowTweaks					: register( PSREG_ENVMAP_TINT__SHADOW_TWEAKS );
const float3 cAmbientCube[6]				: register( PSREG_AMBIENT_CUBE );
const float4 g_EyePos						: register( PSREG_EYEPOS_SPEC_EXPONENT );
const float4 g_FogParams					: register( PSREG_FOG_PARAMS );
const float4 g_FlashlightAttenuationFactors	: register( PSREG_FLASHLIGHT_ATTENUATION );			// On non-flashlight pass
const float4 g_FlashlightPos_RimBoost		: register( PSREG_FLASHLIGHT_POSITION_RIM_BOOST );
const float4x4 g_FlashlightWorldToTexture	: register( PSREG_FLASHLIGHT_TO_WORLD_TEXTURE );
PixelShaderLightInfo cLightInfo[3]			: register( PSREG_LIGHT_INFO_ARRAY );				// 2 registers each - 6 registers total (4th light spread across w's)

#define g_FlashlightPos					g_FlashlightPos_RimBoost.xyz

sampler BaseTextureSampler		: register( s0 );	// Base map, selfillum in alpha
sampler ShadowDepthSampler		: register( s4 );	// Flashlight shadow depth map sampler
sampler NormalizeRandRotSampler	: register( s5 );	// Normalization / RandomRotation samplers
sampler FlashlightSampler		: register( s6 );	// Flashlight cookie

struct PS_INPUT
{
	float2 baseTexCoord	: TEXCOORD0;
	float4 lightAtten	: TEXCOORD1;
	float3 worldNormal	: TEXCOORD2;
	float3 worldPos		: TEXCOORD3;
	float3 projPos		: TEXCOORD4;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );

	float3 diffuseLighting;
	if ( FLASHLIGHT != 0 )
	{
		float4 flashlightSpacePosition = mul( float4( i.worldPos, 1.0f ), g_FlashlightWorldToTexture );

		diffuseLighting = DoFlashlight( g_FlashlightPos, i.worldPos, flashlightSpacePosition,
			i.worldNormal, g_FlashlightAttenuationFactors.xyz, 
			g_FlashlightAttenuationFactors.w, FlashlightSampler, ShadowDepthSampler,
			NormalizeRandRotSampler, FLASHLIGHTDEPTHFILTERMODE, FLASHLIGHTSHADOWS, true, i.projPos, false, g_ShadowTweaks );
	}
	else // non-flashlight path
	{
		// Summation of diffuse illumination from all local lights
		diffuseLighting = PixelShaderDoLighting( i.worldPos, i.worldNormal,
			float3( 0.0f, 0.0f, 0.0f ), false, true, i.lightAtten,
			cAmbientCube, NormalizeRandRotSampler, NUM_LIGHTS, cLightInfo, true,

			// These are dummy parameters:
			false, 1.0f,
			false, BaseTextureSampler );
	}

	float3 result = baseColor.rgb * g_DiffuseModulation.rgb * diffuseLighting;
	float alpha = g_DiffuseModulation.a * baseColor.a;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos.z, i.projPos.z );

#if WRITEWATERFOGTODESTALPHA && ( PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT )
	alpha = fogFactor;
#endif

	bool bWriteDepthToAlpha = ( WRITE_DEPTH_TO_DESTALPHA != 0 ) && ( WRITEWATERFOGTODESTALPHA == 0 );

	return FinalOutput( float4( result, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, i.projPos.z );
}
