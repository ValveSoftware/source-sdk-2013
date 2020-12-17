//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

// FIXMEL4DTOMAINMERGE
// Need to re-enable bloom and disable other L4D-only features in here and the cpp file.

// STATIC: "TOOL_MODE" "0..1"
// STATIC: "DEPTH_BLUR_ENABLE" "0..1"

// DYNAMIC: "AA_ENABLE"						"0..0" [PC]
// DYNAMIC: "AA_ENABLE"						"0..1" [XBOX]

// DYNAMIC: "COL_CORRECT_NUM_LOOKUPS"		"0..3"

// DYNAMIC: "CONVERT_FROM_LINEAR"			"0..1"	[ps20b] [ps30] [PC]
// DYNAMIC: "CONVERT_TO_LINEAR"				"0..1"	[ps20b] [ps30] [PC]
// DYNAMIC: "CONVERT_FROM_LINEAR"			"0..0"	[ps20b] [XBOX]
// DYNAMIC: "CONVERT_TO_LINEAR"				"0..0"	[ps20b] [XBOX]
// SKIP: ( $CONVERT_FROM_LINEAR == 0 ) && ( $CONVERT_TO_LINEAR == 1 )
// SKIP: ( $TOOL_MODE == 0 ) && ( $CONVERT_TO_LINEAR == 1 )

// DYNAMIC: "NOISE_ENABLE"					"0..1"	[ps20b] [ps30]
// DYNAMIC: "VIGNETTE_ENABLE"				"0..1"	[ps20b] [ps30]
// DYNAMIC: "LOCAL_CONTRAST_ENABLE"			"0..1"	[ps20b] [ps30]
// DYNAMIC: "BLURRED_VIGNETTE_ENABLE"		"0..1"	[ps20b] [ps30]
// DYNAMIC: "VOMIT_ENABLE"					"0..1"
// SKIP: ( $LOCAL_CONTRAST_ENABLE == 0 ) && ( $BLURRED_VIGNETTE_ENABLE == 1 )

// DYNAMIC: "TV_GAMMA"						"0..1"	[ps20b] [PC]
// DYNAMIC: "DESATURATEENABLE"				"0..1"	[ps20b] [PC]
// SKIP: ( $TOOL_MODE == 0 ) && $TV_GAMMA
// SKIP: ( $TOOL_MODE == 0 ) && $DESATURATEENABLE

#include "common_ps_fxc.h"

sampler	BaseTextureSampler				: register( s0 );
sampler	FBTextureSampler				: register( s1 );
sampler	ColorCorrectionVolumeTexture0	: register( s2 );
sampler	ColorCorrectionVolumeTexture1	: register( s3 );
sampler	ColorCorrectionVolumeTexture2	: register( s4 );
sampler	ColorCorrectionVolumeTexture3	: register( s5 );
sampler	NoiseSampler					: register( s6 );
sampler	VignetteSampler					: register( s7 );
sampler ScreenEffectSampler				: register( s8 );	// used for vomit/paint screen particle effects

float4	psTapOffs_Packed				: register( c0 ); // psTapOffs_packed contains 1-pixel offsets: ( +dX, 0, +dY, -dX )
float4	tweakables						: register( c1 ); // (x - AA strength/unused) (y - reduction of 1-pixel-line blur)
														  // (z - edge threshold multipler) (w - tap offset multiplier)
float4	uvTransform						: register( c2 ); // Transform BaseTexture UVs for use with the FBTexture
								 
float	ColorCorrectionDefaultWeight	: register( c3 );
float4	ColorCorrectionVolumeWeights	: register( c4 );

// Bloom & Depth Blur parameters
// x: bloom amount; multiply bloom downscale buffer by this value and add to base color
// y: bloom lerp amount; lerp between base color and blurred bloom buffer with this factor (allows for color bleeding in dark areas)
// z: depth blur focal plane distance.  Value is in dest alpha space [0,1], not world units.
// w: depth blur scale value; scale distance from focal plane by this amount
float4	BloomParameters					: register( c5 );
#define g_flBloomAmount ( BloomParameters.x )
#define g_flBloomLerpFactor ( BloomParameters.y )
#define g_flDepthBlurFocalDistance ( BloomParameters.z )
#define g_flDepthBlurScale ( BloomParameters.w )

float	g_flNoiseScalar					: register( c6 );
float	g_flTime						: register( c7 );
float4	g_vLocalContrastParams			: register( c8 );
#define g_flLocalContrastStrength		g_vLocalContrastParams.x
#define g_flLocalContrastMidToneMask	g_vLocalContrastParams.y
#define g_flBlurredVignetteStrength		g_vLocalContrastParams.z

float4	g_vLocalContrastVignetteParams	: register( c9 );
#define g_flLocalContrastVignetteStart	g_vLocalContrastVignetteParams.x
#define g_flLocalContrastVignetteEnd	g_vLocalContrastVignetteParams.y
#define g_flLocalContrastEdgeStrength	g_vLocalContrastVignetteParams.z

float	g_flFadeToBlackStrength			: register( c10 );

float4	g_vVomitColor[2]				: register( c11 );
#define g_flVomitRefractStrength		g_vVomitColor[0].a

float4	g_vViewportTransform			: register( c13 );
float4	g_vInvViewportTransform			: register( c14 );

float4  g_vViewFadeColor				: register( c15 );

float2  g_c16							: register( c16 );
#define g_flDesaturation g_c16.x
#define g_flFadeMode2    g_c16.y

float Luminance( float3 cColor )
{
	float3 tmpv = { 0.2125, 0.7154, 0.0721 };
	float flLuminance = dot( cColor.rgb, tmpv.rgb );
	return flLuminance;
}

float4 GetBloomColor( float2 bloomUV )
{
	return tex2D( BaseTextureSampler, bloomUV );
}

float4 PerformColorCorrection( float4 outColor )
{
	if (COL_CORRECT_NUM_LOOKUPS > 0)
	{
		// NOTE: This code requires the color correction texture to be 32 units to be correct.
		// This code will cause (0,0,0) to be read from 0.5f/32
		// and (1,1,1) to be read from 31.5f/32
		float4 offsetOutColor = outColor*(31.0f/32.0f) + (0.5f/32.0f);

		outColor.rgb  = outColor.rgb * ColorCorrectionDefaultWeight;
		outColor.rgb += tex3D( ColorCorrectionVolumeTexture0, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.x;
		if (COL_CORRECT_NUM_LOOKUPS > 1)
		{
			outColor.rgb += tex3D( ColorCorrectionVolumeTexture1, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.y;
			if (COL_CORRECT_NUM_LOOKUPS > 2)
			{
				outColor.rgb += tex3D( ColorCorrectionVolumeTexture2, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.z;
				if (COL_CORRECT_NUM_LOOKUPS > 3)
				{
					outColor.rgb += tex3D( ColorCorrectionVolumeTexture3, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.w;
				}
			}
		}
	}

	return outColor;
}

float3 PerformVomitBlend( float3 vRefractParams, float3 vFullResColor, float3 vBlurredColor )
{
	float3 vVomitColor = lerp( g_vVomitColor[0].rgb, g_vVomitColor[1].rgb, vRefractParams.z );	// vomit tint
	
	vFullResColor.rgb *= lerp( float3( 1, 1, 1 ), vVomitColor, vRefractParams.z );	// vomit tint full-res buffer
	vFullResColor.rgb = lerp ( vFullResColor.rgb, vVomitColor.rgb * vBlurredColor.rgb, vRefractParams.z );
	return vFullResColor.rgb;
}

float2 PerformUVTransform( float2 bloomUVs )
{
	// NOTE: 'wz' is used since 'zw' is not a valid swizzle for ps20 shaders
	return bloomUVs*uvTransform.wz + uvTransform.xy;
}

// Apply TV Gamma for movie layoff specific to 360 TV movie playback path
float3 SrgbGammaToTvGamma( float3 cInput )
{
	float3 cLinear = SrgbGammaToLinear( cInput );
	return pow( cLinear, 1.0f / 2.5f );
}


struct PS_INPUT
{
	float2 baseTexCoord : TEXCOORD0;
};
	   
float4 main( PS_INPUT i ) : COLOR
{
	float4 fbTexCoord = 0;
	#if !defined( SHADER_MODEL_PS_2_0 )
	{
		fbTexCoord.xy = PerformUVTransform( i.baseTexCoord );
		fbTexCoord.zw = i.baseTexCoord;
	}
	#else
	{
		fbTexCoord.xy = PerformUVTransform( i.baseTexCoord );
	}
	#endif

	float4 cBloomBlurredLum = GetBloomColor( i.baseTexCoord );	// bloom color and blurred luminance in alpha
	float4 vVomitRefractParams;
	#if ( VOMIT_ENABLE == 1 )
	{
		// perturb texture coordinate
		vVomitRefractParams = tex2D( ScreenEffectSampler, i.baseTexCoord );
		fbTexCoord = fbTexCoord + g_flVomitRefractStrength * ( vVomitRefractParams.xyxy - 0.5 );

		#if !defined( SHADER_MODEL_PS_2_0 )
		{
			// screen coords -> viewport coords
			float4 vNormalizedTexCoord = g_vViewportTransform.xyxy * fbTexCoord + g_vViewportTransform.zwzw;
			// mirrored repeat texcoord math doesn't fit into 2.0
			vNormalizedTexCoord = min( 2.0 - vNormalizedTexCoord, abs( vNormalizedTexCoord ) );
			// viewport coords -> screen coords
			fbTexCoord = g_vInvViewportTransform.xyxy * vNormalizedTexCoord + g_vInvViewportTransform.zwzw;

			cBloomBlurredLum = GetBloomColor( fbTexCoord.zw );	// fetch again with perturbed texcoords
		}
		#else
		{
			cBloomBlurredLum = GetBloomColor( fbTexCoord.xy );	// fetch again with perturbed texcoords
		}
		#endif
	}
	#endif

	float4 rawColor = tex2D( FBTextureSampler, fbTexCoord.xy ).rgba;
	float3 baseColor  = rawColor.rgb;
	float depthValue = rawColor.a;
	
	#if ( CONVERT_FROM_LINEAR == 1 )
	{
		baseColor.rgb = SrgbLinearToGamma( baseColor.rgb );
	}
	#endif

	float4 outColor = float4( baseColor, 1 );

	#if ( AA_ENABLE == 1 )
	{
		float3 up;
		float3 dn;
		float3 lf;
		float3 rt;
		float3 uplf;
		float3 uprt;
		float3 dnlf;
		float3 dnrt;

		// psTapOffs_packed contains 1-pixel offsets: ( +dX, 0, +dY, -dX )
		float4 texelDelta = psTapOffs_Packed.xyzw;
		dn = tex2D( FBTextureSampler, fbTexCoord.xy + texelDelta.yz ).rgb; // ( 0,+1)
		rt = tex2D( FBTextureSampler, fbTexCoord.xy + texelDelta.xy ).rgb; // (+1, 0)
		up = tex2D( FBTextureSampler, fbTexCoord.xy - texelDelta.yz ).rgb; // ( 0,-1)
		lf = tex2D( FBTextureSampler, fbTexCoord.xy - texelDelta.xy ).rgb; // (-1, 0)
		dnlf = tex2D( FBTextureSampler, fbTexCoord.xy + texelDelta.wz ).rgb; // (-1,+1)
		uprt = tex2D( FBTextureSampler, fbTexCoord.xy - texelDelta.wz ).rgb; // (+1,-1)
		texelDelta.y = texelDelta.z; // Can't quite get all 8 sample offsets from a single float4 with the allowed swizzles!
		uplf = tex2D( FBTextureSampler, fbTexCoord.xy + texelDelta.xy ).rgb; // (+1,+1)
		dnrt = tex2D( FBTextureSampler, fbTexCoord.xy - texelDelta.xy ).rgb; // (-1,-1)

		// Generate the edge mask
		float flBaseLum = Luminance( baseColor.rgb );
		float flEdge = saturate( abs( Luminance( dn.rgb ) - flBaseLum ) - 0.1 );
		flEdge += saturate( abs( Luminance( up.rgb ) - flBaseLum ) - 0.1 );
		flEdge += saturate( abs( Luminance( lf.rgb ) - flBaseLum ) - 0.1 );
		flEdge += saturate( abs( Luminance( rt.rgb ) - flBaseLum ) - 0.1 );
		flEdge *= 5.0;

		// Average full 3x3 neighborhood of pixels giving more weight to the center sample
		float3 vBlurColor = ( baseColor.rgb * 4.0f ) + up.rgb + dn.rgb + lf.rgb + rt.rgb + dnrt.rgb + uprt.rgb + dnlf.rgb + uplf.rgb;
		vBlurColor.rgb *= 0.0833333; // 1.0 / 12.0

		// Lerp between crisp and blurry pixel based on edge mask
		outColor.rgb = lerp( baseColor.rgb, vBlurColor.rgb, saturate( flEdge ) );
	}
	#endif

	#if ( VOMIT_ENABLE == 1 )
	{
		outColor.rgb = PerformVomitBlend( vVomitRefractParams.xyz, outColor.rgb, cBloomBlurredLum.aaa );
	}
	#endif

	#if ( LOCAL_CONTRAST_ENABLE == 1 )
	{
		float fMask = 1.0;

		// Extract midtones and limit contrast enhancement there
		// TODO: This can probably go away for perf.
		//float fBrightness = dot( outColor.rgb, float3( 0.3, 0.59, 0.11 ) );
		// bell-shaped mask
		//fMask = smoothstep( 0.5 - g_flLocalContrastMidToneMask, 0.5, fBrightness );
		//fMask *= smoothstep( 0.5 + g_flLocalContrastMidToneMask, 0.5, fBrightness );

		//fMask = smoothstep( 1.0, 0.5, fBrightness );
		
		/*
		// unsharp mask on luminance only
		// This is the technically correct way, going to YUV, applying contrast to Y, and converting back to RGB
		float3 outColorYUV;
		outColorYUV.x = dot( outColor.rgb, float3( 0.299, 0.587, 0.114 ) );
		outColorYUV.y = dot( outColor.rgb, float3( -0.14713, -0.28886, 0.436 ) );
		outColorYUV.z = dot( outColor.rgb, float3( 0.615, -0.51499, -0.10001 ) );
		outColorYUV.x = outColorYUV.x + g_flLocalContrastStrength * fMask * ( outColorYUV.x - cBloomBlurredLum.aaa );
		outColor.r = dot( outColorYUV.xyz, float3( 1.0, 0.0, 1.13983 ) );
		outColor.g = dot( outColorYUV.xyz, float3( 1.0, -0.39465, -0.58060 ) );
		outColor.b = dot( outColorYUV.xyz, float3( 1.0, 2.03211, 0.0 ) );
		*/

		// This applies the delta contrast derived from the luminance to all color channels. The difference to the
		// correct way is imperceptible.
		float fLuminance = dot( outColor.rgb, float3( 0.299, 0.587, 0.114 ) );
		float fContrastLum = fLuminance + g_flLocalContrastStrength * ( fLuminance - cBloomBlurredLum.a );
		
		// Mask off pixels that got very bright, to control super-contrast 
		//fMask = 1.0 - smoothstep( 0.3, 1.0, fContrastLum );

		float2 vCenterDir = ( 2.0 * i.baseTexCoord.xy ) - 1.0;
		float fMyVignette = smoothstep( g_flLocalContrastVignetteStart, g_flLocalContrastVignetteEnd, length( vCenterDir ) );
		float fMyVignette2 = fMyVignette;
		fMyVignette = lerp( g_flLocalContrastStrength, g_flLocalContrastEdgeStrength, fMyVignette );

		fMask = fMyVignette;

		// If the mask is positive, only brighten pixels. If the mask is negative, don't let it get less than -1.0.
		//outColor.rgb += fMask * ( fLuminance - cBloomBlurredLum.aaa );
		outColor.rgb += max( fMask * ( fLuminance - cBloomBlurredLum.aaa ), -1.0 + step( 0.0, fMask ) ); // Selective clamp to positive adds 4 instructions

		#if ( BLURRED_VIGNETTE_ENABLE == 1 )
			outColor.rgb = lerp( outColor.rgb, cBloomBlurredLum.aaa, fMyVignette2 * g_flBlurredVignetteStrength );
		#endif
	}
	#endif

	// Composite bloom and full-screen + depth blur effects
	#if ( DEPTH_BLUR_ENABLE )
	{
		float blurFactor = g_flBloomLerpFactor + abs( depthValue - g_flDepthBlurFocalDistance ) * g_flDepthBlurScale;
		blurFactor = clamp( blurFactor, 0, 1 );
		outColor.rgb = lerp( outColor.rgb, cBloomBlurredLum.rgb, blurFactor );
		outColor.rgb += g_flBloomAmount * cBloomBlurredLum.rgb;
	}
	#else
	{
		outColor.rgb += g_flBloomAmount * cBloomBlurredLum.rgb;
	}
	#endif

	// Used to be FADE_TYPE 0..2 combo
	float3 vFadeDestColor = lerp( g_vViewFadeColor.rgb, g_vViewFadeColor.rgb * outColor.rgb, g_flFadeMode2 );
	outColor.rgb = lerp( outColor.rgb, vFadeDestColor.rgb, g_vViewFadeColor.a );

	#if ( DESATURATEENABLE )
	{
		float flLum = saturate( dot( outColor.rgb, float3( 0.3f, 0.59f, 0.11f) ) );
		outColor.rgb = lerp( saturate( outColor.rgb ), flLum.xxx, saturate( g_flDesaturation ) );
	}
	#else
	{
		outColor = PerformColorCorrection( outColor );	// Color correction
	}
	#endif

	// Vignette
	#if ( VIGNETTE_ENABLE == 1 )
	{
		// Vignette
		float2 vUv = i.baseTexCoord.xy;
		//float2 vTmp = ( vUv.xy * 2.0 ) - 1.0;
		float flVignette;

		//flVignette = 1.0 - pow( abs( vTmp.x ), 6.0f );
		//flVignette *= 1.0 - pow( abs( vTmp.y ), 6.0f );
		//flVignette = 1.0 - ( 1.0 - flVignette ) * ( ( saturate( ( 1.0 - vUv.y ) - 0.3 ) / 0.7 ) );

		// This tex2D solves the 3 lines of math above
		flVignette = tex2D( VignetteSampler, vUv.xy ).r; // Red is for the PC
		flVignette = saturate( flVignette * 0.55 + 0.46 );

		outColor.rgb *= flVignette;
	}
	#endif

	// Noise
	#if ( NOISE_ENABLE == 1 )
	{
		// Additive Noise
		float2 vUv0 = i.baseTexCoord.xy * 10.0 + g_flTime;
		float2 vUv1 = i.baseTexCoord.yx * 20.0 - g_flTime;
		float2 vNoiseTexelUv;
		vNoiseTexelUv.x = tex2D( NoiseSampler, vUv0.xy ).g;
		vNoiseTexelUv.y = tex2D( NoiseSampler, vUv1.xy ).g;
		float flNoiseTexel = tex2D( NoiseSampler, vNoiseTexelUv.xy ).g;

		float3 vTmp = { 0.2125f, 0.7154f, 0.0721f };
		float flLuminance = saturate( dot( outColor.rgb, vTmp.rgb ) );

		float flNoiseScalar = 0.2f + 0.8f * ( saturate( pow( 1.0 - flLuminance, 12.0 ) ) );
		outColor.rgb += ( ( flNoiseTexel * 0.3f ) - 0.15f  ) * g_flNoiseScalar * flNoiseScalar;
	}
	#endif

	// Fade to black
	outColor.rgb = lerp( outColor.rgb, 0.0, g_flFadeToBlackStrength );

	#if TV_GAMMA
	{
		// Used for SFM to record movies in native TV gamma space
		outColor.rgb = SrgbGammaToTvGamma( outColor.rgb );
	}
	#endif

	#if ( CONVERT_TO_LINEAR == 1 )
	{
		// If we have a float back buffer, we want to remain in linear space after this shader
		outColor.rgb = SrgbGammaToLinear( outColor.rgb );
	}
	#endif

	return FinalOutput( outColor, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
