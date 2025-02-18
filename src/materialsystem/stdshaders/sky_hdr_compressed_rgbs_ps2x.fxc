// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]
#include "common_ps_fxc.h"

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

sampler RGBSTextureSampler	: register( s0 );
HALF4 InputScale : register( c0 );

float2 texWidthHeight : register( c1 );

float4 texOffsets : register( c2 );

struct PS_INPUT
{
//#if defined( _X360 )
//	float2 baseTexCoord		: TEXCOORD0;
//#else
	float2 baseTexCoord00 : TEXCOORD0;
	float2 baseTexCoord01 : TEXCOORD1;
	float2 baseTexCoord10 : TEXCOORD2;
	float2 baseTexCoord11 : TEXCOORD3;
	float2 baseTexCoord_In_Pixels: TEXCOORD4;
//#endif
};

float4 main( PS_INPUT i ) : COLOR
{
	float3 result;
	
//#if defined( _X360 ) //360 has a cheaper way to handle RGBscale
//	float4 Weights;
//	float4 samples_0; //no arrays allowed in inline assembly
//	float4 samples_1;
//	float4 samples_2;
//	float4 samples_3;
//	float2 vTexCoord = i.baseTexCoord;
//	
//	asm {
//		tfetch2D samples_0, vTexCoord.xy, RGBSTextureSampler, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
//		tfetch2D samples_1, vTexCoord.xy, RGBSTextureSampler, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
//		tfetch2D samples_2, vTexCoord.xy, RGBSTextureSampler, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
//		tfetch2D samples_3, vTexCoord.xy, RGBSTextureSampler, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
//
//		getWeights2D Weights, vTexCoord.xy, RGBSTextureSampler
//	};
//
//	Weights = float4( (1-Weights.x)*(1-Weights.y), Weights.x*(1-Weights.y), (1-Weights.x)*Weights.y, Weights.x*Weights.y );
//
//	result.rgb = samples_0.rgb * (samples_0.a * Weights.x);
//	result.rgb += samples_1.rgb * (samples_1.a * Weights.y);
//	result.rgb += samples_2.rgb * (samples_2.a * Weights.z);
//	result.rgb += samples_3.rgb * (samples_3.a * Weights.w);
//	
//#else
	float4 s00 = tex2D(RGBSTextureSampler, i.baseTexCoord00); 
	float4 s10 = tex2D(RGBSTextureSampler, i.baseTexCoord10); 
	float4 s01 = tex2D(RGBSTextureSampler, i.baseTexCoord01); 
	float4 s11 = tex2D(RGBSTextureSampler, i.baseTexCoord11); 
	
	float2 fracCoord = frac(i.baseTexCoord_In_Pixels);

	s00.rgb*=s00.a;
	s10.rgb*=s10.a;

	s00.xyz = lerp(s00, s10, fracCoord.x); 
	
	s01.rgb*=s01.a;
	s11.rgb*=s11.a;
	s01.xyz = lerp(s01, s11, fracCoord.x); 
	
	result = lerp(s00, s01, fracCoord.y); 
//#endif
 
	// This is never fogged.
	return FinalOutput( float4( InputScale*result, 1.0f ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR, WRITE_DEPTH_TO_DESTALPHA, 1e20 ); //when writing depth to dest alpha, write a value guaranteed to saturate
}
