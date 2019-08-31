// based on vertexlit_and_unlit_generic_vs20.fxc
//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

//  STATIC: "VERTEXCOLOR"			"0..1"
//  STATIC: "LIGHTING_PREVIEW"		"0..1"	[PC]
//  STATIC: "LIGHTING_PREVIEW"		"0..0"	[XBOX]
//  STATIC: "FLASHLIGHT"			"0..1"  

//	DYNAMIC: "DOWATERFOG"			"0..1"
//  DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..0"  [vs20]
//	DYNAMIC: "SKINNING"				"0..1"  [vs30]

// DYNAMIC: "MORPHING"				"0..1" [vs30] [ = pShaderAPI->IsHWMorphingEnabled() ]

#include "common_vs_fxc.h"
 
static const bool g_bSkinning		= SKINNING ? true : false;
static const int  g_FogType			= DOWATERFOG;
static const bool g_bVertexColor	= VERTEXCOLOR ? true : false;
#if ( defined( SHADER_MODEL_VS_3_0 ) && MORPHING )
	#define DECALOFFSET 1
#else
	#define DECALOFFSET 0
#endif

const float4 cBaseTexCoordTransform[2]		: register( SHADER_SPECIFIC_CONST_0 );

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim			: register( SHADER_SPECIFIC_CONST_10 );
const float4 cMorphSubrect					: register( SHADER_SPECIFIC_CONST_11 );
sampler2D morphSampler						: register( D3DVERTEXTEXTURESAMPLER0, s0 );
#endif

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
	float4 vNormal			: NORMAL;
	float4 vColor			: COLOR0;
	float3 vSpecular		: COLOR1;
	// make these float2's and stick the [n n 0 1] in the dot math.
	float4 vTexCoord0		: TEXCOORD0;
	float4 vTexCoord1		: TEXCOORD1;
	float4 vTexCoord2		: TEXCOORD2;
	float4 vTexCoord3		: TEXCOORD3;

	// Position and normal/tangent deltas
	float3 vPosFlex			: POSITION1;
	float3 vNormalFlex		: NORMAL1;
#ifdef SHADER_MODEL_VS_3_0
	float vVertexID			: POSITION2;
#endif
};


struct VS_OUTPUT
{
    float4 projPos					: POSITION;			// Projection-space position	
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	float  fog						: FOG;
#endif

	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
	float4 worldPos_ProjPosZ		: TEXCOORD1;
	float4 color					: COLOR1;		// Vertex color (from lighting or unlit)
	
#if FLASHLIGHT
	float4 vProjPos					: TEXCOORD2;
#endif
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float4 vPosition = v.vPos;
	float3 vNormal = 0;
	if ( LIGHTING_PREVIEW || DECALOFFSET )
	{
		// The vertex only contains valid normals if they are actually needed (fetching them when absent makes D3D complain)
		DecompressVertex_Normal( v.vNormal, vNormal );
	}
	
#if !defined( SHADER_MODEL_VS_3_0 ) || !MORPHING
	ApplyMorph( v.vPosFlex, v.vNormalFlex, vPosition.xyz, vNormal );
#else
	ApplyMorph( morphSampler, cMorphTargetTextureDim, cMorphSubrect, 
		v.vVertexID, v.vTexCoord2, vPosition.xyz, vNormal );
#endif
	  
	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPositionAndNormal( 
		g_bSkinning, 
		vPosition, vNormal,
		v.vBoneWeights, v.vBoneIndices,
		worldPos, worldNormal );

	if ( !g_bVertexColor )
	{
		worldNormal = normalize( worldNormal );
	}

#if defined( SHADER_MODEL_VS_3_0 ) && MORPHING
	// Avoid z precision errors
	worldPos += worldNormal * 0.05f * v.vTexCoord2.z;
#endif
	
	// Transform into projection space
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = projPos;
	
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	o.fog = CalcFixedFunctionFog( worldPos, g_FogType );
#endif

#if FLASHLIGHT
	// Transform into projection space
	projPos.z = dot( float4( worldPos, 1 ), cViewProjZ );

	o.vProjPos = projPos;
#endif

	o.worldPos_ProjPosZ.xyz = worldPos.xyz;
	o.worldPos_ProjPosZ.w = projPos.z;

	if ( g_bVertexColor )
	{
		// Assume that this is unlitgeneric if you are using vertex color.
		o.color.rgb = GammaToLinear( v.vColor.rgb );
		o.color.a = v.vColor.a;
	}
	else
	{
		o.color = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	}
	
	// Base texture coordinates
	o.baseTexCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );
	o.baseTexCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );

#if LIGHTING_PREVIEW
	float dot=0.5+0.5*worldNormal*float3(0.7071,0.7071,0);
	o.color.xyz=float3(dot,dot,dot);
#endif

	return o;
}


