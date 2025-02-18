// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "MACROS"				"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"


sampler BaseSampler	: register( s0 );
// NOTE: LightmapSampler is at the same place as the lightmap sampler in lightmappedgeneric so that we have
// generally the same texture state here.
// CENTROID: TEXCOORD1
sampler LightmapSampler: register( s1 );
sampler BaseSampler2: register( s2 );
sampler LightmapAlphaSampler: register( s3 );
sampler MacrosSampler: register( s4 );

struct PS_INPUT
{
	float2 baseCoord				: TEXCOORD0;
	float2 baseCoord2				: TEXCOORD1;
	float2 lightmapCoord			: TEXCOORD2;
	float2 macrosCoord				: TEXCOORD3;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bMacros = MACROS ? true : false;

	float4 base = tex2D( BaseSampler, i.baseCoord );
	float4 base2 = tex2D( BaseSampler2, i.baseCoord2 );

	float4 lightmap = tex2D( LightmapSampler, i.lightmapCoord );
	float blendFactor = lightmap.a;

	float4 color = 2.0f * lightmap * lerp( base2, base, blendFactor );
	if( bMacros )
	{
		float4 macros = tex2D( MacrosSampler, i.macrosCoord );

		// Not sure what to do with macro alpha
		color.rgb *= 2.0f * lerp( macros.a, macros.b, blendFactor );
	}

	return FinalOutput( color, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

