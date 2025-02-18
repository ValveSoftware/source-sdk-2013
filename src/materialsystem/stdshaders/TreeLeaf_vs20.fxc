//  STATIC: "HALFLAMBERT"				"0..1"
//  STATIC: "USE_STATIC_CONTROL_FLOW"	"0..1" [vs20]

//	DYNAMIC: "DYNAMIC_LIGHT"			"0..1"
//	DYNAMIC: "STATIC_LIGHT"				"0..1"
//  DYNAMIC: "NUM_LIGHTS"				"0..2" [vs20]

// If using static control flow on Direct3D, we should use the NUM_LIGHTS=0 combo
//  SKIP: $USE_STATIC_CONTROL_FLOW && ( $NUM_LIGHTS > 0 ) [vs20]

#include "common_vs_fxc.h"

static const bool g_bHalfLambert	= HALFLAMBERT ? true : false;

const float3 cLeafCenter : register(SHADER_SPECIFIC_CONST_0);

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vNormal			: NORMAL;
	float2 vTexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos		: POSITION;	
	float2 texCoord		: TEXCOORD0;
	float3 color		: COLOR;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	bool bDynamicLight = DYNAMIC_LIGHT ? true : false;
	bool bStaticLight = STATIC_LIGHT ? true : false;

	float3 worldPos;
	worldPos = mul( v.vPos, cModel[0] );

	float3 normal = v.vPos.xyz - cLeafCenter.xyz;
	normal = normalize( normal );

	float3 worldNormal = mul( float4( normal, 0.0f ), cModel[0] );

#if ( USE_STATIC_CONTROL_FLOW ) || defined ( SHADER_MODEL_VS_3_0 )
	float3 lighting = DoLighting( worldPos, worldNormal, float3(0,0,0), bStaticLight, bDynamicLight, g_bHalfLambert );
#else
	float3 lighting = DoLightingUnrolled( worldPos, worldNormal, float3(0,0,0), bStaticLight, bDynamicLight, g_bHalfLambert, NUM_LIGHTS );
#endif

	float3 xAxis = float3( cViewModel[0].x, cViewModel[1].x, cViewModel[2].x );
	float3 yAxis = float3( cViewModel[0].y, cViewModel[1].y, cViewModel[2].y );

	worldPos += xAxis * v.vTexCoord.x;
	worldPos += yAxis * (1.0f-v.vTexCoord.y);

	float4 projPos  = mul( float4(worldPos, 1.0f), cViewProj );

	float3 light_vec = float3( 1.0f, 0.0, 1.0 );
	light_vec = normalize( light_vec );

	o.projPos       = projPos;
	// FIXME: if this shader gets put back into use, be sure this usage of normals jives with compressed verts
	o.texCoord      = v.vNormal.xy;
	o.color         = lighting;

	return o;
}


