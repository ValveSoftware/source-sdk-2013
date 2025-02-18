// STATIC: "CONVERT_TO_SRGB"				"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"				"0..0"	[= 0] [XBOX]
// STATIC: "LINEAR_INPUT"					"0..1"  [ps20b]
// STATIC: "LINEAR_OUTPUT"					"0..1"  [ps20b]

// DYNAMIC: "AA_ENABLE"						"0..1"
// rem DYNAMIC: "AA_DEBUG_MODE"				"0..3"
#define AA_DEBUG_MODE						0	
// DYNAMIC: "AA_QUALITY_MODE"				"0..0"  [ps20]
// DYNAMIC: "AA_QUALITY_MODE"				"0..1"  [ps20b]
// DYNAMIC: "AA_QUALITY_MODE"				"0..1"  [ps30]
// DYNAMIC: "AA_REDUCE_ONE_PIXEL_LINE_BLUR"	"0..0"	[ps20]
// DYNAMIC: "AA_REDUCE_ONE_PIXEL_LINE_BLUR"	"0..1"	[ps20b]
// DYNAMIC: "AA_REDUCE_ONE_PIXEL_LINE_BLUR"	"0..1"	[ps30]
// DYNAMIC: "COL_CORRECT_NUM_LOOKUPS"		"0..4"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"


#if !(defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0))
// Only allow debug modes and high-quality mode if in ps2b or higher (not enough instruction slots in ps20)
#undef  AA_DEBUG_MODE
#define AA_DEBUG_MODE					0
#endif


/* 
 * Engine_Post combines bloom (the final simple addition) with software anti-aliasing
 * and colour-correction. Combining them has these benefits:
 *  (a) saves fillrate+bandwidth (big on PC)
 *  (b) saves calls to UpdateScreenEffectTexture (big on 360)
 *  (c) reduces quantization errors caused by multiple passes
 *  (d) improves AA quality (it works better on sRGB values than linear)
 *
 *
 * Software AA Summary
 * -------------------
 *
 * This AA process works by sampling neighbour pixels (4 or 8 of them):
 *
 *   5-tap filter:         #     9-tap filter:        ###
 *   (AA_QUALITY_MODE 0)  ###    (AA_QUALITY_MODE 1)  ###
 *                         #                          ###
 *
 * It then figures out which of these neighbours are 'unlike' the centre pixel.
 * This is based on RGB distance, weighted by the maximum luminance of the samples
 * (so the difference between 0.1 and 0.2 is the same as between 0.5 and 1.0).
 * This detects high-contrast edges in both dark and bright scenes.
 *
 * It then counts how many 'unlike' samples there are. Some example cases for 5-tap:
 *
 *         O      #      #      #      #      #
 *        OOO    OOO    #OO    OOO    #O#    #O#
 *         O      O      O      #      O      #
 *        Zero   One    TwoA   TwoB  Three   Four
 *
 * We then blend towards the average of the unlike neighbours, based on how many
 * unlike neighbours there are. The key case is 'TwoA' - this detects stairstep pixels
 * on non-axis-aligned edges. In that case, we blend the output colour towards the
 * average of the unlike samples by 33%. This yields a 3-pixel transition (0->33->66->100)
 * where before there was a 1-pixel transition (0->100).
 *
 * The 9-tap filter (which works the same as 5-tap, just with more samples and different
 * weights) has two advantages over the 5-tap filter:
 *  - it can differentiate between stairsteps on 45-degree edges and near-horizontal edges
 *    (so the 5-tap version smudges 45-degree edges more than you want, e.g. chain-link fences)
 *  - it blurs less texture detail, by virtue of averaging out noise over more samples
 *
 * One problem case that both filters have to consider is one-pixel-thick lines (this is
 * case 'TwoB' above). Sometimes you do want to soften these lines (for slivers of brightly-lit
 * geometry in a dark area, e.g. a window frame), but sometimes you do NOT want to soften them
 * (for thin geometry which is alternating between 1-pixel and 2-pixel thickness, e.g. cables,
 * and also where 1-pixel lines appear in textures, e.g. roof tiles). So, blurring of 1-pixel
 * lines is tunable (it defaults to half-blurred as a compromise between the want/don't cases),
 * in the 'AA_REDUCE_ONE_PIXEL_LINE_BLUR' section below. Case TwoB is differentiated from TwoA by
 * computing the centroid of the unlike samples (the centroid will be at zero for case TwoB,
 * but not for TwoA).
 *
 */

sampler	BaseTextureSampler				: register( s0 );
sampler	FBTextureSampler				: register( s1 );
sampler	ColorCorrectionVolumeTexture0	: register( s2 );
sampler	ColorCorrectionVolumeTexture1	: register( s3 );
sampler	ColorCorrectionVolumeTexture2	: register( s4 );
sampler	ColorCorrectionVolumeTexture3	: register( s5 );

float4	psTapOffs_Packed				: register( c0 ); // psTapOffs_packed contains 1-pixel offsets: ( +dX, 0, +dY, -dX )
float4	tweakables						: register( c1 ); // (x - AA strength/unused) (y - reduction of 1-pixel-line blur)
														  // (z - edge threshold multipler) (w - tap offset multiplier)
float4	uvTransform						: register( c2 ); // Transform BaseTexture UVs for use with the FBTexture

float	ColorCorrectionDefaultWeight	: register( c3 );
float4	ColorCorrectionVolumeWeights	: register( c4 );
float	BloomFactor						: register( c5 );

float4 GetBloomColor( float2 bloomUV )
{
	#if ( LINEAR_INPUT == 1 )
	{
		// In this case, which is only used on OpenGL, we want sRGB data from this tex2D.
		// Hence, we have to undo the sRGB conversion that we are forced to apply by OpenGL
		return LinearToGamma( tex2D( BaseTextureSampler, bloomUV ) );
	}
	#else
	{
		return tex2D( BaseTextureSampler, bloomUV );
	}
	#endif
}

float4 PerformColorCorrection( float4 outColor, float2 fbTexCoord )
{
	#if ( COL_CORRECT_NUM_LOOKUPS > 0 )
	{
		// NOTE: This code requires the color correction texture to be 32 units to be correct.
		// This code will cause (0,0,0) to be read from 0.5f/32
		// and (1,1,1) to be read from 31.5f/32
		float4 offsetOutColor = outColor*(31.0f/32.0f) + (0.5f/32.0f);

		outColor.rgb  = outColor.rgb * ColorCorrectionDefaultWeight;
		outColor.rgb += tex3D( ColorCorrectionVolumeTexture0, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.x;
		#if ( COL_CORRECT_NUM_LOOKUPS > 1 )
		{
			outColor.rgb += tex3D( ColorCorrectionVolumeTexture1, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.y;
			#if ( COL_CORRECT_NUM_LOOKUPS > 2 )
			{
				outColor.rgb += tex3D( ColorCorrectionVolumeTexture2, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.z;
				#if ( COL_CORRECT_NUM_LOOKUPS > 3 )
				{
					outColor.rgb += tex3D( ColorCorrectionVolumeTexture3, offsetOutColor.rgb ) * ColorCorrectionVolumeWeights.w;
				}
				#endif
			}
			#endif
		}
		#endif
	}
	#endif

	return outColor;
}

float3 PerformAA( float3 baseColor, float2 fbTexCoord, out float3 unlike, out float unlikeSum, out float lerpFactor )
{
	float3  a,  b,  c,  d,  e,  f,  g,  h;
	float3 dA, dB, dC, dD, dE, dF, dG, dH;
	float4 deltas, deltas2;
	float4 weights, weights2;
	float4 lumS;
	float  maxLumS;

	// Set FAST_DELTAS to '1' to use Manhattan distance (in colour-space) rather than Euclidean distance:
	const int	FAST_DELTAS					= 1;
#if AA_QUALITY_MODE == 0
	const float COLOUR_DELTA_BASE			= (FAST_DELTAS == 0) ? 0.11f : 0.5f;
	const float COLOUR_DELTA_CONTRAST		= 100;
	// Scaling down colour deltas (DELTA_SCALE) reduces the over-blurring of 45-degree edges
	// by the 5-tap filter. Conversely, increasing it smooths stairsteps more strongly.
	const float DELTA_SCALE					= 0.75f;
#else // AA_QUALITY_MODE == 0
	const float COLOUR_DELTA_BASE			= (FAST_DELTAS == 0) ? 0.24f : 0.65f;
	const float COLOUR_DELTA_CONTRAST		= 100;
	const float DELTA_SCALE					= 1.0f;
#endif // AA_QUALITY_MODE == 0
	const float MAX_LERP_FACTOR				= 0.66f;
	const float SQRT3						= 1.73205080757f;
	float		onePixelLineBlurReduction	= tweakables.y;


	// psTapOffs_packed contains 1-pixel offsets: ( +dX, 0, +dY, -dX )
	float4 texelDelta = psTapOffs_Packed*tweakables.w;

	// Allowed ps20 swizzles:
	//   .xyzw on (+dX,0,+dY,-dX) gives: (+dX,  0) & (-dX,  0)  (former with 'add', latter with 'sub')
	//   .yzxw on (+dX,0,+dY,-dX) gives: (  0,+dY) & (  0,-dY)
	//   .wzyx on (+dX,0,+dY,-dX) gives: (-dX,+dY) & (+dX,-dY)
	//   .zxyw on (not used)
	// NOTE: These don't give us (+dX,+dY) and (-dX,-dY), we need to copy +dY: ( +dX, 0, +dY, -dX ) -> ( +dX, +dY, +dY, -dX )
	// NOTE: tex2D() can't swizzle the source register in ps2x, so we have no
	//       choice but to add each float2 offset to fbTexCoord one at a time :o/
 	a = tex2D( FBTextureSampler, fbTexCoord + texelDelta.yz ).rgb;	// ( 0,+1)
	b = tex2D( FBTextureSampler, fbTexCoord + texelDelta.xy ).rgb;	// (+1, 0)
	c = tex2D( FBTextureSampler, fbTexCoord - texelDelta.yz ).rgb;	// ( 0,-1)
	d = tex2D( FBTextureSampler, fbTexCoord - texelDelta.xy ).rgb;	// (-1, 0)
#if AA_QUALITY_MODE == 1
	// 9-tap method (do diagonal neighbours too)
	e = tex2D( FBTextureSampler, fbTexCoord + texelDelta.wz ).rgb;	// (-1,+1)
	f = tex2D( FBTextureSampler, fbTexCoord - texelDelta.wz ).rgb;	// (+1,-1)
	texelDelta.y = texelDelta.z; // Can't quite get all 8 sample offsets from a single float4 with the allowed swizzles! :o/
	g = tex2D( FBTextureSampler, fbTexCoord + texelDelta.xy ).rgb;	// (+1,+1)
	h = tex2D( FBTextureSampler, fbTexCoord - texelDelta.xy ).rgb;	// (-1,-1)
#endif // AA_QUALITY_MODE == 1

	// Compute the like<-->unlike weights
	dA = a - baseColor;
	dB = b - baseColor;
	dC = c - baseColor;
	dD = d - baseColor;
#if AA_QUALITY_MODE == 1
	dE = e - baseColor;
	dF = f - baseColor;
	dG = g - baseColor;
	dH = h - baseColor;
#endif // AA_QUALITY_MODE == 1
	#if ( FAST_DELTAS == 0 )
	{
		// Colour-space Euclidean distance
		deltas = float4( dot(dA, dA), dot(dB, dB), dot(dC, dC), dot(dD, dD) );
		deltas = DELTA_SCALE*DELTA_SCALE*(deltas / 3);
		deltas = sqrt(deltas);
	}
	#else
	{
		// Colour-space Manhattan distance
		// OPT: to avoid the 'abs', try dividing colours by maxLumS then dotprodding w/ baseColor
		deltas.x = dot( abs( dA ), 1 );
		deltas.y = dot( abs( dB ), 1 );
		deltas.z = dot( abs( dC ), 1 );
		deltas.w = dot( abs( dD ), 1 );
		deltas  *= DELTA_SCALE;
	}
	#endif

	weights = deltas;
#if AA_QUALITY_MODE == 1
	#if ( FAST_DELTAS == 0 )
	{
		deltas2 = float4( dot(dE, dE), dot(dF, dF), dot(dG, dG), dot(dH, dH) );
		deltas2 = DELTA_SCALE*DELTA_SCALE*(deltas2 / 3);
		deltas2 = sqrt(deltas2);
	}
	#else
	{
		deltas2.x = dot( abs( dE ), 1);
		deltas2.y = dot( abs( dF ), 1);
		deltas2.z = dot( abs( dG ), 1);
		deltas2.w = dot( abs( dH ), 1);
		deltas2  *= DELTA_SCALE;
	}
	#endif

	weights2 = deltas2;
#endif // AA_QUALITY_MODE == 1

	// Adjust weights relative to maximum sample luminance (local, relative contrast: 0.1 Vs 0.2 is the same as 0.5 Vs 1.0)
	lumS	= float4( dot(a, a), dot(b, b), dot(c, c), dot(d, d) );
	lumS.xy	= max( lumS.xy, lumS.wz );
	lumS.x	= max( lumS.x,  lumS.y  );
	maxLumS	= max( lumS.x, dot( baseColor, baseColor ) );
#if AA_QUALITY_MODE == 1
	lumS	= float4( dot(e, e), dot(f, f), dot(g, g), dot(h, h) );
	lumS.xy	= max( lumS.xy, lumS.wz );
	lumS.x	= max( lumS.x,  lumS.y  );
	maxLumS	= max( lumS.x,  maxLumS );
#endif // AA_QUALITY_MODE == 1
	float lumScale	= 1.0f / sqrt( maxLumS );
	weights		   *= lumScale;
#if AA_QUALITY_MODE == 1
	weights2	   *= lumScale;
#endif // AA_QUALITY_MODE == 1

	// Contrast-adjust weights such that only large contrast differences are taken into account
	// (pushes weights to 0.0 for 'like' neighbours and to 1.0 for 'unlike' neighbours)
	float colourDeltaBase = tweakables.z*COLOUR_DELTA_BASE;
	weights		= saturate(colourDeltaBase + COLOUR_DELTA_CONTRAST*(weights - colourDeltaBase));
#if AA_QUALITY_MODE == 1
	weights2	= saturate(colourDeltaBase + COLOUR_DELTA_CONTRAST*(weights2 - colourDeltaBase));
#endif // AA_QUALITY_MODE == 1

	// Determine the average 'unlike' colour
	unlikeSum	= dot(weights, 1);
	unlike		= weights.x*a  + weights.y*b  + weights.z*c  + weights.w*d;
#if AA_QUALITY_MODE == 1
	unlikeSum  += dot(weights2, 1);
	unlike	   += weights2.x*e + weights2.y*f + weights2.z*g + weights2.w*h;
#endif // AA_QUALITY_MODE == 1
	// NOTE: this can cause div-by-zero, but lerpFactor ends up at zero in that case so it doesn't matter
	unlike		= unlike / unlikeSum;


#if AA_REDUCE_ONE_PIXEL_LINE_BLUR
	// Reduce lerpFactor for 1-pixel-thick lines - otherwise you lose texture detail, and it looks
	// really weird where geometry (e.g. cables) alternates between being 1 and 2 pixels thick.
	// [ The "*2" below is because the values here were tuned to reduce blurring one 1-pixel lines
	//   by about half (which is a good compromise between the bad cases at either end). So you
	//   want the controlling convar to default to 0.5 ]
	const float ONE_PIXEL_LINE_BIAS_BASE		= 0.4f;
	const float ONE_PIXEL_LINE_BIAS_CONTRAST	= 16.0f;
	float2 unlikeCentroid = 0;
	unlikeCentroid.x += dot( 1-weights,  float4(  0, +1,  0, -1 ) ); // This 2x4 matrix is the transpose of
	unlikeCentroid.y += dot( 1-weights,  float4( +1,  0, -1,  0 ) ); // the neighbour sample texel offsets
#if AA_QUALITY_MODE == 0
	unlikeCentroid /= 4 - unlikeSum;
#else // AA_QUALITY_MODE == 0
	unlikeCentroid.x += dot( 1-weights2, float4( -1, +1, +1, -1 ) );
	unlikeCentroid.y += dot( 1-weights2, float4( +1, -1, +1, -1 ) );
	unlikeCentroid /= 8 - unlikeSum;
#endif // AA_QUALITY_MODE == 0
	float onePixelLineBias = 1 - saturate( length(unlikeCentroid) ); // OPTIMIZE: try using distSquared, remove this sqrt
	onePixelLineBias = onePixelLineBlurReduction*saturate(ONE_PIXEL_LINE_BIAS_BASE + ONE_PIXEL_LINE_BIAS_CONTRAST*(onePixelLineBias - ONE_PIXEL_LINE_BIAS_BASE));
#if AA_QUALITY_MODE == 0
	unlikeSum -= 2*onePixelLineBias*0.4f*saturate( 3 - unlikeSum ); // The 'min' thing avoids this affecting lone/pair pixels
#else // AA_QUALITY_MODE == 0
	unlikeSum -= 2*onePixelLineBias*1.9f*saturate( 7 - unlikeSum );
#endif // AA_QUALITY_MODE == 0
#endif // AA_REDUCE_ONE_PIXEL_LINE_BLUR


	// Compute the lerp factor we use to blend between 'baseColor' and 'unlike'.
	// We want to lerp 'stairstep' pixels (which have 2 unlike neighbours)
	// 33% towards the 'unlike' colour, such that these hard, 1-pixel transitions
	// (0% -> 100%) become soft, 3-pixel transitions (0% -> 33% -> 66% -> 100%).
	float strengthMultiplier = tweakables.x;
	#if ( AA_QUALITY_MODE == 0 )
	{
		lerpFactor = saturate( strengthMultiplier*DELTA_SCALE*( (unlikeSum - 1) / 3 ) );
		// Uncomment the following to blend slightly across vertical/horizontal edges (better for 45-degree edges, worse for 90-degree edges)
		//lerpFactor = saturate( strengthMultiplier*DELTA_SCALE*( unlikeSum / 6 ) );
	}
	#else // AA_QUALITY_MODE != 0
	{
		lerpFactor = saturate( strengthMultiplier*DELTA_SCALE*( (unlikeSum - 3) / 3 ) );
	}
	#endif

	// Clamp the blend factor so that lone dot pixels aren't blurred into oblivion
	lerpFactor = min( lerpFactor, MAX_LERP_FACTOR );
	baseColor = lerp( baseColor, unlike, lerpFactor );

	return baseColor;
}

float4 GenerateAADebugColor( float4 outColor, float3 unlike, float unlikeSum, float lerpFactor )
{
	#if ( AA_DEBUG_MODE == 1 )
	{
		// Debug: Visualize the number of 'unlike' samples
		outColor.rgb = 0;
		if ( AA_QUALITY_MODE == 0 )
		{
			if (unlikeSum >= 0.95f) outColor.rgb = float3(1,0,0);
			if (unlikeSum >= 1.95f) outColor.rgb = float3(0,1,0);
			if (unlikeSum >= 2.95f) outColor.rgb = float3(0,0,1);
		}
		else
		{
			if (unlikeSum >= 2.95f) outColor.rgb = float3(1,0,0);
			if (unlikeSum >= 3.95f) outColor.rgb = float3(0,1,0);
			if (unlikeSum >= 4.95f) outColor.rgb = float3(0,0,1);
		}
		// Don't sRGB-write
	}
	#elif ( AA_DEBUG_MODE == 2 )
	{
		// Debug: Visualize the strength of lerpFactor
		outColor.rgb = 0;
		outColor.g   = lerpFactor;
		// Don't sRGB-write
	}
	#elif ( AA_DEBUG_MODE == 3 )
	{
		// Debug: Visualize the 'unlike' colour that we blend towards
		outColor.rgb = lerp( 0, unlike, saturate(5*lerpFactor) );
		// Do sRGB-write (if it's enabled)
		outColor = FinalOutput( outColor, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
	#endif

	return outColor;
}

float2 PerformUVTransform( float2 bloomUVs )
{
	// NOTE: 'wz' is used since 'zw' is not a valid swizzle for ps20 shaders
	return bloomUVs*uvTransform.wz + uvTransform.xy;
}

struct PS_INPUT
{
	float2 baseTexCoord : TEXCOORD0;
	
#if defined( _X360 ) //avoid a shader patch on 360 due to pixel shader inputs being fewer than vertex shader outputs
	float2 ZeroTexCoord			: TEXCOORD1;
	float2 bloomTexCoord		: TEXCOORD2;
#endif	
};
	   
float4 main( PS_INPUT i ) : COLOR
{
	float2 fbTexCoord = PerformUVTransform( i.baseTexCoord );
	float3 baseColor  = tex2D( FBTextureSampler, fbTexCoord ).rgb;

	#if ( LINEAR_INPUT == 1 )
	{
		// In this case, which is only used on OpenGL, we want sRGB data from this tex2D.
		// Hence, we have to undo the sRGB conversion that we are forced to apply by OpenGL
		baseColor = LinearToGamma( baseColor );
	}
	#endif

	float4 outColor = float4( baseColor, 1 );

	#if ( AA_ENABLE == 1 )
	{
		float  unlikeSum, lerpFactor;
		float3 unlike;

		outColor.rgb = PerformAA( outColor.rgb, fbTexCoord, unlike, unlikeSum, lerpFactor );

		#if ( AA_DEBUG_MODE > 0 )
		{
			return GenerateAADebugColor( outColor, unlike, unlikeSum, lerpFactor );
		}
		#endif
	}
	#endif

	float4 bloomColor = BloomFactor * GetBloomColor( i.baseTexCoord );
	outColor.rgb += bloomColor.rgb;
	outColor = PerformColorCorrection( outColor, fbTexCoord );
	outColor = FinalOutput( outColor, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );

	// Go to linear since we're forced to do an sRGB write on OpenGL in ps2b
	#if ( LINEAR_OUTPUT == 1 )
	{
		outColor = GammaToLinear( outColor );
	}
	#endif

	return outColor;
}
