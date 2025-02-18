//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// DYNAMIC: "COMPRESSED_VERTS"			"0..1"
// DYNAMIC: "SKINNING"					"0..1"

// Includes
#include "common_vs_fxc.h"

// Globals
static const bool g_bSkinning = SKINNING ? true : false;

const float3 g_vTime : register( SHADER_SPECIFIC_CONST_0 );
#define g_flTime1x g_vTime.x
#define g_flTime2x g_vTime.y
#define g_flTime4x g_vTime.z

const float4 cBaseTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );

// Structs
struct VS_INPUT
{
	float4 vPos					: POSITION;		// Position
	float4 vNormal				: NORMAL;		// Normal
	float4 vBoneWeights			: BLENDWEIGHT;	// Skin weights
	float4 vBoneIndices			: BLENDINDICES;	// Skin indices
	float4 vTexCoord0			: TEXCOORD0;	// Base texture coordinates
	float4 vUserData			: TANGENT;
};

struct VS_OUTPUT
{
    float4 vProjPosition			: POSITION; // Projection-space position
	float4 v2DTangentViewVector01		: TEXCOORD0;
	float4 vUv01						: TEXCOORD1;
	float4 v2DTangentViewVector2_vUv2	: TEXCOORD2;
};

// Main
VS_OUTPUT main( const VS_INPUT i )
{
	VS_OUTPUT o;

	// Decompress compressed normal and tangent
	float4 vObjPosition = i.vPos.xyzw;
	float3 vObjNormal;
	float4 vObjTangent;
	DecompressVertex_NormalTangent( i.vNormal, i.vUserData, vObjNormal, vObjTangent );

	// Transform the position
	float3 vWorldPosition = { 0.0f, 0.0f, 0.0f };
	float3 vWorldNormal = { 0.0f, 0.0f, 0.0f };
	float3 vWorldTangent = { 0.0f, 0.0f, 0.0f };
	float3 vWorldBinormal = { 0.0f, 0.0f, 0.0f };
	SkinPositionNormalAndTangentSpace( g_bSkinning, vObjPosition, vObjNormal.xyz, vObjTangent.xyzw, i.vBoneWeights, i.vBoneIndices, vWorldPosition, vWorldNormal, vWorldTangent, vWorldBinormal );
	vWorldNormal.xyz = normalize( vWorldNormal.xyz );
	vWorldTangent.xyz = normalize( vWorldTangent.xyz );
	vWorldBinormal.xyz = normalize( vWorldBinormal.xyz );

	// Transform into projection space
	float4 vProjPosition = mul( float4( vWorldPosition, 1.0f ), cViewProj );
	o.vProjPosition = vProjPosition;

	// View vector
	float3 vWorldViewVector = normalize( vWorldPosition.xyz - cEyePos.xyz );
	float3 vTangentViewVector = Vec3WorldToTangentNormalized( vWorldViewVector.xyz, vWorldNormal.xyz, vWorldTangent.xyz, vWorldBinormal.xyz );

	// Texture coordinates
	float4 mRotate;
	float2 vBaseUv = i.vTexCoord0.xy;

	// Inner layer
	mRotate.x = cos( g_flTime4x );
	mRotate.y = -sin( g_flTime4x );
	mRotate.z = -mRotate.y;
	mRotate.w = mRotate.x;
	o.vUv01.xy = ( vBaseUv.xy - 0.5f ) * 1.0f;
	o.vUv01.xy = float2( dot( o.vUv01.xy, mRotate.xy ), dot( o.vUv01.xy, mRotate.zw ) );
	o.vUv01.xy += 0.5f;
	o.v2DTangentViewVector01.xy = float2( dot( vTangentViewVector.xy, mRotate.xy ), dot( vTangentViewVector.xy, mRotate.zw ) );

	// Middle layer
	mRotate.x = cos( g_flTime2x );
	mRotate.y = -sin( g_flTime2x );
	mRotate.z = -mRotate.y;
	mRotate.w = mRotate.x;
	o.vUv01.wz = ( vBaseUv.xy - 0.5f ) * 1.0f;
	o.vUv01.wz = float2( dot( o.vUv01.wz, mRotate.xy ), dot( o.vUv01.wz, mRotate.zw ) );
	o.vUv01.wz += 0.5f;
	o.v2DTangentViewVector01.wz = float2( dot( vTangentViewVector.xy, mRotate.xy ), dot( vTangentViewVector.xy, mRotate.zw ) );

	// Outer layer
	mRotate.x = cos( g_flTime1x );
	mRotate.y = -sin( g_flTime1x );
	mRotate.z = -mRotate.y;
	mRotate.w = mRotate.x;
	float2 vUv2 = ( vBaseUv.xy - 0.5f ) * 1.0f;
	vUv2.xy = float2( dot( vUv2.xy, mRotate.xy ), dot( vUv2.xy, mRotate.zw ) );
	vUv2.xy += 0.5f;
	o.v2DTangentViewVector2_vUv2.wz = vUv2.xy;
	o.v2DTangentViewVector2_vUv2.xy = float2( dot( vTangentViewVector.xy, mRotate.xy ), dot( vTangentViewVector.xy, mRotate.zw ) );

	return o;
}
