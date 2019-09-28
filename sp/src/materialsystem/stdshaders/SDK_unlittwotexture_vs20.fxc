//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;
static const int  g_FogType			= DOWATERFOG;

const float4 cBaseTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_0 ); // 0 & 1
const float4 cBaseTexCoordTransform2[2]	:  register( SHADER_SPECIFIC_CONST_2 ); // 2 & 3

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
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	float  fog						: FOG;
#endif
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	HALF2 baseTexCoord2				: TEXCOORD1;		// Base texture coordinate
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for water fog dest alpha
	
	float4 vColor					: COLOR0;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;
	
	float4 vPosition = v.vPos;

	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPosition( g_bSkinning, vPosition, v.vBoneWeights, v.vBoneIndices, worldPos );

	// Transform into projection space
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	o.fog = CalcFixedFunctionFog( worldPos, g_FogType );
#endif

	// Needed for water fog alpha; 
	o.worldPos_projPosZ = float4( worldPos.xyz, o.projPos.z );	// FIXME: we shouldn't have to compute this all thie time.

	o.baseTexCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );		// Base texture coordinates
	o.baseTexCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );
	o.baseTexCoord2.x = dot( v.vTexCoord0, cBaseTexCoordTransform2[0] );	// Secondary texture coordinates
	o.baseTexCoord2.y = dot( v.vTexCoord0, cBaseTexCoordTransform2[1] );
	
	o.vColor = cModulationColor;

	return o;
}
