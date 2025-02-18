// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]
#include "common_ps_fxc.h"

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

sampler ExposureTextureSampler0	: register( s0 );
sampler ExposureTextureSampler1	: register( s1 );
sampler ExposureTextureSampler2	: register( s2 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	HALF3 color0 = 0.25*tex2D( ExposureTextureSampler0, i.baseTexCoord );
	HALF3 color1 = 2.0*tex2D( ExposureTextureSampler1, i.baseTexCoord );
	HALF3 color2 = 16.0*tex2D( ExposureTextureSampler2, i.baseTexCoord );

	// This is never fogged.
//	return FinalOutput( float4( max(max(color0,color1),color2), 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR, WRITE_DEPTH_TO_DESTALPHA, 1e20 ); //when writing depth to dest alpha, write a value guaranteed to saturate
	return FinalOutput( float4(1,0,0,1 ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR, WRITE_DEPTH_TO_DESTALPHA, 1e20 ); //when writing depth to dest alpha, write a value guaranteed to saturate
}
