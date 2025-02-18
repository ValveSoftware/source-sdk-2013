// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"

// DYNAMIC: "HDRENABLED"		"0..1"
// DYNAMIC: "PIXELFOGTYPE"		"0..1"

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

const HALF3 g_StaticAmount	: register( c0 ); //x is static, y is 1.0 - static

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

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

	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};




float4 main( PS_INPUT i ) : COLOR
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


	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}