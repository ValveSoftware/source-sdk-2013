// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "PASS"				"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

// CENTROID: TEXCOORD1

sampler BaseSampler	: register( s0 );
sampler LightmapSampler: register( s1 );
sampler LightmapAlphaSampler: register( s2 );

struct PS_INPUT
{
	float2 baseCoord				: TEXCOORD0;
	float2 lightmapCoord			: TEXCOORD1;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bAlphaPass = PASS ? true : false;

	float4 base = tex2D( BaseSampler, i.baseCoord );
	float4 lightmap = tex2D( LightmapSampler, i.lightmapCoord );
	float4 alpha = tex2D( LightmapAlphaSampler, i.lightmapCoord );

	float4 color;

	base.a = dot( base, HALF3( HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f) ) );
	color = 2.0f * base * lightmap; // The 2x is for an assumed overbright 2 (it's always 2 on dx9)

	if( bAlphaPass )
	{
		// Don't care about color, just return pre-multiplied alpha
		return FinalOutput( float4( 0.0f, 0.0f, 1.0f, (1.0f - alpha.a) * color.a ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
	else
	{
		return FinalOutput( float4( color.rgb, (1.0f - alpha.a) ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
}

