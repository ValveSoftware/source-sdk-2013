//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. ===========================

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]

// DYNAMIC: "QUALITY"			"0..3"

#ifdef HDRTYPE
 #undef HDRTYPE
#endif
#define HDRTYPE HDR_TYPE_NONE

// Includes =======================================================================================
#include "common_ps_fxc.h"

// Texture Samplers ===============================================================================
sampler g_tTexSampler : register( s0 );

// Shaders Constants and Globals ==================================================================
float g_flMaxMotionBlur : register( c0 );
float4 g_vConst5 : register( c1 );
#define g_vGlobalBlurVector        g_vConst5.xy
#define g_flFallingMotionIntensity g_vConst5.z
#define g_flRollBlurIntensity      g_vConst5.w

// Interpolated values ============================================================================
struct PS_INPUT
{
	float2 vUv0 : TEXCOORD0;
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	// Calculate blur vector
	float2 vFallingMotionBlurVector = ( ( i.vUv0.xy * 2.0f ) - 1.0f );
	float2 vRollBlurVector = cross( float3( vFallingMotionBlurVector.xy, 0.0f ), float3( 0.0f, 0.0f, 1.0f ) ).xy;
	float2 vGlobalBlurVector = g_vGlobalBlurVector;
	vGlobalBlurVector.y = -vGlobalBlurVector.y;
	//vGlobalBlurVector.xy = float2( 1.0f, 0.0f ); // For debugging

	float flFallingMotionBlurIntensity = -abs( g_flFallingMotionIntensity ); // Keep samples on screen by keeping vector pointing in
	//flFallingMotionBlurIntensity = step( 10, abs(g_flFallingMotionIntensity) ); // For finding the sweet spot in debug mode
	vFallingMotionBlurVector.xy *= dot( vFallingMotionBlurVector.xy, vFallingMotionBlurVector.xy ); // Dampen the effect in the middle of the screen
	vFallingMotionBlurVector.xy *= flFallingMotionBlurIntensity;

	float flRollBlurIntensity = g_flRollBlurIntensity;
	vRollBlurVector.xy *= flRollBlurIntensity;

	float2 vFinalBlurVector = vGlobalBlurVector.xy + vFallingMotionBlurVector.xy + vRollBlurVector.xy;

	// Clamp length of blur vector to unit length
	//vFinalBlurVector.xy = max( -1.0f, min( 1.0f, vFinalBlurVector.xy ) );
	if ( length( vFinalBlurVector.xy ) > g_flMaxMotionBlur )
	{
		vFinalBlurVector.xy = normalize( vFinalBlurVector.xy ) * g_flMaxMotionBlur;
	}

	// Set number of samples
	#if QUALITY == 0
		const int kNumSamples = 1;
	#endif
	#if QUALITY == 1
		const int kNumSamples = 7;
	#endif
	#if QUALITY == 2
		const int kNumSamples = 11;
	#endif
	#if QUALITY == 3
		const int kNumSamples = 15;
	#endif

	float4 cColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	float2 vUvOffset = vFinalBlurVector.xy / ( kNumSamples - 1 );
	for ( int x=0; x<kNumSamples; x++ )
	{
		// Calculate uv
		float2 vUvTmp = i.vUv0.xy + ( vUvOffset.xy * x );

		// Sample pixel
		//cColor += kernel[x] * tex2D( g_tTexSampler, vUvTmp ); // Use kernal from above
		cColor += ( 1.0f / kNumSamples ) * tex2D( g_tTexSampler, vUvTmp ); // Evenly weight all samples
	}

	/*
	// Brute-force experimental code to keep colors in NTSC and PAL gamut, but I don't think this will work correctly.
	//   I think we need to know the final RGB values sent to the TV, which would mean applying the final HW gamma curve first
	//   to each RGB chanel and then just subtracting 191 instead of the funky algorithm here.  Then the results would need to
	//   to be converted back to the 360 gamma PWL space and applied here to cColor.rgb.  Too much effort right now.
	#if QUALITY == 30
		// This washes out the darks...no good
		float flLargest360GammaValue = max( max( cColor.r, cColor.g ), cColor.b );
		float flLargestFinalGamma25Value = pow( SrgbLinearToGamma( X360GammaToLinear( flLargest360GammaValue ) ), ( 2.5f / 2.2f ) ) * ( 219.0f / 255.0f ) + ( 16.0f / 255.0f );
		float flSmallestFinalGamma25ValueAllowed = saturate( flLargestFinalGamma25Value - ( 191.0f / 255.0f ) );
		float flSmallest360GammaValueAllowed = X360LinearToGamma( SrgbGammaToLinear( pow( ( flSmallestFinalGamma25ValueAllowed - ( 16.0f / 255.0f ) ) / ( 219.0f / 255.0f ), ( 2.2f / 2.5f ) ) ) );
		cColor.rgb = max( flSmallest360GammaValueAllowed, cColor.rgb );
	#endif

	#if QUALITY == 3
		// This brings down the saturated colors. I think the 360 hardware is already doing this for us
		float flSmallest360GammaValue = min( min( cColor.r, cColor.g ), cColor.b );
		float flSmallestFinalGamma25Value = pow( SrgbLinearToGamma( X360GammaToLinear( flSmallest360GammaValue ) ), ( 2.5f / 2.2f ) ) * ( 219.0f / 255.0f ) + ( 16.0f / 255.0f );
		float flLargestFinalGamma25ValueAllowed = saturate( flSmallestFinalGamma25Value + ( 191.0f / 255.0f ) );
		float flLargest360GammaValueAllowed = X360LinearToGamma( SrgbGammaToLinear( pow( ( flLargestFinalGamma25ValueAllowed - ( 16.0f / 255.0f ) ) / ( 219.0f / 255.0f ), ( 2.2f / 2.5f ) ) ) );
		cColor.rgb = min( flLargest360GammaValueAllowed, cColor.rgb );
	#endif
	//*/

	//return float4( cColor.rgb, 1.0f );
	return FinalOutput( float4( cColor.rgb, 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );

	// This is histogram testing code that I need access to for a while on other machines to tweak the 360
	/*
	if ( 1 )
	{
		float4 cColor = { 0.0f, 0.0f, 0.0f, 0.0f };

		float2 uv = ( i.vUv0.xy * 1.2f - 0.1 );
		if ( ( uv.x < 0.0f ) || ( uv.x > 1.0f ) || ( uv.y < 0.0f ) || ( uv.y > 1.0f ) )
		{
			cColor.rgb = float3( 1.0f, 0.0f, 0.0f ) * ( 1 - abs( uv.x ) );
		}
		else
		{
			cColor.rgb = uv.x;
			//cColor = tex2D( g_tTexSampler, uv.xy );

			// Simulate 360 sRGB read
			//float3 v360Linear = { X360GammaToLinear( cColor.r ), X360GammaToLinear( cColor.g ), X360GammaToLinear( cColor.b ) };
			//cColor.rgb = v360Linear.rgb;

			// On the PC, simulate the remapping for the 360


		}

		// Blue ruler
		if ( ( uv.y <= 1.0f ) && ( uv.x >= 0.0f ) && ( uv.x <= 1.0f ) )
		{
			if ( uv.y > 0.9f )
			{
				if ( frac( uv.x * 10.0f ) < 0.01f )
				{
					cColor.rgb = float3( 0.0f, 0.0f, 1.0f );
				}
			}

			if ( uv.y > 0.925f )
			{
				if ( frac( uv.x * 20.0f ) < 0.02f )
				{
					cColor.rgb = float3( 0.0f, 0.0f, 1.0f );
				}
			}

			if ( uv.y > 0.95f )
			{
				if ( frac( uv.x * 100.0f ) < 0.1f )
				{
					cColor.rgb = float3( 0.0f, 0.0f, 1.0f );
				}
			}
		}

		//if ( ( uv.x >= 0.0f ) && ( uv.x <= 1.0f ) && ( uv.y >= 0.0f ) && ( uv.y <= 1.0f ) )
		//{
		//	cColor = tex2D( g_tTexSampler, uv.xy );
		//}

		float3 vShaderColor = cColor.rgb;
		float3 v360Linear = { SrgbGammaToLinear( vShaderColor.r ), SrgbGammaToLinear( vShaderColor.g ), SrgbGammaToLinear( vShaderColor.b ) };
		cColor.rgb = v360Linear.rgb;
		//float3 v360Gamma = { X360LinearToGamma( v360Linear.r ), X360LinearToGamma( v360Linear.g ), X360LinearToGamma( v360Linear.b ) };
		//cColor.rgb = v360Gamma.rgb;


		//float3 vGamma = { SrgbLinearToGamma( vShaderColor.r ), SrgbLinearToGamma( vShaderColor.g ), SrgbLinearToGamma( vShaderColor.b ) };
		//float3 v360Linear = { X360GammaToLinear( vShaderColor.r ), X360GammaToLinear( vShaderColor.g ), X360GammaToLinear( vShaderColor.b ) };
		//cColor.rgb = v360Linear.rgb;


		// Simulate 360 sRGB write
		//float3 v360Gamma = { X360LinearToGamma( vShaderColor.r ), X360LinearToGamma( vShaderColor.g ), X360LinearToGamma( vShaderColor.b ) };
		//cColor.rgb = v360Gamma.rgb;

		return cColor;
	}
	//*/
}
