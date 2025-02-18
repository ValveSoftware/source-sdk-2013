//	DYNAMIC: "DOWATERFOG"				"0..1"

#include "common_vs_fxc.h"

static const int g_FogType			= DOWATERFOG;

const float4 cBaseTexCoordTransform[2]			: register( SHADER_SPECIFIC_CONST_0 );
const float4 cBaseTexCoordTransform2[2]			: register( SHADER_SPECIFIC_CONST_2 );
const float4 cMacrosTexCoordTransform[2]		: register( SHADER_SPECIFIC_CONST_4 );

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vColor			: COLOR0;
	float4 vTexCoord0		: TEXCOORD0;
	float4 vTexCoord1		: TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float2 baseCoord				: TEXCOORD0;
	float2 baseCoord2				: TEXCOORD1;
	float2 lightmapCoord			: TEXCOORD2;
	float2 macrosCoord				: TEXCOORD3;
	float4 color					: COLOR0;
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldNormal, worldPos;
	float2 texCoord;
	worldPos = mul( v.vPos, cModel[0] );
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );
	o.fogFactorW = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif
	o.color = v.vColor;

	o.baseCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );
	o.baseCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );

	o.baseCoord2.x = dot( v.vTexCoord0, cBaseTexCoordTransform2[0] );
	o.baseCoord2.y = dot( v.vTexCoord0, cBaseTexCoordTransform2[1] );

	o.lightmapCoord = v.vTexCoord1;

	o.macrosCoord.x = dot( v.vTexCoord0, cMacrosTexCoordTransform[0] );
	o.macrosCoord.y = dot( v.vTexCoord0, cMacrosTexCoordTransform[1] );

	return o;
}


