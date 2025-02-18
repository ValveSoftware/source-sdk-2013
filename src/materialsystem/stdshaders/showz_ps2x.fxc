// STATIC: "DEPTH_IN_ALPHA"				"0..1"
// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]

#include "common_ps_fxc.h"

const float4 g_Parameters	: register( c0 );

sampler DepthTextureSampler : register( s0 );	// Scalar shadow depth map

float4 main( float2 baseTexCoord : TEXCOORD0 ) : COLOR
{
	float fDepth = 0;

#	if (DEPTH_IN_ALPHA == 1)
	{
		fDepth = tex2D( DepthTextureSampler, baseTexCoord ).a;
	
	}
#	else
	{
#		if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) //Minimum requirement of ps2b
		{
			fDepth = tex2D( DepthTextureSampler, baseTexCoord ).r;
		}
#		endif
	}
#	endif

	fDepth = pow( fDepth, g_Parameters.x );

	return FinalOutput( float4( fDepth, fDepth, fDepth, 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

