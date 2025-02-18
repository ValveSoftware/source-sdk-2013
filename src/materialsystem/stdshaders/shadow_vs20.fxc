//	DYNAMIC: "DOWATERFOG"				"0..1"

#include "common_vs_fxc.h"

static const int g_FogType = DOWATERFOG;

const float4 cBaseTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cTextureJitter[2]					: register( SHADER_SPECIFIC_CONST_2 );

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vColor			: COLOR0;
	float4 vTexCoord0		: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float2 texCoord0				: TEXCOORD0;
	float2 texCoord1				: TEXCOORD1;
	float2 texCoord2				: TEXCOORD2;
	float2 texCoord3				: TEXCOORD3;
	float2 texCoord4				: TEXCOORD4;
	HALF4 worldPos_projPosZ		    : TEXCOORD5;
	float4 shadowColor				: COLOR0;
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldNormal, worldPos;
	float2 texCoord;
	worldPos = mul( v.vPos, cModel[0] );
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	projPos.z = dot( float4( worldPos, 1 ), cViewProjZ );
	
	// NOTE: CalcFog returns 1 for non-water fog, so it's ok to use fog
	// It returns the real fog for range fog, since we do vertex fog
	o.fogFactorW = CalcFog( worldPos, projPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;	
#endif
	o.shadowColor = v.vColor;
	o.worldPos_projPosZ = float4( worldPos, projPos.z );

	texCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );
	texCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );

	o.texCoord0.xy = texCoord;
	o.texCoord1.xy = texCoord + cTextureJitter[0];
	o.texCoord2.xy = texCoord - cTextureJitter[0];
	o.texCoord3.xy = texCoord + cTextureJitter[1];
	o.texCoord4.xy = texCoord - cTextureJitter[1];

	return o;
}


