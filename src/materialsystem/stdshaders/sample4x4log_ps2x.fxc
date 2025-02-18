// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"


sampler TexSampler	: register( s0 );

struct PS_INPUT
{
	float2 coordTap0				: TEXCOORD0;
	float2 coordTap1				: TEXCOORD1;
	float2 coordTap2				: TEXCOORD2;
	float2 coordTap3				: TEXCOORD3;
};

float AlphaConst : register( c0 );

#define LOG_EPSILON 0.000001

float luminance(float3 color) : FLOAT
{
	return 0.2125*color.x+0.7154*color.y+0.0721*color.z;
}

float logluminance(float3 color) : FLOAT
{
	return log(0.2125*color.x+0.7154*color.y+0.0721*color.z+LOG_EPSILON);
}

float4 main( PS_INPUT i ) : COLOR
{
	float3 s0, s1, s2, s3;

	// Sample 4 taps. We use the trick of sampling four taps with bilinear in order
	// to average 16 texels.
	s0 = tex2D( TexSampler, i.coordTap0);
	s1 = tex2D( TexSampler, i.coordTap1);
	s2 = tex2D( TexSampler, i.coordTap2);
	s3 = tex2D( TexSampler, i.coordTap3);
	float maxlum=max(max(luminance(s0),luminance(s1)),max(luminance(s2),luminance(s3)));
	float loglum0=logluminance(s0);
	float loglum1=logluminance(s1);
	float loglum2=logluminance(s2);
	float loglum3=logluminance(s3);

//	return float4(0.25*(loglum0+loglum1+loglum2+loglum3),maxlum,0,1);
	return FinalOutput( float4(0.25*(loglum0+loglum1+loglum2+loglum3),0,.5,AlphaConst), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}

