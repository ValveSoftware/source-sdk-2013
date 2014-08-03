// STATIC: "VERTEXCOLOR"			"0..1"
// DYNAMIC: "COMPRESSED_VERTS"		"0..1"
// DYNAMIC: "DOWATERFOG"			"0..1"
// DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const int g_FogType			= DOWATERFOG;
static const bool g_bSkinning		= SKINNING ? true : false;

const float4 cBaseTextureTransform[2]	: register( SHADER_SPECIFIC_CONST_0 );
const float4 cMaskTextureTransform[2]	: register( SHADER_SPECIFIC_CONST_2 );
const float4 cDetailTextureTransform[2] : register( SHADER_SPECIFIC_CONST_4 );
const float4 g_vVertexColor				: register( SHADER_SPECIFIC_CONST_6 );

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float4 vBoneWeights	: BLENDWEIGHT;
	float4 vBoneIndices	: BLENDINDICES;
	float4 vNormal		: NORMAL;

#if VERTEXCOLOR
	float4 vColor		: COLOR0;
#endif	

	float4 vTexCoord0	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;
	float2 vTexCoord3	: TEXCOORD3;

	float4 vColor		: COLOR0;
	float4 fogFactorW	: COLOR1;

#if !defined( _X360 )    
	float  fog		: FOG;
#endif

	float4 worldPos_projPosZ : TEXCOORD7;		// Necessary for pixel fog
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	float3 worldNormal;

	//------------------------------------------------------------------------------
	// Vertex blending
	//------------------------------------------------------------------------------
	SkinPosition( g_bSkinning, v.vPos, v.vBoneWeights, v.vBoneIndices, worldPos );

	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.vProjPos = vProjPos;
	vProjPos = dot( float4( worldPos, 1 ), cViewProjZ );
	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );

	//------------------------------------------------------------------------------
	// Fog
	//------------------------------------------------------------------------------
	o.fogFactorW = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif

	//------------------------------------------------------------------------------
	// Texture coord transforms
	//------------------------------------------------------------------------------
	o.vTexCoord0 = mul( v.vTexCoord0, (float2x4)cBaseTextureTransform );
	o.vTexCoord3 = mul( v.vTexCoord0, (float2x4)cDetailTextureTransform );

	o.vColor = cModulationColor;
	
#if VERTEXCOLOR	
	// 0 or 1 for g_vVertexColor.x, eliminating a bool
	o.vColor = lerp( o.vColor, o.vColor * v.vColor, g_vVertexColor.x );
#endif	

	return o;
}



