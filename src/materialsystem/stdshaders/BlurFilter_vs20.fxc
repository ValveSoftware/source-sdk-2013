#include "common_vs_fxc.h"

struct VS_INPUT
{
	float3 vPos						: POSITION;
	float2 vBaseTexCoord			: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
	float2 coordTap0				: TEXCOORD0;
	float2 coordTap1				: TEXCOORD1;
	float2 coordTap2				: TEXCOORD2;
	float2 coordTap3				: TEXCOORD3;
	float2 coordTap1Neg				: TEXCOORD4;
	float2 coordTap2Neg				: TEXCOORD5;
	float2 coordTap3Neg				: TEXCOORD6;
};

float2 vsTapOffs[3] : register ( SHADER_SPECIFIC_CONST_0 );

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.projPos = float4( v.vPos, 1.0f );
	o.coordTap0    = v.vBaseTexCoord;
	o.coordTap1    = v.vBaseTexCoord + vsTapOffs[0];
	o.coordTap2    = v.vBaseTexCoord + vsTapOffs[1];
	o.coordTap3    = v.vBaseTexCoord + vsTapOffs[2];
	o.coordTap1Neg = v.vBaseTexCoord - vsTapOffs[0];
	o.coordTap2Neg = v.vBaseTexCoord - vsTapOffs[1];
	o.coordTap3Neg = v.vBaseTexCoord - vsTapOffs[2];

	return o;
}


