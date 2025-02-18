// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1" [ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0" [ps20b] [XBOX]

// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

sampler BaseTextureSampler	: register( s0 );

const float4 g_WhiteGrayMix	: register( c0 );
const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

struct PS_INPUT
{
	float2 vTexCoord0	: TEXCOORD0;
	float4 vColor		: COLOR0;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 textureColor = tex2D( BaseTextureSampler, i.vTexCoord0 );

	float4 resultColor = saturate( textureColor * i.vColor );
	resultColor.rgb = lerp( g_WhiteGrayMix.rgb, resultColor.rgb, resultColor.a );

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( resultColor, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE, (WRITE_DEPTH_TO_DESTALPHA != 0), i.worldPos_projPosZ.w );
}
