// DYNAMIC: "DOWATERFOG"	"0..1"

#include "common_vs_fxc.h"

static const int g_FogType	= DOWATERFOG;

const float4 cShaderConst0	: register( SHADER_SPECIFIC_CONST_0 );
const float4 cShaderConst1	: register( SHADER_SPECIFIC_CONST_1 );

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float4 vTexCoord0	: TEXCOORD0;
	float4 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;

	float4 vColor		: COLOR0;
};

struct VS_OUTPUT
{
	float4 vProjPos		: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;
	float2 vTexCoord3	: TEXCOORD3;
	
	float4 worldPos_projPosZ		: TEXCOORD4;		// Necessary for pixel fog

	float4 vColor : COLOR0;

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


	// Compute the texture coordinates given the offset between
	// each bumped lightmap
	float2 offset;
	offset.x = v.vTexCoord2.x;
	offset.y = 0.0f;
	
	o.vTexCoord0.x = dot( v.vTexCoord0, cShaderConst0 );
	o.vTexCoord0.y = dot( v.vTexCoord0, cShaderConst1 );
	
	o.vTexCoord1 = offset + v.vTexCoord1.xy;
	o.vTexCoord2 = (offset * 2.0) + v.vTexCoord1.xy;	
	o.vTexCoord3 = (offset * 3.0) + v.vTexCoord1.xy;
	
	o.vColor = v.vColor;

	return o;
}