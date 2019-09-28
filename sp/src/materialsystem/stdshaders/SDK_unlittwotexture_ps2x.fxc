//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

// STATIC: "TRANSLUCENT"				"0..1"

// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]

#if defined( SHADER_MODEL_PS_2_0 )
	#define WRITE_DEPTH_TO_DESTALPHA 0
#endif

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

const HALF4 g_DiffuseModulation	: register( c1 );
#if !FLASHLIGHT
		// we don't use these with HDR.
		const HALF3 g_EnvmapContrast		: register( c2 );
		const HALF3 g_EnvmapSaturation		: register( c3 );
#endif

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

const float4 g_FlashlightAttenuationFactors	    : register( c22 );
const HALF3 g_FlashlightPos						: register( c23 );
const float4x4 g_FlashlightWorldToTexture		: register( c24 ); // through c27

sampler BaseTextureSampler	   : register( s0 );
sampler BaseTextureSampler2	   : register( s1 );

struct PS_INPUT
{
    float4 projPos					: POSITION;			// Projection-space position	
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	HALF2 baseTexCoord2				: TEXCOORD1;		// Base texture coordinate
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for water fog dest alpha
	
#if defined( _X360 ) //matching pixel shader inputs to vertex shader outputs to avoid shader patches	
	float4 vColor	: COLOR0;
#endif
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoord.xy );
	float4 baseColor2 = tex2D( BaseTextureSampler2, i.baseTexCoord2.xy );
	float4 result = baseColor * baseColor2 * g_DiffuseModulation;

	// This material can only get a non-opaque alpha if the material is marked as translucent
# if ( TRANSLUCENT == 0 )
	result.a = 1.0f;
# endif

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.xyz, i.worldPos_projPosZ.xyz, i.worldPos_projPosZ.w );
	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, (WRITE_DEPTH_TO_DESTALPHA != 0), i.worldPos_projPosZ.w );
}

