//	DYNAMIC: "DOWATERFOG"			"0..1"
#include "common_vs_fxc.h"
 
static const int  g_FogType			= DOWATERFOG;

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vTexCoord0		: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;			// Projection-space position	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	float4 worldPos_projPosZ		: TEXCOORD1;		// Necessary for water fog dest alpha
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float4 projPos;
	float3 worldPos;

	projPos = mul( float4( v.vPos.xyz, 1 ), cModelViewProj );
	o.projPos = projPos;

	worldPos = mul( float4( v.vPos.xyz, 1 ), cModel[0] );

	o.fogFactorW.w = CalcFog( worldPos, projPos, g_FogType );
#if !defined( _X360 )	
	o.fog = o.fogFactorW.w;
#endif

	o.worldPos_projPosZ = float4( worldPos, projPos.z );
	o.baseTexCoord.xy = v.vTexCoord0;
	return o;
}


