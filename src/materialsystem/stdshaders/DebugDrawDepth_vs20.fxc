//  DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;

const float2 cDepthFactor			:  register( SHADER_SPECIFIC_CONST_0 );

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
	float2 zValue					: TEXCOORD0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	SkinPosition( g_bSkinning, v.vPos, v.vBoneWeights, v.vBoneIndices, worldPos );
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	o.zValue.x = (o.projPos.z - cDepthFactor.y) / cDepthFactor.x;
	o.zValue.y = (o.projPos.w - cDepthFactor.y) / cDepthFactor.x;
	return o;
}




