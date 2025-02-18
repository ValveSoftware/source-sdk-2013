// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

// this pixel shader compares the luminance against a conatnt value and retruns all 1s when
// greater.

struct PS_INPUT
{
	float2 coordTap0				: TEXCOORD0;
	
#if defined( _X360 ) //matching pixel shader inputs to vertex shader outputs to avoid shader patches	
	float2 ZeroTexCoord				: TEXCOORD1;
    float2 bloomTexCoord            : TEXCOORD2;
#endif
};

float4 Constant_color : register( c0 );

float4 main( PS_INPUT i ) : COLOR
{
	return FinalOutput( float4(0,1,0,1), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE ); //Constant_color;
}

