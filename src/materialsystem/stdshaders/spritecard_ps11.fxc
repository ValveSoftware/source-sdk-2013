//	STATIC: "ADDBASETEXTURE2"		"0..1"
//  STATIC: "ADDSELF"               "0..1"
//  STATIC: "USEALPHAASRGB"         "0..1"
//  SKIP: $USEALPHAASRGB && $ADDSELF
//  SKIP: $USEALPHAASRGB && $ADDBASETEXTURE2

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD1;
	float4 argbcolor		: COLOR;
	float4 blendfactor0		: TEXCOORD2;
#if ADDBASETEXTURE2
	float2 texCoord2		: TEXCOORD3;
#endif
	float4 vScreenPos		: TEXCOORD7;
};

sampler BaseTextureSampler	: register( s0 );

#if ADDBASETEXTURE2
sampler BaseTextureSampler2	: register( s3 );
#endif

sampler BaseTextureSampler1	: register( s1 );
const float4 g_Parameters	: register( c0 );
const float4 g_ColorPowers	: register( c1 );

#define fAdditiveBlendWeight		g_Parameters.x
#define fOverbrightFactor			g_Parameters.y
#define fAdditiveSelfBlendWeight	g_Parameters.z
#define fSoftParticleBlendScale		g_Parameters.w

#pragma warning( disable : 4707 4704 ) 
float4 main( PS_INPUT i ) : COLOR
{
	// Sample frames from texture 0
#if ( ! ADDSELF ) && ( ! ADDBASETEXTURE2 )
	float4 baseTex0 = tex2D( BaseTextureSampler, i.texCoord0 );
	float4 baseTex1 = tex2D( BaseTextureSampler1, i.texCoord1 );
	float4 blended_rgb = lerp( baseTex0, baseTex1, i.blendfactor0.x );
#else
	float4 blended_rgb = tex2D( BaseTextureSampler, i.texCoord0 );
#endif
#if USEALPHAASRGB
	blended_rgb.rgb = blended_rgb.a;
#endif

#if ADDBASETEXTURE2
	blended_rgb.a *= i.argbcolor.a;
	
	// In this case, we don't really want to pre-multiply by alpha
	float4 color2 = tex2D( BaseTextureSampler2, i.texCoord2 );
	blended_rgb.rgb *= blended_rgb.a;
	blended_rgb.rgb += fOverbrightFactor * fAdditiveBlendWeight * i.argbcolor.a * color2;
	blended_rgb.rgb *= 2 * i.argbcolor.rgb;
#else
#if ADDSELF
	blended_rgb.a *= i.argbcolor.a;
	blended_rgb.rgb *= blended_rgb.a;
	blended_rgb.rgb += fOverbrightFactor * 8 * fAdditiveSelfBlendWeight * blended_rgb;
	blended_rgb.rgb *= 2 * i.argbcolor.rgb;
#else
	blended_rgb *= i.argbcolor;
#endif
#endif
	return blended_rgb;
}

