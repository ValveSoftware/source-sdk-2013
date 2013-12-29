// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
    float4 projPos					: POSITION;	
	float3 zValue					: TEXCOORD0;
};

const float3 g_ZFilter				: register( c1 );
const float3 g_ModulationColor		: register( c2 );

float4 main( PS_INPUT i ) : COLOR
{
	float z = dot( i.zValue, g_ZFilter );
	z = saturate(  z );
	float4 color = float4( z, z, z, 1.0f );
	color.rgb *= g_ModulationColor;
	return FinalOutput( color, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
