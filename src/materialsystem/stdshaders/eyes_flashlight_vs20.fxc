// ------------------------------------------------------------------------------
// $cLight0Pos = world space light position
// $SHADER_SPECIFIC_CONST_1 = spotlight projection
// $SHADER_SPECIFIC_CONST_2 = spotlight projection
// $SHADER_SPECIFIC_CONST_3 = spotlight projection
// $SHADER_SPECIFIC_CONST_4 = spotlight projection
// $SHADER_SPECIFIC_CONST_5 = far z
// $SHADER_SPECIFIC_CONST_6 = eyeball origin
// $SHADER_SPECIFIC_CONST_7 = eyeball up * 0.5
// $SHADER_SPECIFIC_CONST_8 = iris projection U
// $SHADER_SPECIFIC_CONST_9 = iris projection V
// ------------------------------------------------------------------------------

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"
//  DYNAMIC: "MORPHING"				"0..1" [vs30]

#include "common_vs_fxc.h"

static const bool g_bSkinning	= SKINNING ? true : false;
static const int g_FogType		= DOWATERFOG;

const float4 cLightPosition		:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cSpotlightProj1	:  register( SHADER_SPECIFIC_CONST_1 );
const float4 cSpotlightProj2	:  register( SHADER_SPECIFIC_CONST_2 );
const float4 cSpotlightProj3	:  register( SHADER_SPECIFIC_CONST_3 );
const float4 cSpotlightProj4	:  register( SHADER_SPECIFIC_CONST_4 );
const float4 cFlashlighAtten	:  register( SHADER_SPECIFIC_CONST_5 ); // const, linear, quadratic & farZ
const float4 cIrisProjectionU	:  register( SHADER_SPECIFIC_CONST_8 );
const float4 cIrisProjectionV	:  register( SHADER_SPECIFIC_CONST_9 );

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim : register( SHADER_SPECIFIC_CONST_10 );
const float4 cMorphSubrect		: register( SHADER_SPECIFIC_CONST_11 );

sampler2D morphSampler			: register( D3DVERTEXTEXTURESAMPLER0, s0 );
#endif

struct VS_INPUT
{
	float4 vPos					: POSITION;			// Position
	float4 vBoneWeights			: BLENDWEIGHT;		// Skin weights
	float4 vBoneIndices			: BLENDINDICES;		// Skin indices
	float4 vNormal				: NORMAL;
	float4 vTexCoord0			: TEXCOORD0;		// Base (sclera) texture coordinates

	// Position and normal/tangent deltas
	float3 vPosFlex					: POSITION1;
	float3 vNormalFlex				: NORMAL1;
#ifdef SHADER_MODEL_VS_3_0
	float vVertexID					: POSITION2;
#endif
};

struct VS_OUTPUT
{
    float4 projPos				: POSITION;			// Projection-space position
#if !defined( _X360 )
	float  fog					: FOG;				// Fixed-function fog factor
#endif
	float4 spotTexCoord			: TEXCOORD0;		// Spotlight texture coordinates
	float2 baseTexCoord			: TEXCOORD1;		// Base texture coordinates
	float2 irisTexCoord			: TEXCOORD3;		// Iris texture coordinates
	float3 vertAtten			: TEXCOORD4;		// vertex attenuation
	float3 worldPos				: TEXCOORD5;
	float3 projPosXYZ			: TEXCOORD7;
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

	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPositionAndNormal( 
		g_bSkinning, 
		vPosition, vNormal,
		v.vBoneWeights, v.vBoneIndices,
		worldPos, worldNormal );

	worldNormal = normalize( worldNormal );

	// Transform into projection space
	float4 projPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos    = projPos;
	o.projPosXYZ = projPos.xyz;
	o.worldPos   = worldPos.xyz;

#if !defined( _X360 )
	// Set fixed-function fog factor
	o.fog = CalcFog( worldPos, o.projPos, g_FogType );
#endif

	// Base texture coordinates
	o.baseTexCoord = v.vTexCoord0;

	// Spotlight texture coordinates
	o.spotTexCoord.x = dot( cSpotlightProj1, float4(worldPos, 1) );
	o.spotTexCoord.y = dot( cSpotlightProj2, float4(worldPos, 1) );
	o.spotTexCoord.z = dot( cSpotlightProj3, float4(worldPos, 1) );
	o.spotTexCoord.w = dot( cSpotlightProj4, float4(worldPos, 1) );

	// Compute vector to light
	float3 vWorldPosToLightVector = cLightPosition.xyz - worldPos;

	float3 vDistAtten = float3(1, 1, 1);
	vDistAtten.z = dot( vWorldPosToLightVector, vWorldPosToLightVector );		// distsquared
	vDistAtten.y = rsqrt( vDistAtten.z );										// 1 / dist

	float flDist = vDistAtten.z * vDistAtten.y;									// dist
	vDistAtten.z = 1.0f / vDistAtten.z;											// 1 / distsquared

	float fFarZ = cFlashlighAtten.w;

	float endFalloffFactor = RemapValClamped_01( flDist, fFarZ, 0.6 * fFarZ );
	o.vertAtten.xyz = endFalloffFactor * dot( vDistAtten, cFlashlighAtten.xyz );
	
	o.vertAtten *= dot( normalize( vWorldPosToLightVector ), worldNormal );

	o.irisTexCoord.x  = dot( cIrisProjectionU,  float4(worldPos, 1) );
	o.irisTexCoord.y  = dot( cIrisProjectionV,  float4(worldPos, 1) );

	return o;
}