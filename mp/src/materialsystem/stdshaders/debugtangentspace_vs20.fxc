// DYNAMIC: "COMPRESSED_VERTS"	"0..1"
// DYNAMIC: "DOWATERFOG"		"0..1"
// DYNAMIC: "SKINNING"			"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;
static const bool g_bSkinning		= SKINNING ? true : false;


struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
	float4 vNormal			: NORMAL;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	
	float4 vDiffuse		: COLOR0;
	
	float4 fogFactorW	: COLOR1;

#if !defined( _X360 )    
	float  fog			: FOG;
#endif
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	float3 worldPos, worldNormal;
	SkinPositionAndNormal( g_bSkinning, v.vPos, vObjNormal, v.vBoneWeights, v.vBoneIndices, worldPos, worldNormal );
		
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	
	o.fogFactorW = CalcFog( worldPos, o.vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif

	// stick the normal in the color channel
	o.vDiffuse.rgb = worldNormal;
	o.vDiffuse.a = 1.0f;
	
	return o;
}