// STATIC: "CONVERT_TO_SRGB"		"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"		"0..0"	[= 0] [XBOX]
// STATIC: "APPROX_SRGB_ADAPTER"	"0..1" [ps20b] [PC]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler TexSampler	: register( s0 );

struct PS_INPUT
{
	float2 coordTap0				: TEXCOORD0;
	float2 coordTap1				: TEXCOORD1;
	float2 coordTap2				: TEXCOORD2;
	float2 coordTap3				: TEXCOORD3;
	float2 coordTap1Neg				: TEXCOORD4;
	float2 coordTap2Neg				: TEXCOORD5;
	float2 coordTap3Neg				: TEXCOORD6;
};

float2 psTapOffs[3] : register( c0 );
float3 scale_factor : register( c3 );

float4 SampleTexture( sampler texSampler, float2 uv )
{
	float4 cSample = tex2D( texSampler, uv );

	#if ( APPROX_SRGB_ADAPTER )
	{
		cSample.rgb = max( cSample.rgb, float3(  0.00001f, 0.00001f, 0.00001f ) ); // rsqrt doesn't like inputs of zero

		float3 ooSQRT;                  //
		ooSQRT.r = rsqrt( cSample.r );  //
		ooSQRT.g = rsqrt( cSample.g );  // Approximate linear-to-sRGB conversion
		ooSQRT.b = rsqrt( cSample.b );  //
		cSample.rgb *= ooSQRT.rgb;      //
	}
	#endif

	return cSample;
}

float4 main( PS_INPUT i ) : COLOR
{
	float4 s0, s1, s2, s3, s4, s5, s6, color;

	// Sample taps with coordinates from VS
	s0 = SampleTexture( TexSampler, i.coordTap0 );
	s1 = SampleTexture( TexSampler, i.coordTap1 );
	s2 = SampleTexture( TexSampler, i.coordTap2 );
	s3 = SampleTexture( TexSampler, i.coordTap3 );
	s4 = SampleTexture( TexSampler, i.coordTap1Neg );
	s5 = SampleTexture( TexSampler, i.coordTap2Neg );
	s6 = SampleTexture( TexSampler, i.coordTap3Neg );

	color = s0 * 0.2013f;
	color += ( s1 + s4 ) * 0.2185f;
	color += ( s2 + s5 ) * 0.0821f;
	color += ( s3 + s6 ) * 0.0461f;

	// Compute tex coords for other taps
	float2 coordTap4 = i.coordTap0 + psTapOffs[0];
	float2 coordTap5 = i.coordTap0 + psTapOffs[1];
	float2 coordTap6 = i.coordTap0 + psTapOffs[2];
	float2 coordTap4Neg = i.coordTap0 - psTapOffs[0];
	float2 coordTap5Neg = i.coordTap0 - psTapOffs[1];
	float2 coordTap6Neg = i.coordTap0 - psTapOffs[2];

	// Sample the taps
	s1 = SampleTexture( TexSampler, coordTap4 );
	s2 = SampleTexture( TexSampler, coordTap5 );
	s3 = SampleTexture( TexSampler, coordTap6 );
	s4 = SampleTexture( TexSampler, coordTap4Neg );
	s5 = SampleTexture( TexSampler, coordTap5Neg );
	s6 = SampleTexture( TexSampler, coordTap6Neg );

	color += ( s1 + s4 ) * 0.0262f;
	color += ( s2 + s5 ) * 0.0162f;
	color += ( s3 + s6 ) * 0.0102f;
	color.xyz*=scale_factor.xyz;

	#if ( APPROX_SRGB_ADAPTER )
	{
		color.xyz *= color.xyz; // Approximate sRGB-to-linear conversion
	}
	#endif

	return color;
	//return FinalOutput( color, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

