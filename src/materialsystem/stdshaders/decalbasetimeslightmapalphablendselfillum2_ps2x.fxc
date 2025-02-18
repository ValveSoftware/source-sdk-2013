// DYNAMIC: "PIXELFOGTYPE"		"0..1"

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

struct PS_INPUT
{
	float2 vTexCoord0	: TEXCOORD0;
	float4 worldPos_projPosZ		: TEXCOORD4;		// Necessary for pixel fog
};

float4 main( PS_INPUT i ) : COLOR
{
	HALF4 result = tex2D( BaseTextureSampler, i.vTexCoord0 );
	
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}
