//  STATIC: "VERTEXCOLOR"			"0..1"
//  STATIC: "SRGB"			"0..1"
//	DYNAMIC: "DOWATERFOG"				"0..1"

#include "common_vs_fxc.h"
 
static const int g_FogType			= DOWATERFOG;
static const bool g_bVertexColor     = VERTEXCOLOR ? true : false;

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vColor			: COLOR0;
	// make these float2's and stick the [n n 0 1] in the dot math.
	float4 vTexCoord0		: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;			// Projection-space position
#if !defined( _X360 )    
	float  fog						: FOG;
#endif
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	float4 color					: TEXCOORD2;		// Vertex color (from lighting or unlit)
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;
	worldPos = mul4x3( v.vPos, cModel[0] );

	// Transform into projection space
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	projPos.z = dot( float4( worldPos, 1 ), cViewProjZ );
	
	o.worldPos_projPosZ = float4( worldPos.xyz, projPos.z );
	
#if !defined( _X360 )
	o.fog = CalcFog( worldPos, projPos, g_FogType );
#endif
	if ( g_bVertexColor )
	{
		// Assume that this is unlitgeneric if you are using vertex color.
#if SRGB
		o.color.rgba = GammaToLinear( v.vColor.rgba );
#else
		o.color.rgba = v.vColor.rgba;
#endif
	}

	// Base texture coordinates
	o.baseTexCoord.xy = v.vTexCoord0.xy;

	return o;
}


