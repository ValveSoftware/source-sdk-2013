// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_NONE
#define HDRENABLED 0
#include "common_ps_fxc.h"

sampler AlbedoSampler	: register( s0 );
sampler NormalSampler   : register( s1 );
sampler PositionSampler : register( s2 );
sampler AccBuf_In       : register( s3 );

float4 EyePosition : register (c10);

float4 Light_origin : register( c0 );
#define INNER_COS (Light_origin.w)
#define OUTER_COS (Light_dir.w)

float4 Light_dir : register( c1 );

float4 Light_attn : register (c2);
#define QUADRATIC_ATTN (Light_attn.x)
#define LINEAR_ATTN (Light_attn.y)
#define CONSTANT_ATTN (Light_attn.z)
#define SCALE_FACTOR (Light_attn.w)

float3 Light_color: register(c3);

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
};


float Lerp5(float f1, float f2, float i1, float i2, float x)
{
  return f1+(f2-f1)*(x-i1)/(i2-i1);
}

float4 main( PS_INPUT i ) : COLOR
{
	float4 normal=tex2D( NormalSampler, i.texCoord );
	float4 albedo=tex2D( AlbedoSampler, i.texCoord );
	float4 pos=tex2D( PositionSampler, i.texCoord );
	float3 old_acc=tex2D( AccBuf_In, i.texCoord );
//	pos.xyz+=EyePosition.xyz;

	float3 ldir=Light_origin.xyz-pos.xyz;
	float dist=sqrt(dot(ldir,ldir));
	ldir=normalize(ldir);
	float spot_dot=dot(ldir,-Light_dir);
	float3 ret=Light_color*0.09*albedo.xyz;         // ambient
	float dist_falloff=(SCALE_FACTOR/(QUADRATIC_ATTN*dist*dist+LINEAR_ATTN*dist+CONSTANT_ATTN));
 	if (spot_dot>OUTER_COS)
 	{
		float falloff=1;
		if (spot_dot<INNER_COS)
		{
			falloff=Lerp5(1,0,INNER_COS,OUTER_COS,spot_dot);
		}
		float dotprod=max(0,dot(ldir.xyz,normal.xyz));
		ret+=dotprod*falloff*(Light_color*albedo.xyz);
	}
	else
		dist_falloff=min(1,dist_falloff);
	ret*=dist_falloff;
//	ret=float3(1,0,0);
	return FinalOutput( float4(ret+old_acc,1), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
