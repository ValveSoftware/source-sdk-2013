// STATIC: "SRGB_ADAPTER"		"0..1"  [ps20b] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b] [= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "CSTRIKE"			"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler TexSampler	: register( s0 );
float4 params : register( c0 );

struct PS_INPUT
{
	float2 coordTap0				: TEXCOORD0;
	float2 coordTap1				: TEXCOORD1;
	float2 coordTap2				: TEXCOORD2;
	float2 coordTap3				: TEXCOORD3;
};

#if ( CSTRIKE == 0 )
	//---------------------------------------//
	// Everything but Counter-Strike: Source //
	//---------------------------------------//
	float4 Shape( float2 uv )
	{
		float4 pixel = tex2D( TexSampler, uv );

		#if ( SRGB_ADAPTER == 1 )
		{
			pixel.rgb = LinearToGamma( pixel.rgb );
		}
		#endif

		float lum = dot( pixel.xyz, params.xyz );
		pixel.xyz = pow( pixel.xyz, params.w ) * lum;

		#if ( SRGB_ADAPTER == 1 )
		{
			pixel.rgb = GammaToLinear( pixel.rgb );
		}
		#endif

		return pixel;
	}

	float4 main( PS_INPUT i ) : COLOR
	{
		float4 s0, s1, s2, s3;

		// Sample 4 taps
		s0 = Shape( i.coordTap0 );
		s1 = Shape( i.coordTap1 );
		s2 = Shape( i.coordTap2 );
		s3 = Shape( i.coordTap3 );

		float4 avgColor = ( s0 + s1 + s2 + s3 ) * 0.25f;
		return FinalOutput( avgColor, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
#else
	//------------------------//
	// Counter-Strike: Source //
	//------------------------//
	float3 Shape( float3 s )
	{
		float lum = ( 0.3f * s.x ) + ( 0.59f * s.y ) + ( 0.11f * s.z );
		return lum * s;
	}

	float4 main( PS_INPUT i ) : COLOR
	{
		float3 s0, s1, s2, s3;

		// Sample 4 taps
		s0 = Shape( GammaToLinear( tex2D( TexSampler, i.coordTap0 ) ) );
		s1 = Shape( GammaToLinear( tex2D( TexSampler, i.coordTap1 ) ) );
		s2 = Shape( GammaToLinear( tex2D( TexSampler, i.coordTap2 ) ) );
		s3 = Shape( GammaToLinear( tex2D( TexSampler, i.coordTap3 ) ) );

		float3 avgColor = ( s0 + s1 + s2 + s3 ) * 0.25f;
		return float4( avgColor, 1.0f );
	}
#endif
