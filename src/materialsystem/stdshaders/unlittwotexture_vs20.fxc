//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;
static const int  g_FogType			= DOWATERFOG;

const float4 cBaseTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cBaseTexCoordTransform2[2]			:  register( SHADER_SPECIFIC_CONST_2 );

struct VS_INPUT
{
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
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
	HALF2 baseTexCoord2				: TEXCOORD1;		// Base texture coordinate
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for water fog dest alpha
	
	float4 vColor					: COLOR0;
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;
	
	float4 vPosition = v.vPos;

	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPosition( 
		g_bSkinning, 
		vPosition,
		v.vBoneWeights, v.vBoneIndices,
		worldPos );

	// Transform into projection space
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = vProjPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );
	o.fogFactorW.w = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW.w;
#endif	
	// Needed for water fog alpha; 
	// FIXME: we shouldn't have to compute this all thie time.
	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );

	// Base texture coordinates
	o.baseTexCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );
	o.baseTexCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );

	// Base texture coordinates
	o.baseTexCoord2.x = dot( v.vTexCoord0, cBaseTexCoordTransform2[0] );
	o.baseTexCoord2.y = dot( v.vTexCoord0, cBaseTexCoordTransform2[1] );
	
	o.vColor = cModulationColor;

	return o;
}
