//======= Copyright © 1996-2007, Valve Corporation, All rights reserved. ======

//	STATIC: "INTRO"					"0..1"

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"
//	DYNAMIC: "SKINNING"				"0..1"
//  DYNAMIC: "MORPHING"				"0..1" [vs30]

#include "vortwarp_vs20_helper.h"

static const int g_FogType			= DOWATERFOG;
static const bool g_bSkinning		= SKINNING ? true : false;

const float4 cFlashlightPosition	:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cSpotlightProj1		:  register( SHADER_SPECIFIC_CONST_1 );
const float4 cSpotlightProj2		:  register( SHADER_SPECIFIC_CONST_2 );
const float4 cSpotlightProj3		:  register( SHADER_SPECIFIC_CONST_3 );
const float4 cSpotlightProj4		:  register( SHADER_SPECIFIC_CONST_4 );
const float4 cFlashlighAtten		:  register( SHADER_SPECIFIC_CONST_5 ); // const, linear, quadratic & farZ

const float4 cTeethLighting			:  register( SHADER_SPECIFIC_CONST_8 );
#if INTRO
const float4 const4					:  register( SHADER_SPECIFIC_CONST_9 );
#define g_Time const4.w
#define modelOrigin const4.xyz
#endif

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim	: register( SHADER_SPECIFIC_CONST_6 );
const float4 cMorphSubrect			: register( SHADER_SPECIFIC_CONST_7 );

sampler2D morphSampler				: register( D3DVERTEXTEXTURESAMPLER0, s0 );
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
	float2 baseTexCoord		: TEXCOORD0;		// Base texture coordinates
	float4 spotTexCoord		: TEXCOORD1;		// Spotlight texture coordinates
	float3 vertAtten		: TEXCOORD2;		// Distance/spot attenuation
	float4 vProjPos			: TEXCOORD3;		// Projective space position
	float3 worldPos			: TEXCOORD4;		// Necessary for pixel fog
};


float RemapValClamped_01( float val, float A, float B )
{
	float cVal = (val - A) / (B - A);
	cVal = saturate( cVal );
	return cVal;
}

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

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
	o.projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.worldPos = worldPos.xyz;
	o.vProjPos = o.projPos;
#if !defined( _X360 )
	// Set fixed-function fog factor
	o.fog = CalcFog( worldPos, o.projPos, g_FogType );
#endif
	// Spotlight texture coordinates
	o.spotTexCoord.x = dot( cSpotlightProj1, float4(worldPos, 1) );
	o.spotTexCoord.y = dot( cSpotlightProj2, float4(worldPos, 1) );
	o.spotTexCoord.z = dot( cSpotlightProj3, float4(worldPos, 1) );
	o.spotTexCoord.w = dot( cSpotlightProj4, float4(worldPos, 1) );

	// Compute vector to light
	float3 vWorldPosToLightVector = cFlashlightPosition.xyz - worldPos;

	float3 vDistAtten = float3(1, 1, 1);
	vDistAtten.z = dot( vWorldPosToLightVector, vWorldPosToLightVector );
	vDistAtten.y = rsqrt( vDistAtten.z );

	float flDist = vDistAtten.z * vDistAtten.y;	// Distance to light
	vDistAtten.z = 1.0f / vDistAtten.z;			// 1 / distsquared

	float fFarZ = cFlashlighAtten.w;

	float NdotL = saturate( dot( worldNormal, normalize( vWorldPosToLightVector ) ) );

	float endFalloffFactor = RemapValClamped_01( flDist, fFarZ, 0.6 * fFarZ );
	o.vertAtten.xyz = endFalloffFactor * dot( vDistAtten, cFlashlighAtten.xyz );

	// Final attenuation from flashlight only...
	float linearAtten = NdotL * dot( vDistAtten, cFlashlighAtten.xyz ) * endFalloffFactor;

	// Forward vector
	float3 vForward = cTeethLighting.xyz;
	float fIllumFactor = cTeethLighting.w;

	// Modulate flashlight by mouth darkening
	o.vertAtten = linearAtten * fIllumFactor * saturate( dot( worldNormal, vForward ) );

	o.baseTexCoord = v.vTexCoord0;

	return o;
}


