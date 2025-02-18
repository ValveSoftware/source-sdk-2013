// DYNAMIC: "DOWATERFOG"	"0..1"
// DYNAMIC: "SKINNING"		"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;
static const bool g_bSkinning = SKINNING ? true : false;

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	SkinPosition( g_bSkinning, v.vPos, v.vBoneWeights, v.vBoneIndices, worldPos );
	
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	
	return o;
}