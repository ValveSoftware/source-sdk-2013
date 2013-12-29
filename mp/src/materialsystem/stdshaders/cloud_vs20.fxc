#include "common_vs_fxc.h"

const float4 g_matBaseTexCoordTransform[2] : register( SHADER_SPECIFIC_CONST_0 );
const float4 g_matCloudTexCoordTransform[2] : register( SHADER_SPECIFIC_CONST_2 );

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float4 vTexCoord0	: TEXCOORD0;
	float4 vTexCoord1	: TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;
	float2 baseCoords		: TEXCOORD0;
	float2 cloudAlphaCoords	: TEXCOORD1;
	float  fogFactor		: TEXCOORD2;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;
	o.projPos = mul( v.vPos, cModelViewProj );

	// Compute fog based on the position
	float3 vWorldPos = mul( v.vPos, cModel[0] );
	o.fogFactor = CalcFog( vWorldPos, o.projPos, FOGTYPE_RANGE );

	// Texture coordinate transforms
	o.baseCoords.x = dot( v.vTexCoord0.xyzw, g_matBaseTexCoordTransform[0] );
	o.baseCoords.y = dot( v.vTexCoord0.xyzw, g_matBaseTexCoordTransform[1] );
	o.cloudAlphaCoords.x = dot( v.vTexCoord1.xyzw, g_matCloudTexCoordTransform[0] );
	o.cloudAlphaCoords.y = dot( v.vTexCoord1.xyzw, g_matCloudTexCoordTransform[1] );

	return o;
}
