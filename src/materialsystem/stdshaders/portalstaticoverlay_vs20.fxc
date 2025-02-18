// STATIC: "MODEL"			"0..1"

// DYNAMIC: "SKINNING"		"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning	= SKINNING ? true : false;
static const bool g_bModel		= MODEL ? true : false;

struct VS_INPUT
{
	float4 vPos							: POSITION;

	float4 vBoneWeights					: BLENDWEIGHT;
	float4 vBoneIndices					: BLENDINDICES;
	float4 vTexCoord					: TEXCOORD0;
};


struct VS_OUTPUT
{
	float4 vProjPos					: POSITION;
	float2 vTexCoord1				: TEXCOORD0;
	float2 vTexCoord2				: TEXCOORD1; //just a copy of vTexCoord1, ps11 compatibility issue
	float vFog						: FOG;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	if( MODEL == 1 )
	{
		SkinPosition( 
				g_bSkinning, 
				v.vPos,
				v.vBoneWeights, v.vBoneIndices,
				worldPos );
	}
	else
	{
		worldPos = mul( v.vPos.xyz, cModel[0] );
	}
	
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.worldPos_projPosZ = float4( worldPos.xyz, o.vProjPos.z );
	o.vTexCoord1 = v.vTexCoord.xy;
	o.vTexCoord2 = v.vTexCoord.xy;
	o.vFog = CalcFog( worldPos, o.vProjPos.xyz, FOGTYPE_RANGE );

	return o;
}

	