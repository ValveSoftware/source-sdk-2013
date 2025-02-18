//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

//	STATIC: "INTRO"						"0..1"
//  STATIC: "USE_STATIC_CONTROL_FLOW"	"0..1" [vs20]

//	DYNAMIC: "COMPRESSED_VERTS"			"0..1"
//	DYNAMIC: "DOWATERFOG"				"0..1"
//	DYNAMIC: "SKINNING"					"0..1"
//	DYNAMIC: "DYNAMIC_LIGHT"			"0..1"
//	DYNAMIC: "STATIC_LIGHT"				"0..1"
//  DYNAMIC: "MORPHING"					"0..1" [vs30]
//  DYNAMIC: "NUM_LIGHTS"				"0..2" [vs20]

// If using static control flow on Direct3D, we should use the NUM_LIGHTS=0 combo
//  SKIP: $USE_STATIC_CONTROL_FLOW && ( $NUM_LIGHTS > 0 ) [vs20]

#include "vortwarp_vs20_helper.h"

static const int g_FogType			= DOWATERFOG;
static const bool g_bSkinning		= SKINNING ? true : false;

const float4 cTeethLighting			:  register( SHADER_SPECIFIC_CONST_0 );
#if INTRO
const float4 const4	:  register( SHADER_SPECIFIC_CONST_1 );
#define g_Time const4.w
#define modelOrigin const4.xyz
#endif

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim			: register( SHADER_SPECIFIC_CONST_6 );
const float4 cMorphSubrect					: register( SHADER_SPECIFIC_CONST_7 );

sampler2D morphSampler : register( D3DVERTEXTEXTURESAMPLER0, s0 );
#endif

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
	float4 vNormal			: NORMAL;
	float2 vTexCoord0		: TEXCOORD0;

	// Position and normal/tangent deltas
	float3 vPosFlex			: POSITION1;
	float3 vNormalFlex		: NORMAL1;
#ifdef SHADER_MODEL_VS_3_0
	float vVertexID			: POSITION2;
#endif
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	
#if !defined( _X360 )    
	float  fog				: FOG;
#endif
	float2 baseTexCoord		: TEXCOORD0;
	float3 vertAtten		: TEXCOORD1;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	bool bDynamicLight = DYNAMIC_LIGHT ? true : false;
	bool bStaticLight = STATIC_LIGHT ? true : false;

	float4 vPosition = v.vPos;
	float3 vNormal;
	DecompressVertex_Normal( v.vNormal, vNormal );

#if !defined( SHADER_MODEL_VS_3_0 ) || !MORPHING
	ApplyMorph( v.vPosFlex, v.vNormalFlex, vPosition.xyz, vNormal );
#else
	ApplyMorph( morphSampler, cMorphTargetTextureDim, cMorphSubrect, v.vVertexID, float3( 0, 0, 0 ), vPosition.xyz, vNormal );
#endif

	// Normalize the flexed normal
	vNormal.xyz = normalize( vNormal.xyz );

	// Transform the position
	float3 worldPos, worldNormal;
	SkinPositionAndNormal( g_bSkinning, vPosition, vNormal, v.vBoneWeights, v.vBoneIndices, worldPos, worldNormal );

#if INTRO
	float3 dummy = float3( 0.0f, 0.0f, 0.0f );
	WorldSpaceVertexProcess( g_Time, modelOrigin, worldPos, worldNormal, dummy, dummy );
#endif

	// Transform into projection space
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = vProjPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );

	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );
#if !defined( _X360 )
	// Set fixed-function fog factor
	o.fog = CalcFog( worldPos, vProjPos, g_FogType );
#endif

	// Compute lighting
#if ( USE_STATIC_CONTROL_FLOW ) || defined ( SHADER_MODEL_VS_3_0 )
	float3 linearColor = DoLighting( worldPos, worldNormal, float3(0.0f, 0.0f, 0.0f), bStaticLight, bDynamicLight, false );
#else
	float3 linearColor = DoLightingUnrolled( worldPos, worldNormal, float3(0.0f, 0.0f, 0.0f), bStaticLight, bDynamicLight, false, NUM_LIGHTS );
#endif

	// Forward vector
	float3 vForward = cTeethLighting.xyz;
	float fIllumFactor = cTeethLighting.w;

	// Darken by forward dot normal and illumination factor
	linearColor *= fIllumFactor * saturate( dot( worldNormal, vForward ) );	

	o.vertAtten = linearColor;
	o.baseTexCoord = v.vTexCoord0;

	return o;
}


