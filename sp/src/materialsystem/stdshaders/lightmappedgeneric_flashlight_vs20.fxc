//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

//	STATIC: "NORMALMAP"				"0..1"
//  STATIC: "WORLDVERTEXTRANSITION" "0..1"
//  STATIC: "SEAMLESS"				"0..1"
//  STATIC: "DETAIL"				"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"

#include "common_vs_fxc.h"

const float3 g_FlashlightPos							: register( SHADER_SPECIFIC_CONST_0 );
const float4x4 g_FlashlightWorldToTexture				: register( SHADER_SPECIFIC_CONST_1 );
const float4 g_FlashlightAttenuationFactors				: register( SHADER_SPECIFIC_CONST_5 );

#if SEAMLESS
const float4 SeamlessScale : register( SHADER_SPECIFIC_CONST_6 );
#define SEAMLESS_SCALE (SeamlessScale.x)
#endif
const float4 cBaseTexCoordTransform[2]					:  register( SHADER_SPECIFIC_CONST_6 );
const float4 cNormalMapOrDetailTexCoordTransform[2]		:  register( SHADER_SPECIFIC_CONST_8 );

static const int g_FogType					= DOWATERFOG;

struct VS_INPUT
{
	float3 vPos							: POSITION; //This HAS to match lightmappedgeneric_vs20.fxc's position input. Otherwise depth fighting errors occur on the 360
	float4 vNormal						: NORMAL;
	float2 vBaseTexCoord				: TEXCOORD0;
#if WORLDVERTEXTRANSITION
	float2 vLightmapTexCoord			: TEXCOORD1;
	float4 vColor						: COLOR0;
#endif
#if NORMALMAP
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL;
#endif
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;
#if !defined( _X360 )
	float  fog						: FOG;
#endif

	float4 spotTexCoord				: TEXCOORD0;

#if SEAMLESS
	float3 SeamlessTexCoord			: TEXCOORD1;
#else
	float2 baseTexCoord				: TEXCOORD1;
#endif

#if NORMALMAP
	float3 tangentPosToLightVector	: TEXCOORD2;
	float2 normalMapTexCoord		: TEXCOORD3;
#else
	float3 worldPosToLightVector	: TEXCOORD2;
	float3 normal					: TEXCOORD3;
#endif

	float2 detailCoords				: TEXCOORD4;
	float4 worldPos_worldTransition	: TEXCOORD5;	
	float3 vProjPos					: TEXCOORD6;
	float4 fogFactorW				: TEXCOORD7;
};

float RemapValClamped( float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = saturate( cVal );

	return C + (D - C) * cVal;
}


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	float4 projPos;
	float3 worldPos;
	float3 worldNormal;
	float3 eyeVector;

	//Projection math HAS to match lightmappedgeneric_vs20.fxc's math exactly. Otherwise depth fighting errors occur on the 360
	projPos = mul( float4( v.vPos, 1 ), cModelViewProj );
	o.projPos = projPos;
	o.vProjPos.xyz = projPos.xyw;

	worldPos = mul( float4( v.vPos, 1 ), cModel[0] );
	worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
	
	o.worldPos_worldTransition = float4( worldPos.xyz, 1.0f );

	o.fogFactorW = CalcFog( worldPos, projPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW.w;
#endif

#if NORMALMAP
	float3 worldTangentS = mul( v.vTangentS, cModel[0] );
	float3 worldTangentT = mul( v.vTangentT, cModel[0] );
#endif
#if SEAMLESS
	float3 vNormal=normalize( worldNormal );
	o.fogFactorW.xyz = vNormal * vNormal;           // sums to 1.
	o.SeamlessTexCoord = SEAMLESS_SCALE*worldPos;

	// Generate new tangent and binormal with seamless projection
	#if NORMALMAP
		// Brute-force for prototype - This must match the projection in the pixel shader!
		//float3 vVecX = { 1.0f, 0.0f, 0.0f };
		//float3 vVecY = { 0.0f, 1.0f, 0.0f };
		//float3 vVecZ = { 0.0f, 0.0f, 1.0f };
		//worldTangentS.xyz = normalize( ( o.fogFactorW.x * vVecZ.xyz ) + ( o.fogFactorW.y * vVecX.xyz ) + ( o.fogFactorW.z * vVecX.xyz ) );
  		//worldTangentT.xyz = normalize( ( o.fogFactorW.x * vVecY.xyz ) + ( o.fogFactorW.y * vVecZ.xyz ) + ( o.fogFactorW.z * vVecY.xyz ) );

		// Optimized version - This must match the projection in the pixel shader!
		worldTangentS.xyz = normalize( float3( o.fogFactorW.y + o.fogFactorW.z, 0.0f, o.fogFactorW.x ) );
		worldTangentT.xyz = normalize( float3( 0.0f, o.fogFactorW.x + o.fogFactorW.z, o.fogFactorW.y ) );
	#endif
#else
#if (SEAMLESS == 0 )
	o.baseTexCoord.x = dot( v.vBaseTexCoord, cBaseTexCoordTransform[0] ) + cBaseTexCoordTransform[0].w;
	o.baseTexCoord.y = dot( v.vBaseTexCoord, cBaseTexCoordTransform[1] ) + cBaseTexCoordTransform[1].w;
#endif
#endif

	float4 spotTexCoord = mul( float4( worldPos, 1.0f ), g_FlashlightWorldToTexture );
	o.spotTexCoord = spotTexCoord.xyzw;

	float3 worldPosToLightVector = g_FlashlightPos - worldPos;
#if NORMALMAP

#if (DETAIL == 0)
	o.normalMapTexCoord.x = dot( v.vBaseTexCoord, cNormalMapOrDetailTexCoordTransform[0] ) + cNormalMapOrDetailTexCoordTransform[0].w;
	o.normalMapTexCoord.y = dot( v.vBaseTexCoord, cNormalMapOrDetailTexCoordTransform[1] ) + cNormalMapOrDetailTexCoordTransform[1].w;
#else

#if SEAMLESS
	o.normalMapTexCoord = v.vBaseTexCoord;
#else
	o.normalMapTexCoord = o.baseTexCoord;
#endif

#endif

	o.tangentPosToLightVector.x = dot( worldPosToLightVector, worldTangentS );
	o.tangentPosToLightVector.y = dot( worldPosToLightVector, worldTangentT );
	o.tangentPosToLightVector.z = dot( worldPosToLightVector, worldNormal );
#else
	o.worldPosToLightVector = worldPosToLightVector;
	o.normal = worldNormal;
#endif

#if DETAIL
	o.detailCoords.x = dot( v.vBaseTexCoord, cNormalMapOrDetailTexCoordTransform[0] ) + cNormalMapOrDetailTexCoordTransform[0].w;
	o.detailCoords.y = dot( v.vBaseTexCoord, cNormalMapOrDetailTexCoordTransform[1] ) + cNormalMapOrDetailTexCoordTransform[1].w;
#else
	o.detailCoords = float2(0,0);
#endif

	//float3 delta = worldPosToLightVector;
	//float distSquared = dot( delta, delta );
	//float dist = sqrt( distSquared );
	//float farZ = g_FlashlightAttenuationFactors.w;
	//float endFalloffFactor = RemapValClamped( dist, farZ, 0.6f * farZ, 0.0f, 1.0f );
	//o.projPos_atten.w = endFalloffFactor * dot( g_FlashlightAttenuationFactors, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) );
	//o.projPos_atten.w = saturate( o.projPos_atten.w );

#if WORLDVERTEXTRANSITION
	o.worldPos_worldTransition.w = v.vColor.w;
#endif
	
	return o;
}
