// DYNAMIC: "DOWATERFOG"	"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float2 vTexCoord1		: TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	
	float4 vDiffuse		: COLOR0;
	
	float4 fogFactorW	: COLOR1;

#if !defined( _X360 )    
	float  fog			: FOG;
#endif
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	worldPos = mul4x3( v.vPos, cModel[0] );
		
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	
	o.fogFactorW = CalcFog( worldPos, o.vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif

	// YUCK!  This is to make texcoords continuous for mat_softwaretl
	o.vTexCoord0 = 0.0f;
	o.vTexCoord1 = v.vTexCoord1;
	
	o.vDiffuse = 1.0f;
	
	return o;
}