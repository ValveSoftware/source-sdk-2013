// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#include "common_ps_fxc.h"

struct PS_INPUT
{
	float4 vColor	: COLOR0;
};

HALF4 main( PS_INPUT i ) : COLOR
{
	return i.vColor;
}
