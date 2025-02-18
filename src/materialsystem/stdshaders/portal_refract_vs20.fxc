//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// STATIC: "STAGE" "0..2"

// Includes
#include "common_vs_fxc.h"

// Globals
const float g_flTime : register( SHADER_SPECIFIC_CONST_0 );
const float4 cBaseTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );

const float2 g_vConst3 : register( SHADER_SPECIFIC_CONST_3 );
#define g_flPortalOpenAmount g_vConst3.x
#define g_flPortalStaticAmount g_vConst3.y

// Structs
struct VS_INPUT
{
	float4 vPos					: POSITION;		// Position
	float4 vNormal				: NORMAL;		// Normal
	float4 vTexCoord0			: TEXCOORD0;	// Base texture coordinates
	float4 vTangent				: TANGENT;		// Flip in w
};

struct VS_OUTPUT
{
	float4 vProjPosition		: POSITION;  // Projection-space position
	float2 vUv0					: TEXCOORD0;
	float3 vWorldTangent		: TEXCOORD1;
	float3 vWorldBinormal		: TEXCOORD2;
	float4 vWorldPosition		: TEXCOORD3; // Proj pos z in w
	float3 vProjPosForRefract	: TEXCOORD4;
	float4 vNoiseTexCoord		: TEXCOORD5;
};

// Main
VS_OUTPUT main( const VS_INPUT i )
{
	float kFlPortalOuterBorder = 0.075f; // Must match PS!

	VS_OUTPUT o;

	float3 vObjNormal;
	DecompressVertex_Normal( i.vNormal, vObjNormal );

	// Transform the position
	float3 vWorldPosition = mul( i.vPos, cModel[0] );
	float3 vWorldNormal = normalize( mul( vObjNormal, ( float3x3 )cModel[0] ) );
	float3 vWorldTangent = normalize( mul( i.vTangent, ( float3x3 )cModel[0] ) );
	float3 vWorldBinormal = normalize( cross( vWorldNormal, vWorldTangent ) * i.vTangent.w );

	o.vWorldPosition.xyz = vWorldPosition.xyz;
	//o.vWorldNormal.xyz = vWorldNormal.xyz;
	o.vWorldTangent.xyz = vWorldTangent.xyz + vWorldPosition.xyz;
	o.vWorldBinormal.xyz = vWorldBinormal.xyz + vWorldPosition.xyz;

	// Transform into projection space
	float4 vProjPosition = mul( float4( vWorldPosition, 1.0f ), cViewProj );
	o.vProjPosition.xyzw = vProjPosition.xyzw;
	o.vWorldPosition.w = vProjPosition.z;

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPosition.x;
	vRefractPos.y = -vProjPosition.y; // Invert Y
	vRefractPos = ( vRefractPos + vProjPosition.w ) * 0.5f;
	o.vProjPosForRefract.xyz = float3(vRefractPos.x, vRefractPos.y, vProjPosition.w);

	// View vector
	float3 vWorldViewVector = normalize (vWorldPosition.xyz - cEyePos.xyz);

	// Texture coordinates
	float2 vBaseUv;
	vBaseUv.x = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[0] );
	vBaseUv.y = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[1] );
	//o.vUv0.xy = vBaseUv.xy;
	o.vUv0.xy = vBaseUv.xy * ( 1.0f + kFlPortalOuterBorder ) - ( kFlPortalOuterBorder * 0.5f ); // Adjust uv's for shrunken portal

	// Portal open time
	float flPortalOpenAmount = saturate( g_flPortalOpenAmount + 0.001f ); // 0.001f to avoid divide by zero

	// Noise UV
	float kFlBorderNoiseScale = 0.3f;
	float kFlNoiseUvScroll = g_flTime * 0.0275f;
	float2 vNoiseUv = ( ( vBaseUv.xy - 0.5f ) / flPortalOpenAmount ) + 0.5f;
	o.vNoiseTexCoord.xy = ( vNoiseUv.xy * kFlBorderNoiseScale ) + float2( kFlNoiseUvScroll, 0 );
	o.vNoiseTexCoord.zw = ( vNoiseUv.xy * kFlBorderNoiseScale ) - float2( kFlNoiseUvScroll, 0 ); // Will fetch as wz to avoid matching layers

	return o;
}
