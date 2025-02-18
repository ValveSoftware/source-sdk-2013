// STATIC: "HALFLAMBERT"				"0..1"
// STATIC: "USE_STATIC_CONTROL_FLOW"	"0..1" [vs20]

// DYNAMIC: "DOWATERFOG"				"0..1"
// DYNAMIC: "SKINNING"					"0..1"
// DYNAMIC: "DYNAMIC_LIGHT"				"0..1"
// DYNAMIC: "STATIC_LIGHT"				"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..2" [vs20]

// If using static control flow on Direct3D, we should use the NUM_LIGHTS=0 combo
// SKIP: $USE_STATIC_CONTROL_FLOW && ( $NUM_LIGHTS > 0 ) [vs20]

#include "common_vs_fxc.h"

static const int  g_FogType			= DOWATERFOG;
static const bool g_bHalfLambert	= HALFLAMBERT ? true : false;
static const int  g_nSkinning		= SKINNING;
static const bool g_bDynamicLight	= DYNAMIC_LIGHT ? true : false;
static const bool g_bStaticLight	= STATIC_LIGHT ? true : false;

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
	float4 vNormal			: NORMAL;
	
	float3 vSpecular		: COLOR1;
	
	float2 vTexCoord0		: TEXCOORD0;
	float2 vTexCoord1		: TEXCOORD1;
	float2 vTexCoord2		: TEXCOORD2;
	
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;	
	float3 vColor0		: COLOR0;
	
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

	// Compute vertex lighting
#if ( USE_STATIC_CONTROL_FLOW ) || defined ( SHADER_MODEL_VS_3_0 )
	o.vColor0 = DoLighting( worldPos, worldNormal, float3(0.0f, 0.0f, 0.0f), g_bStaticLight, g_bDynamicLight, g_bHalfLambert );
#else
	o.vColor0 = DoLightingUnrolled( worldPos, worldNormal, float3(0.0f, 0.0f, 0.0f), g_bStaticLight, g_bDynamicLight, g_bHalfLambert, NUM_LIGHTS );
#endif

	return o;
}