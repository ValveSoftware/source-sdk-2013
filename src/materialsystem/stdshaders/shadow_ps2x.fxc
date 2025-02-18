// DYNAMIC: "PIXELFOGTYPE"		"0..1"

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

const HALF4 g_ShadowColor						: register( c1 );
const HALF3 g_EyePos							: register( c2 );
const HALF4 g_FogParams							: register( c3 );

sampler ShadowSampler	: register( s0 );

// CENTROID: TEXCOORD0
// CENTROID: TEXCOORD1
// CENTROID: TEXCOORD2
// CENTROID: TEXCOORD3
// CENTROID: TEXCOORD4
struct PS_INPUT
{
	float2 texCoord0				: TEXCOORD0;
	float2 texCoord1				: TEXCOORD1;
	float2 texCoord2				: TEXCOORD2;
	float2 texCoord3				: TEXCOORD3;
	float2 texCoord4				: TEXCOORD4;
	HALF4 worldPos_projPosZ		    : TEXCOORD5;
	HALF4 shadowColor				: COLOR0;
	HALF4 fogFactorW				: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	HALF4 samples[5];
	samples[0] = tex2D( ShadowSampler, i.texCoord0 );
	samples[1] = tex2D( ShadowSampler, i.texCoord1 );
	samples[2] = tex2D( ShadowSampler, i.texCoord2 );
	samples[3] = tex2D( ShadowSampler, i.texCoord3 );
	samples[4] = tex2D( ShadowSampler, i.texCoord4 );

	// Interpolate between a bunch of jittered shadow samples.
	HALF shadowCoverage = (samples[0].a + samples[1].a + samples[2].a + samples[3].a + samples[4].a) * 0.2;

	// To accomplish shadow fading, subtract vertex alpha from texture alpha
	shadowCoverage = saturate( shadowCoverage - i.shadowColor.a );
		
	// Blend between white and the constant color...
	// return lerp( 1.0-shadowCoverage, 1.0, g_ShadowColor );  
	
	// this is equivalent, and saves an instruction
	HALF4 result = shadowCoverage*g_ShadowColor - shadowCoverage;
	result = 1.0 + result;

	float alpha = 1.0f;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	
	// Apply fog here to compensate for our srcColor*dstColor alpha blend into already fogged pixels
	result.rgb = 1.0f - ( ( 1.0f - result.rgb ) * pow( ( 1.0f - fogFactor ), 4.0f ) );

	// Call FinalOutput without fog!
	return FinalOutput( float4( result.rgb, alpha ), fogFactor, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
