// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#define HDRENABLED 0
#include "common_ps_fxc.h"

sampler InputTexture	: register( s0 );
sampler Albedo	: register( s1 );
sampler Flags	: register( s2 );

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
};


float4 main( PS_INPUT i ) : COLOR
{
	float3 flags_check=tex2D( Flags, i.texCoord );
	float3 input=tex2D( InputTexture, i.texCoord );
	float3 albedo=tex2D( Albedo, i.texCoord );
	input=lerp(albedo,input,flags_check.r);
	return FinalOutput( float4(input.xyz,1), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
