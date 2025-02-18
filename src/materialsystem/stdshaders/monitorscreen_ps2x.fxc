// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// STATIC: "TEXTURE2"					"0..1"
// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );
sampler SecondaryTextureSampler	: register( s1 );


const float4 g_Contrast		: register( c1 );
const float4 g_Saturation	: register( c2 );
const float4 g_Tint			: register( c3 );
const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

#define g_Grey float4( 0.33333f, 0.33333f, 0.33333f, 0.33333f )

struct PS_INPUT
{
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	
	float4 vColor		: COLOR0;
	
	float4 worldPos_projPosZ		: TEXCOORD7;
};

float4 main( PS_INPUT i ) : COLOR
{	
	float4 resultColor = tex2D( BaseTextureSampler, i.vTexCoord0 ) * i.vColor; // base texture modulated with vertex color
		
#if (TEXTURE2 == 1)
	resultColor = tex2D( SecondaryTextureSampler, i.vTexCoord1 ) * resultColor; // modulate base color by another texture
#endif

	float3 tempColor = resultColor.rgb * resultColor.rgb;								//base * base
	resultColor.rgb = lerp( resultColor.rgb, tempColor.rgb, g_Contrast.rgb );			// blend between color and color * color
	tempColor = dot( resultColor.rgb, g_Grey );						// color greyscaled
	resultColor.rgb = lerp( tempColor.rgb, resultColor.rgb, g_Saturation.rgb );			// blend between color and greyscale
	resultColor.rgb = resultColor.rgb * g_Tint.rgb;										// tint
	
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( resultColor, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE, (WRITE_DEPTH_TO_DESTALPHA != 0), i.worldPos_projPosZ.w );
}
