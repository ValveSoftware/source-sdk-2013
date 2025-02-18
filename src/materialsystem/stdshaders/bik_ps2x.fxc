// DYNAMIC: "PIXELFOGTYPE"				"0..1"

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

sampler YTextureSampler		: register( s0 );
sampler cRTextureSampler	: register( s1 );
sampler cBTextureSampler	: register( s2 );
//sampler ATextureSampler		: register( s3 );

struct PS_INPUT
{
	HALF2 baseTexCoord					: TEXCOORD0;
	HALF4 worldPos_projPosZ				: TEXCOORD1;
	float4 fogFactorW					: COLOR1;
};

#if 0
static float yuvtorgb[] =
{
   1.164123535f,  1.595794678f,  0.0f,         -0.87065506f,
   1.164123535f, -0.813476563f, -0.391448975f,  0.529705048f,
   1.164123535f,  0.0f,          2.017822266f, -1.081668854f,
   1.0f, 0.0f, 0.0f, 0.0f
};
" sampler tex0   : register( s0 );      "
" sampler tex1   : register( s1 );      "
" sampler tex2   : register( s2 );      "
" sampler tex3   : register( s3 );      "
" float4  tor    : register( c0 );      "
" float4  tog    : register( c1 );      "
" float4  tob    : register( c2 );      "
" float4  consts : register( c3 );      "
"                                       "
" struct VS_OUT                         "
" {                                     "
"     float2 T0: TEXCOORD0;             "
" };                                    "
"                                       "
" float4 main( VS_OUT In ) : COLOR      "
" {                                     "
"   float4 c;                           "
"   float4 p;                           "
"   c.x = tex2D( tex0, In.T0 ).x;       "
"   c.y = tex2D( tex1, In.T0 ).x;       "
"   c.z = tex2D( tex2, In.T0 ).x;       "
"   c.w = consts.x;                     "
"   p.w = tex2D( tex3, In.T0 ).x;       "
"   p.x = dot( tor, c );                "
"   p.y = dot( tog, c );                "
"   p.z = dot( tob, c );                "
"   p.w *= consts.w;                    "
"   return p;                           "
" }                                     ";
#endif

float4 main( PS_INPUT i ) : COLOR
{
	half y, cR, cB;
	y = tex2D( YTextureSampler, i.baseTexCoord.xy );
	cR = tex2D( cRTextureSampler, i.baseTexCoord.xy );
	cB = tex2D( cBTextureSampler, i.baseTexCoord.xy );
//	half a = tex2D( ATextureSampler, i.baseTexCoord.xy );
	
	HALF4 c;
	c = float4( y, cR, cB, 1.0f );

	float4 tor = float4( 1.164123535f,  1.595794678f,  0.0f,         -0.87065506f );
	float4 tog = float4( 1.164123535f, -0.813476563f, -0.391448975f,  0.529705048f );
	float4 tob = float4( 1.164123535f,  0.0f,          2.017822266f, -1.081668854f );

	HALF4 rgba;
	
	rgba.r = dot( c, tor );
	rgba.g = dot( c, tog );
	rgba.b = dot( c, tob );
	rgba.a = 1.0f;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	float4 result = FinalOutput( rgba, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE );

	// The 360 will do an sRGB write to put the linear values into 360 gamma space, but the PC just outputs gamma values directly
	#if defined( _X360 )
		result.r = SrgbGammaToLinear( result.r );
		result.g = SrgbGammaToLinear( result.g );
		result.b = SrgbGammaToLinear( result.b );
	#endif

	return result.rgba;
}
