// DYNAMIC: "NUM_LOOKUPS" "0..4"

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler BaseTextureSampler				: register( s0 );
sampler ColorCorrectionVolumeTexture0	: register( s1 );
sampler ColorCorrectionVolumeTexture1	: register( s2 );
sampler ColorCorrectionVolumeTexture2	: register( s3 );
sampler ColorCorrectionVolumeTexture3	: register( s4 );

float ColorCorrectionDefaultWeight : register( c0 );
float ColorCorrectionVolumeWeight0 : register( c1 );
float ColorCorrectionVolumeWeight1 : register( c2 );
float ColorCorrectionVolumeWeight2 : register( c3 );
float ColorCorrectionVolumeWeight3 : register( c4 );

struct PS_INPUT
{
	float2 baseTexCoord : TEXCOORD0;
};
	   
float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );
	
	// NOTE: This code requires the color correction texture to be 32 units to be correct.
	// This code will cause (0,0,0) to be read from 0.5f/32
	// and (1,1,1) to be read from 31.5f/32
	float4 offsetBaseColor = baseColor*( 31.0f / 32.0f ) + ( 0.5f / 32.0f );

	float4 outColor = float4( 0.0f, 0.0f, 0.0f, baseColor.a ); 
	outColor.rgb = baseColor * ColorCorrectionDefaultWeight;
	if (NUM_LOOKUPS > 0)
	{
		outColor.rgb += tex3D( ColorCorrectionVolumeTexture0, offsetBaseColor.rgb ) * ColorCorrectionVolumeWeight0;
		if (NUM_LOOKUPS > 1)
		{
			outColor.rgb += tex3D( ColorCorrectionVolumeTexture1, offsetBaseColor.rgb ) * ColorCorrectionVolumeWeight1;
			if (NUM_LOOKUPS > 2)
			{
				outColor.rgb += tex3D( ColorCorrectionVolumeTexture2, offsetBaseColor.rgb ) * ColorCorrectionVolumeWeight2;
				if (NUM_LOOKUPS > 3)
				{
					outColor.rgb += tex3D( ColorCorrectionVolumeTexture3, offsetBaseColor.rgb ) * ColorCorrectionVolumeWeight3;
				}
			}
		}
	}

	return FinalOutput( outColor, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
