#include "common_vs_fxc.h"

const float4 cLightmapTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_0 );

struct VS_INPUT
{
	float3 vPos							: POSITION;
	float2 vBaseTexCoord				: TEXCOORD0;
	float2 vLightmapTexCoord			: TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 projPos					: POSITION;	
	float2 baseTexCoord				: TEXCOORD0;
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float4 projPos;
	float3 worldPos;

	worldPos = mul4x3( float4( v.vPos, 1 ), cModel[0] );
	projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
		
	o.baseTexCoord.x = dot( v.vLightmapTexCoord, cLightmapTexCoordTransform[0].xy ) + cLightmapTexCoordTransform[0].w;
	o.baseTexCoord.y = dot( v.vLightmapTexCoord, cLightmapTexCoordTransform[1].xy ) + cLightmapTexCoordTransform[1].w;
	
#ifdef _PS3
	// Account for OpenGL's flipped y coordinate and expanded z range [-1,1] instead of [0,1]
	o.projPos.y = -o.projPos.y;
	o.projPos.z = 2.0f * o.projPos.z - o.projPos.w;
#endif // _PS3

	return o;
}
