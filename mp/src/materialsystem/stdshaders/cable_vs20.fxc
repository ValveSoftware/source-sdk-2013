// DYNAMIC: "DOWATERFOG"	"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;

struct VS_INPUT
{
	float4 vPos		: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;

	float4 directionalLightColor : COLOR0;

	float3 vTangentS	: TANGENT;
	float3 vTangentT	: BINORMAL;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog

	float4 directionalLightColor : COLOR0;

	float4 fogFactorW	: COLOR1;

#if !defined( _X360 )    
	float  fog		: FOG;
#endif
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	worldPos = mul( v.vPos, cModel[0] );
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.vProjPos = vProjPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );

	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );

	o.fogFactorW = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif

	//------------------------------------------------------------------------------
	// Setup the tangent space
	//------------------------------------------------------------------------------
	
	// Get S crossed with T (call it R)
	float3 r = cross( v.vTangentS, v.vTangentT );
	
	// Normalize S (into s)
	float3 s = normalize( v.vTangentS );
	
	// Normalize R (into r)
	r = normalize( r );

	// Regenerate T (into t)
	float3 t = cross( r, v.vTangentS );

	//------------------------------------------------------------------------------
	// Copy texcoords for the normal map and base texture
	//------------------------------------------------------------------------------
	o.vTexCoord0 = v.vTexCoord0;
	o.vTexCoord1 = v.vTexCoord1;

	// Pass the dirlight color through
	o.directionalLightColor = v.directionalLightColor;

	return o;
}