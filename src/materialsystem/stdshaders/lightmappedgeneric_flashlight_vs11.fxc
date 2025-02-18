//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

//	STATIC: "NORMALMAP"				"0..1"
//	STATIC: "WORLDVERTEXTRANSITION"	"0..1"
//	STATIC: "VERTEXCOLOR"			"0..1"
//	DYNAMIC: "DOWATERFOG"			"0..1"

#include "common_vs_fxc.h"

const float3 g_FlashlightPos					: register( SHADER_SPECIFIC_CONST_0 );
const float4x4 g_FlashlightWorldToTexture		: register( SHADER_SPECIFIC_CONST_1 );
const float4 g_FlashlightAttenuationFactors		: register( SHADER_SPECIFIC_CONST_5 );

const float4 cBaseTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_6 );
const float4 cNormalMapTexCoordTransform[2]		:  register( SHADER_SPECIFIC_CONST_8 );

static const int g_FogType					= DOWATERFOG;

struct VS_INPUT
{
	// If this is float4, and the input is float3, the w component default to one.
	float4 vPos							: POSITION; 
	float3 vNormal						: NORMAL;
	float2 vBaseTexCoord				: TEXCOORD0;
#if NORMALMAP
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL;
#endif
	float4 vColor						: COLOR0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float4 spotTexCoord				: TEXCOORD0;
	float2 baseTexCoord				: TEXCOORD1;
#if NORMALMAP
	float3 tangentPosToLightVector	: TEXCOORD2;
	float2 normalMapTexCoord		: TEXCOORD3;
#else
	float3 worldPosToLightVector	: TEXCOORD2;
	float3 normal					: TEXCOORD3;
#endif
	float4 vertAtten				: COLOR0;
};

float RemapValClamped( float val, float A, float B )
{
	float cVal = (val - A) / (B - A);
	cVal = saturate( cVal );
	return cVal;
}

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	float4 projPos;
	float3 worldPos;
	float3 worldNormal;
	float3 eyeVector;

	projPos = mul( v.vPos, cModelViewProj );
	o.projPos = projPos;

	worldPos = mul( v.vPos, cModel[0] );
	worldNormal = mul( v.vNormal, ( float3x3 )cModel[0] );

#if NORMALMAP
	float3 worldTangentS = mul( v.vTangentS, cModel[0] );
	float3 worldTangentT = mul( v.vTangentT, cModel[0] );
#endif

#if !defined( _X360 )
	o.fog = CalcFog( worldPos, projPos, g_FogType );
#endif

	o.baseTexCoord.x = dot( v.vBaseTexCoord, cBaseTexCoordTransform[0] ) + cBaseTexCoordTransform[0].w;
	o.baseTexCoord.y = dot( v.vBaseTexCoord, cBaseTexCoordTransform[1] ) + cBaseTexCoordTransform[1].w;

	float4 spotTexCoord = mul( float4( worldPos, 1.0f ), g_FlashlightWorldToTexture );
	o.spotTexCoord = spotTexCoord.xyzw;

	float3 worldPosToLightVector = g_FlashlightPos - worldPos;
#if NORMALMAP
	o.normalMapTexCoord.x = dot( v.vBaseTexCoord, cNormalMapTexCoordTransform[0] ) + cNormalMapTexCoordTransform[0].w;
	o.normalMapTexCoord.y = dot( v.vBaseTexCoord, cNormalMapTexCoordTransform[1] ) + cNormalMapTexCoordTransform[1].w;

	o.tangentPosToLightVector.x = dot( worldPosToLightVector, worldTangentS );
	o.tangentPosToLightVector.y = dot( worldPosToLightVector, worldTangentT );
	o.tangentPosToLightVector.z = dot( worldPosToLightVector, worldNormal );
#else
	o.worldPosToLightVector = worldPosToLightVector;
	o.normal = worldNormal;
#endif
	
	float3 delta = worldPosToLightVector;
	float distSquared = dot( delta, delta );
	float dist = sqrt( distSquared );
	float farZ = g_FlashlightAttenuationFactors.w;
	float endFalloffFactor = RemapValClamped( dist, farZ, 0.6 * farZ );
	o.vertAtten.xyz = saturate( endFalloffFactor * dot( g_FlashlightAttenuationFactors, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) ) );
	
#if WORLDVERTEXTRANSITION
	o.vertAtten.w = 1 - v.vColor.w;
#else
#if VERTEXCOLOR
	o.vertAtten.w = v.vColor.w;
#else
	o.vertAtten.w = 1.0f;
#endif
#endif
		
	return o;
}
