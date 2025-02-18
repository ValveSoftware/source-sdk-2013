// STATIC: "CONVERT_TO_SRGB"		"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"		"0..0"	[= 0] [XBOX]
// STATIC: "DUALSEQUENCE"			"0..1"
// STATIC: "SEQUENCE_BLEND_MODE"	"0..2"
// STATIC: "ADDBASETEXTURE2"		"0..1"
// STATIC: "MAXLUMFRAMEBLEND1"		"0..1"
// STATIC: "MAXLUMFRAMEBLEND2"		"0..1"
// STATIC: "EXTRACTGREENALPHA"		"0..1"
// STATIC: "COLORRAMP"				"0..1"
// STATIC: "ANIMBLEND"				"0..1"
// STATIC: "ADDSELF"				"0..1"
// STATIC: "DEPTHBLEND"				"0..1"	[ps20b]

#define COMBINE_MODE_AVERAGE							0
#define COMBINE_MODE_USE_FIRST_AS_ALPHA_MASK_ON_SECOND	1
#define COMBINE_MODE_USE_FIRST_OVER_SECOND				2

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

const float4 g_Parameters	: register( c0 );
const float4 g_DepthFeatheringConstants : register( c2 );

#define fAdditiveBlendWeight		g_Parameters.x
#define fOverbrightFactor			g_Parameters.y
#define fAdditiveSelfBlendWeight	g_Parameters.z

struct PS_INPUT
{
	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD1;
	float4 argbcolor		: COLOR;
	float4 blendfactor0		: TEXCOORD2;
#if ADDBASETEXTURE2
	float2 texCoord2		: TEXCOORD3;
#endif
#if	EXTRACTGREENALPHA
	float4 blendfactor1		: TEXCOORD4;
#endif

#if DUALSEQUENCE
	float2 vSeq2TexCoord0   : TEXCOORD5;
	float2 vSeq2TexCoord1   : TEXCOORD6; 
#endif
	
#if defined( REVERSE_DEPTH_ON_X360 )
	float4 vScreenPos_ReverseZ		: TEXCOORD7;
#else
	float4 vScreenPos		: TEXCOORD7;
#endif
};

sampler BaseTextureSampler	: register( s0 );
sampler ColorRampSampler	: register( s1 );
sampler DepthSampler		: register( s2 );

float4 main( PS_INPUT i ) : COLOR
{
	bool bMaxLumFrameBlend1 = MAXLUMFRAMEBLEND1 ? true : false;
	bool bMaxLumFrameBlend2 = MAXLUMFRAMEBLEND2 ? true : false;
	bool bExtractGreenAlpha = EXTRACTGREENALPHA ? true : false;
	bool bAddBaseTexture2   = ADDBASETEXTURE2   ? true : false;
	bool bDualSequence      = DUALSEQUENCE      ? true : false;
	bool bColorRamp         = COLORRAMP         ? true : false;
#ifdef DEPTHBLEND
	bool bDepthBlend		= DEPTHBLEND		? true : false;
#endif
	int nSequenceBlendMode  = SEQUENCE_BLEND_MODE;

	// Sample frames from texture 0
	float4 baseTex0 = tex2D( BaseTextureSampler, i.texCoord0 );

	float4 baseTex1 = tex2D( BaseTextureSampler, i.texCoord1 );

	// Blend by default (may override with bMaxLumFrameBlend1 or bExtractGreenAlpha)
#if ANIMBLEND
	float4 blended_rgb = lerp( baseTex0, baseTex1, i.blendfactor0.x );
#else
	float4 blended_rgb = baseTex0;
#endif

	if ( bMaxLumFrameBlend1 )
	{
		// Blend between animation frames based upon max luminance
		float lum0 = dot( float3(.3, .59, .11), baseTex0.rgb * (1-i.blendfactor0.x));
		float lum1 = dot( float3(.3, .59, .11), baseTex1.rgb *    i.blendfactor0.x);

		if ( lum0 > lum1 )
			blended_rgb = baseTex0;
		else
			blended_rgb = baseTex1;
	}
	else if( bExtractGreenAlpha )
	{
#if EXTRACTGREENALPHA
		// Weight Green/Alphas from the two frames for a scalar result
		blended_rgb = dot( baseTex0, i.blendfactor0 ) + dot( baseTex1, i.blendfactor1 );
#endif
	}

#if DUALSEQUENCE
	if ( bDualSequence )
	{
		baseTex0 = tex2D( BaseTextureSampler, i.vSeq2TexCoord0 );
		baseTex1 = tex2D( BaseTextureSampler, i.vSeq2TexCoord1 );

		// Blend by default (may override with bMaxLumFrameBlend2)
		float4 rgb2 = lerp( baseTex0, baseTex1, i.blendfactor0.z );

		if ( bMaxLumFrameBlend2 )
		{
			// blend between animation frames based upon max luminance
			float tlum0 = dot( float3(.3, .59, .11), baseTex0.rgb * (1-i.blendfactor0.x));
			float tlum1 = dot( float3(.3, .59, .11), baseTex1.rgb *    i.blendfactor0.x);

			if ( tlum0 > tlum1 )
				rgb2 = baseTex0;
			else
				rgb2 = baseTex1;
		}

		if ( nSequenceBlendMode == COMBINE_MODE_AVERAGE )
		{
			blended_rgb = 0.5 * ( blended_rgb + rgb2 );
		}
		else if ( nSequenceBlendMode == COMBINE_MODE_USE_FIRST_AS_ALPHA_MASK_ON_SECOND )
		{
			blended_rgb.rgb = rgb2.rgb;
		}
		else if ( nSequenceBlendMode == COMBINE_MODE_USE_FIRST_OVER_SECOND )
		{
			blended_rgb.rgb = lerp( blended_rgb.rgb, rgb2.rgb, rgb2.a );
		}
	} // bDualSequence
#endif

	// Optional color ramp
	if ( bColorRamp )
	{
		blended_rgb.rgb = tex2D( ColorRampSampler, float2( blended_rgb.r, blended_rgb.g ) );
	}

	// Overbright
	blended_rgb.rgb *= fOverbrightFactor;

	//Soft Particles FTW
#	if (DEPTHBLEND == 1)
#		if defined( _X360 )
			float fDepthBlend = DepthFeathering( DepthSampler, i.vScreenPos_ReverseZ.xy / i.vScreenPos_ReverseZ.w, i.vScreenPos_ReverseZ.z, i.vScreenPos_ReverseZ.w, g_DepthFeatheringConstants );
#		else
			float fDepthBlend = DepthFeathering( DepthSampler, i.vScreenPos.xy / i.vScreenPos.w, i.vScreenPos.z, i.vScreenPos.w, g_DepthFeatheringConstants );
#		endif
		i.argbcolor.a *= fDepthBlend;
#	endif

	// Premultiply the alpha for a ONE:INVALPHA blend
#if ADDBASETEXTURE2
	if ( bAddBaseTexture2 )
	{
		blended_rgb.a *= i.argbcolor.a;

		// In this case, we don't really want to pre-multiply by alpha
		if ( !bColorRamp )
		{
			blended_rgb.rgb *= blended_rgb.a;
		}

		if ( bExtractGreenAlpha )
		{
			blended_rgb.rgb += fAdditiveBlendWeight * i.argbcolor.a * blended_rgb.rgb;
		}
		else
		{
			blended_rgb.rgb += fOverbrightFactor * fAdditiveBlendWeight * i.argbcolor.a * tex2D( BaseTextureSampler, i.texCoord2 );
		}

		blended_rgb.rgb *= i.argbcolor.rgb;
	}
	else
#endif
	{
#if ADDSELF
		blended_rgb.a *= i.argbcolor.a;
		blended_rgb.rgb *= blended_rgb.a;
		blended_rgb.rgb += fOverbrightFactor * fAdditiveSelfBlendWeight * i.argbcolor.a * blended_rgb;
		blended_rgb.rgb *= i.argbcolor.rgb;
#else
		blended_rgb *= i.argbcolor;
#endif
	}

	return FinalOutput( blended_rgb, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR );
}

