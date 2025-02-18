//========= Copyright (c) 1996-2006, Valve Corporation, All rights reserved. ============//

// DYNAMIC: "COMPRESSED_VERTS"			"0..1"
// DYNAMIC: "SKINNING"					"0..1"

// Includes
#include "common_vs_fxc.h"

// Globals
static const bool g_bSkinning		= SKINNING ? true : false;
const float g_flTime : register( SHADER_SPECIFIC_CONST_0 );
const float4 cBaseTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );

// Structs
struct VS_INPUT
{
	float4 vPos					: POSITION;		// Position
	float4 vNormal				: NORMAL;		// Normal
	float4 vBoneWeights			: BLENDWEIGHT;	// Skin weights
	float4 vBoneIndices			: BLENDINDICES;	// Skin indices
	float4 vTexCoord0			: TEXCOORD0;	// Base texture coordinates
	float4 vTangent				: TANGENT;
};

struct VS_OUTPUT
{
    float4 vProjPosition		: POSITION;		// Projection-space position
	float3 vWorldNormal			: TEXCOORD0;	// World-space normal
	float3 vWorldTangent		: TEXCOORD1;
	float3 vWorldBinormal		: TEXCOORD2;
	float3 vProjPosForRefract	: TEXCOORD3;
	float3 vWorldViewVector		: TEXCOORD4;
	float4 vUv0					: TEXCOORD5;
	float4 vUv1					: TEXCOORD6;
	float2 vUvGroundNoise		: TEXCOORD7;
};

// Main
VS_OUTPUT main( const VS_INPUT i )
{
	VS_OUTPUT o;

	float4 vObjPosition = i.vPos;
	float4 vObjTangent = i.vTangent;
	float3 vObjNormal;
	DecompressVertex_Normal( i.vNormal, vObjNormal );

	// Transform the position
	float3 vWorldPosition = { 0.0f, 0.0f, 0.0f };
	float3 vWorldNormal = { 0.0f, 0.0f, 0.0f };
	float3 vWorldTangent = { 0.0f, 0.0f, 0.0f };
	float3 vWorldBinormal = { 0.0f, 0.0f, 0.0f };
	SkinPositionNormalAndTangentSpace( g_bSkinning, vObjPosition, vObjNormal.xyz, vObjTangent.xyzw, i.vBoneWeights, i.vBoneIndices, vWorldPosition, vWorldNormal, vWorldTangent, vWorldBinormal );
	vWorldNormal.xyz = normalize( vWorldNormal.xyz );
	vWorldTangent.xyz = normalize( vWorldTangent.xyz );
	vWorldBinormal.xyz = normalize( vWorldBinormal.xyz );

	o.vWorldNormal.xyz = vWorldNormal.xyz;
	o.vWorldTangent.xyz = vWorldTangent.xyz;
	o.vWorldBinormal.xyz = vWorldBinormal.xyz;

	// Transform into projection space
	float4 vProjPosition = mul( float4( vWorldPosition, 1.0f ), cViewProj );
	o.vProjPosition = vProjPosition;

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPosition.x;
	vRefractPos.y = -vProjPosition.y; // Invert Y
	vRefractPos = (vRefractPos + vProjPosition.w) * 0.5f;
	o.vProjPosForRefract.xyz = float3(vRefractPos.x, vRefractPos.y, vProjPosition.w);

	// View vector
	float3 vWorldViewVector = normalize (vWorldPosition.xyz - cEyePos.xyz);
	o.vWorldViewVector.xyz = vWorldViewVector.xyz;

	// Tangent space transform
	//o.mTangentSpaceTranspose[0] = float3( vWorldTangent.x, vWorldBinormal.x, vWorldNormal.x );
	//o.mTangentSpaceTranspose[1] = float3( vWorldTangent.y, vWorldBinormal.y, vWorldNormal.y );
	//o.mTangentSpaceTranspose[2] = float3( vWorldTangent.z, vWorldBinormal.z, vWorldNormal.z );

	// Texture coordinates
	float2 vBaseUv;
	vBaseUv.x = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[0] );
	vBaseUv.y = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[1] );

	// Bump layer 0
	float2 vUv0 = vBaseUv.xy;
	float2 vUv0Scroll = vBaseUv.xy * 3.0f;
	vUv0Scroll.y -= g_flTime * 0.1f;

	o.vUv0.xy = vUv0.xy;
	o.vUv0.wz = vUv0Scroll.xy;

	// Bump layer 1
	float2 vUv1 = vBaseUv.xy * 8.0f;
	float2 vUv1Scroll = vBaseUv.xy * 16.0f;
	vUv1Scroll.y -= g_flTime * 0.1f;
	
	o.vUv1.xy = vUv1.xy;
	o.vUv1.wz = vUv1Scroll.xy;

	// Ground noise
	o.vUvGroundNoise.xy = vBaseUv.xy;
	o.vUvGroundNoise.x *= 3.5f;
	o.vUvGroundNoise.y *= 0.2105f;
	o.vUvGroundNoise.y -= g_flTime * 0.04f;

	return o;
}
