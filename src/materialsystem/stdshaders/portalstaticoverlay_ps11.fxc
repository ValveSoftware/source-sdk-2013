// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"

// DYNAMIC: "HDRENABLED"		"0..1"
// DYNAMIC: "PIXELFOGTYPE"		"0..1"

#include "common_ps_fxc.h"

const HALF3 g_StaticAmount	: register( c0 ); //x is static, y is 1.0 - static

#if( HASSTATICTEXTURE )
	sampler StaticTextureSampler	: register( s0 );
#	if( HASALPHAMASK )
		sampler AlphaMaskSampler	: register( s1 );
#	endif
#else
#	if( HASALPHAMASK )
		sampler AlphaMaskSampler	: register( s0 );
#	endif
#endif

struct PS_INPUT
{
	float4 vProjPos					: POSITION;
	
	//vStaticTexCoord and vAlphaMaskTexCoord are the same numbers, but we need to map TEXCOORD0 to sampler 0, and TEXCOORD1 to sampler1. ps11 compatibility issue
#if( HASSTATICTEXTURE )
	float2 vStaticTexCoord			: TEXCOORD0;
#	if( HASALPHAMASK )
		float2 vAlphaMaskTexCoord	: TEXCOORD1;
#	endif
#else
#	if( HASALPHAMASK )
		float2 vAlphaMaskTexCoord	: TEXCOORD0;
#	else
		float2 vUnusedTexCoord1		: TEXCOORD0;
#	endif
	float2 vUnusedTexCoord2			: TEXCOORD1;
#endif
};




HALF4 main( PS_INPUT i ) : COLOR
{
	HALF4 result;
	
#	if( HASSTATICTEXTURE )
		result.rgb = tex2D( StaticTextureSampler, i.vStaticTexCoord ).rgb;
#	else
		result.rgb = 0.25; //without a static texture, just be gray
#	endif



#	if( HASALPHAMASK )
		result.a = min( g_StaticAmount.x, tex2D( AlphaMaskSampler, i.vAlphaMaskTexCoord ).a ); //when static reaches 0, fades away completely, also never exceeds the mask's alpha
#	else
		result.a = g_StaticAmount.x; //when static reaches 0, fades away completely
#	endif

	//result.r = 1.0;
	//result.gb = 0.0;
	//result.a = 0.5;

	return result;
}