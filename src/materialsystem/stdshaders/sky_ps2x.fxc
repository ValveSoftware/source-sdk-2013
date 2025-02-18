// HDRFIXME: Make this work with nonHDR
// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]
#include "common_ps_fxc.h"

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

sampler BaseTextureSampler	: register( s0 );
HALF4 InputScale			: register( c0 );

struct PS_INPUT
{
	float2 baseTexCoord		: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	HALF4 color = tex2D( BaseTextureSampler, i.baseTexCoord.xy );
	color.rgb *= InputScale.rgb;
	
	// This is never fogged.
	return FinalOutput( color, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR, WRITE_DEPTH_TO_DESTALPHA, 1e20 ); //when writing depth to dest alpha, write a value guaranteed to saturate
}
