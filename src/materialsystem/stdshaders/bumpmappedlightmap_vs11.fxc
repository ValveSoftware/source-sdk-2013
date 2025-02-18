// DYNAMIC: "DOWATERFOG"	"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;
	float2 vTexCoord3	: TEXCOORD3;
	
	
	float4 fogFactorW	: COLOR1;

#if !defined( _X360 )    
	float  fog			: FOG;
#endif
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	worldPos = mul( v.vPos, cModel[0] );
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
		
	o.fogFactorW = CalcFog( worldPos, o.vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif

	
	// Compute the texture coordinates given the offset between each bumped lightmap

	float2 bumpOffset;
	bumpOffset = v.vTexCoord2;
	
	o.vTexCoord0 = v.vTexCoord0;						// bumpmap texcoords
	
	o.vTexCoord1 = bumpOffset + v.vTexCoord1;			// first lightmap texcoord
	o.vTexCoord2 = (bumpOffset * 2.0) + v.vTexCoord1;	// second lightmap texcoord
	
	o.vTexCoord3 = (bumpOffset * 3.0) + v.vTexCoord1;	// third lightmpa texcoord
	
	return o;
}