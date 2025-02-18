//======= Copyright © 1996-2015, Valve Corporation, All rights reserved. ======
// STATIC: "COMBINE_MODE"			"0..6"  // See below for meanings of combine modes.
// DYNAMIC: "DEBUG_MODE"					"0..0"	[ps20]
// DYNAMIC: "DEBUG_MODE"					"0..1"	[ps20b] [ps30]

// Lerps are implemented by a multiply / blend / multiply inverse on ps20
// SKIP: ( $COMBINE_MODE == 2 )		[ps20]
// SKIP: ( $COMBINE_MODE == 4 || $COMBINE_MODE == 5 )		[ps20b]
// SKIP: ( $COMBINE_MODE == 4 || $COMBINE_MODE == 5 )		[ps30]
// SKIP: ( $COMBINE_MODE != 6 && $DEBUG_MODE == 1 )

#include "common_ps_fxc.h"

struct PS_INPUT
{
	float4 texCoord01 : TEXCOORD0;
	float4 texCoord23 : TEXCOORD1;
};

#define COMBINE_MODE_MULTIPLY			0
#define COMBINE_MODE_ADD				1
#define COMBINE_MODE_LERP				2
#define COMBINE_MODE_SELECTOR			3
#define COMBINE_MODE_LERP_TEX_FIRST		4
#define COMBINE_MODE_LERP_TEX_SECOND	5
#define COMBINE_MODE_BLEND				6

#if ( !defined( SHADER_MODEL_PS_2_0 ) )
	#define SKIP_SRGB_ENC_DEC 0
	#define ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE 1
#else
	#define SKIP_SRGB_ENC_DEC 1
	#define ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE 0
#endif

const float4	cAdjustInLevel[4]	: register( c2 );
const int		cNumTextures		: register( c6 );
const float4	cSelectValues[4]	: register( c7 );

sampler InSampler0					: register( s0 );
sampler InSampler1					: register( s1 );
#if ( ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE )
	sampler InSampler2					: register( s2 );
	sampler InSampler3					: register( s3 );
#endif

#define texCoord0	texCoord01.xy
#define texCoord1	texCoord01.zw
#if ( ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE )
	#define texCoord2	texCoord23.xy
	#define texCoord3	texCoord23.zw
#endif

#define g_AdjustInBlack(n)		cAdjustInLevel[n].x
#define g_AdjustInWhite(n)		cAdjustInLevel[n].y
#define g_AdjustGamma(n)		cAdjustInLevel[n].z

static const float4	cErrColor = float4( 0.0, 1.0, 0.0, 1.0 );

#if ( COMBINE_MODE == COMBINE_MODE_MULTIPLY )
	static const float4	cSafeColor = float4( 1.0, 1.0, 1.0, 1.0 );
#elif ( COMBINE_MODE == COMBINE_MODE_ADD ) 
	static const float4	cSafeColor = float4( 0.0, 0.0, 0.0, 0.0 );
#elif ( COMBINE_MODE == COMBINE_MODE_LERP )
	static const float4	cSafeColor = float4( 1.0, 1.0, 1.0, 1.0 );
#elif ( COMBINE_MODE == COMBINE_MODE_SELECTOR )
	static const float4	cSafeColor = float4( 0.0, 0.0, 0.0, 0.0 );
#elif ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_FIRST ) || ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_SECOND ) 
#elif ( COMBINE_MODE == COMBINE_MODE_BLEND )
	static const float4	cSafeColor = float4( 0.0, 0.0, 0.0, 0.0 );
#else
	#error "Need to add mode selection here."
#endif

float invlerp( float x, float y, float r )
{
    return ( r - x ) / ( y - x );
}

float4 invlerp( float x, float y, float4 r )
{
    return ( r - x ) / ( y - x );
}

float4 ConvertLinearTosRGB( float4 lin )
{
	#if ( SKIP_SRGB_ENC_DEC )
		// If we're in ps 2.0, we don't have the instruction slots to do this correctly
		return lin;
	#else
		float3 col_lin = lin.xyz;
		float3 col_srgb;
		for (int i = 0; i < 3; ++i)
		{
			if ( col_lin[i] <= 0.0031308f )
				col_srgb[i] = 12.92 * col_lin[i];
			else
				col_srgb[i] = 1.055 * pow( col_lin[i], 1.0 / 2.4 ) - 0.055;
		}

		return float4( col_srgb.xyz, lin.a );
	#endif
}

float4 ConvertsRGBToLinear( float4 srgb )
{
	#if ( SKIP_SRGB_ENC_DEC )
		// If we're in ps 2.0, we don't have the instruction slots to do this correctly
		return srgb;
	#else
		float3 col_srgb = srgb.xyz;
		float3 col_lin;

		for (int i = 0; i < 3; ++i)
		{
			if ( col_srgb[i] <= 0.04045 )
				col_lin[i] = col_srgb[i] / 12.92;
			else
				col_lin[i] = pow( ( col_srgb[i] + 0.055 ) / 1.055, 2.4 );
		}

		return float4( col_lin.xyz, srgb.a );
	#endif
}


// Uses photoshop math to perform level adjustment. 
// Note: Photoshop does this math in sRGB space, even though that is mathematically wrong. 
// To match photoshop, we have to convert our textures from linear space (they're always linear in the shader)
// to sRGB, perform the calculations and then return to linear space for output from the shader.
// Yuck.
float AdjustLevels( float inSrc, float inBlackPoint, float inWhitePoint, float inGammaValue )
{
	if ( inBlackPoint == 0.0 && inWhitePoint == 1.0 && inGammaValue == 1.0 )
		return inSrc;
	else
	{
		inSrc = ConvertLinearTosRGB( inSrc );

		float pcg = saturate( invlerp( inBlackPoint, inWhitePoint, inSrc ) );
		float gammaAdjusted = pow( pcg, inGammaValue );

		gammaAdjusted = ConvertsRGBToLinear( gammaAdjusted );

		return saturate( gammaAdjusted );
	}
}

float4 AdjustLevels( float4 inSrc, float inBlackPoint, float inWhitePoint, float inGammaValue )
{
	if ( inBlackPoint == 0.0 && inWhitePoint == 1.0 && inGammaValue == 1.0 )
		return inSrc;
	else
	{
		inSrc = ConvertLinearTosRGB( inSrc );

		float4 pcg = saturate( invlerp( inBlackPoint, inWhitePoint, inSrc ) );
		float4 gammaAdjusted = pow( pcg, inGammaValue );

		gammaAdjusted = ConvertsRGBToLinear( gammaAdjusted );

		return saturate( gammaAdjusted );
	}
}

#if ( COMBINE_MODE == COMBINE_MODE_MULTIPLY || COMBINE_MODE == COMBINE_MODE_ADD ) || ( COMBINE_MODE == COMBINE_MODE_BLEND )
	float4 main_simple( PS_INPUT i )
	{
		float4 color0 = cNumTextures > 0 ? tex2D( InSampler0, i.texCoord0 ) : cSafeColor;
		float4 color1 = cNumTextures > 1 ? tex2D( InSampler1, i.texCoord1 ) : cSafeColor;
		#if ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE
			float4 color2 = cNumTextures > 2 ? tex2D( InSampler2, i.texCoord2 ) : cSafeColor;
			float4 color3 = cNumTextures > 3 ? tex2D( InSampler3, i.texCoord3 ) : cSafeColor;
		#endif

		color0 = cNumTextures > 0 ? AdjustLevels( color0, g_AdjustInBlack(0), g_AdjustInWhite(0), g_AdjustGamma(0) ) : cSafeColor;
		color1 = cNumTextures > 1 ? AdjustLevels( color1, g_AdjustInBlack(1), g_AdjustInWhite(1), g_AdjustGamma(1) ) : cSafeColor;
		#if ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE
			color2 = cNumTextures > 2 ? AdjustLevels( color2, g_AdjustInBlack(2), g_AdjustInWhite(2), g_AdjustGamma(2) ) : cSafeColor;
			color3 = cNumTextures > 3 ? AdjustLevels( color3, g_AdjustInBlack(3), g_AdjustInWhite(3), g_AdjustGamma(3) ) : cSafeColor;
		#endif
		#if ( COMBINE_MODE == COMBINE_MODE_MULTIPLY )
			return color0
				 * color1
				#if ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE
					 * color2
					 * color3
				#endif
			;
		#elif ( COMBINE_MODE == COMBINE_MODE_ADD )
			return color0
				 + color1
				#if ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE
					 + color2
					 + color3
				#endif
			;
		#elif ( COMBINE_MODE == COMBINE_MODE_BLEND )
			// color0 is the previous frame's data. 
			// color1 is the sticker's data, with alpha as transparency. 
			// If we're on PS 2.0b, color2 has specular info as a grayscale texture--make sure to write that out.

			#if ( DEBUG_MODE == 0 )
				#if ALLOW_FOUR_TEX_LOOKUPS_PER_STAGE
					float srcSpecular = color2.r;
				#else
					float srcSpecular = 1;
				#endif


				float3 tmpColor = ( 1.0 - color1.a ) * color0.xyz
								+ ( color1.a )       * color1.xyz;

				float tmpSpecular = ( 1.0 - color1.a ) * color0.w
				                  + ( color1.a )       * srcSpecular;

				return float4( tmpColor.xyz, tmpSpecular );
			#else
				if ( i.texCoord1.x < 0 || i.texCoord1.y < 0 
				  || i.texCoord1.x > 1 || i.texCoord1.y > 1 )
				{
					return color0;				
				}
				else
					return float4( i.texCoord1.xy, 0, color0.a );
			#endif
		#else
			#error "Surprising combine mode in function, update code."
		#endif
	}
#endif

#if ( COMBINE_MODE == COMBINE_MODE_LERP )
	float4 main_lerp( PS_INPUT i )
	{
		if (cNumTextures == 3)
		{
			float4 color0 = tex2D( InSampler0, i.texCoord0 );
			float4 color1 = tex2D( InSampler1, i.texCoord1 );
			float4 colSel = tex2D( InSampler2, i.texCoord2 );

			color0 = AdjustLevels( color0, g_AdjustInBlack(0), g_AdjustInWhite(0), g_AdjustGamma(0) );
			color1 = AdjustLevels( color1, g_AdjustInBlack(1), g_AdjustInWhite(1), g_AdjustGamma(1) );
			colSel = AdjustLevels( colSel, g_AdjustInBlack(2), g_AdjustInWhite(2), g_AdjustGamma(2) );

			#if ( COMBINE_MODE == COMBINE_MODE_LERP )
				return lerp( color0, color1, colSel.xxxx );
			#else
				#error "Surprising combine mode in function, update code."
			#endif
		}
		else
		{
			return float4( 1, 0, 0, 1 );
		}
	}
#endif

#if ( COMBINE_MODE == COMBINE_MODE_SELECTOR )
	float4 main_selector( PS_INPUT i )
	{
		if ( cNumTextures == 1 )
		{
			float fNormalizedColor = tex2D( InSampler0, i.texCoord0 ).x;
			float fTestColor = round( ( fNormalizedColor * 255.0 / 16.0f ) );

			bool4 bTestVec[4];
			for ( int i = 0; i < 4; ++i ) 
			{
				bTestVec[i] = cSelectValues[i] != 0 
						    ? round( cSelectValues[i] ) == fTestColor
						    : false;
			}

			bool4 bAny = bool4( 
				any( bTestVec[0] ), 
				any( bTestVec[1] ), 
				any( bTestVec[2] ), 
				any( bTestVec[3] )
			);

			#if ( COMBINE_MODE == COMBINE_MODE_SELECTOR )
				return any( bAny )
					 ? float4( 1, 1, 1, 1 ) 
					 : float4( 0, 0, 0, 0 );
			#else
				#error "Surprising combine mode in function, update code."
			#endif
		}
		else
		{
			return float4( 1, 1, 0, 1 );
		}
	}
#endif

#if ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_FIRST || COMBINE_MODE == COMBINE_MODE_LERP_TEX_SECOND )
	float4 main_lerp_multipass( PS_INPUT i )
	{
		if (cNumTextures == 2)
		{
			float4 colorN = tex2D( InSampler0, i.texCoord0 );
			float4 colSel = tex2D( InSampler1, i.texCoord1 );

			colorN = AdjustLevels( colorN, g_AdjustInBlack(0), g_AdjustInWhite(0), g_AdjustGamma(0) );
			colSel = AdjustLevels( colSel, g_AdjustInBlack(1), g_AdjustInWhite(1), g_AdjustGamma(1) );

			#if ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_FIRST )
				return colorN * ( 1.0 - colSel.xxxx );
			#elif ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_SECOND )
				return colorN * ( colSel.xxxx );
			#else
				#error "Surprising combine mode in function, update code."
			#endif
		}
		else
		{
			return float4( 1, 0, 0, 1 );
		}
	}
#endif

float4 main( PS_INPUT i ) : COLOR
{
	#if ( COMBINE_MODE == COMBINE_MODE_MULTIPLY )
		return main_simple( i );
	#elif ( COMBINE_MODE == COMBINE_MODE_ADD )
		return main_simple( i );
	#elif ( COMBINE_MODE == COMBINE_MODE_LERP )
		return main_lerp( i );
	#elif ( COMBINE_MODE == COMBINE_MODE_SELECTOR )
		return main_selector( i );
	#elif ( COMBINE_MODE == COMBINE_MODE_LERP_TEX_FIRST || COMBINE_MODE == COMBINE_MODE_LERP_TEX_SECOND )
		return main_lerp_multipass( i );
	#elif ( COMBINE_MODE == COMBINE_MODE_BLEND )
		return main_simple( i );
	#else
		#error "Need to add mode selection here."
	#endif
}
