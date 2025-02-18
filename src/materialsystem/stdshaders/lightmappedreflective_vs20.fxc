// STATIC: "BASETEXTURE"				"0..1"

#include "common_vs_fxc.h"

const float4 cBumpTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );
const float4 cBaseTextureTransform[2]	:  register( SHADER_SPECIFIC_CONST_3 );

struct VS_INPUT
{
	float4 vPos							: POSITION;
	float4 vNormal						: NORMAL;
	float4 vBaseTexCoord				: TEXCOORD0;
	float2 vLightmapTexCoord			: TEXCOORD1;
	float2 vLightmapTexCoordOffset		: TEXCOORD2;
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL0;
};

struct VS_OUTPUT
{
    float4 vProjPos_POSITION		: POSITION;	
	float  vFog						: FOG;
	float4 vBumpTexCoordXY_vTexCoordXY : TEXCOORD0;
	float3 vTangentEyeVect			: TEXCOORD1;
	float4 vReflectXY_vRefractYX	: TEXCOORD2;
	float  W						: TEXCOORD3;
	float4 vProjPos					: TEXCOORD4;
	float  screenCoord				: TEXCOORD5;
#if BASETEXTURE
	HALF4 lightmapTexCoord1And2		: TEXCOORD6;
	HALF4 lightmapTexCoord3			: TEXCOORD7;
#endif
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	// Projected position
	float4 vProjPos = mul( v.vPos, cModelViewProj );
	o.vProjPos = o.vProjPos_POSITION = vProjPos;

	// Project tangent basis
	float2 vProjTangentS = mul( v.vTangentS, cViewProj );
	float2 vProjTangentT = mul( v.vTangentT, cViewProj );

	// Map projected position to the reflection texture
	float2 vReflectPos;
	vReflectPos.x = -vProjPos.x;
	vReflectPos.y = -vProjPos.y; // invert Y
	vReflectPos = (vReflectPos + vProjPos.w) * 0.5f;

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPos.x;
	vRefractPos.y = -vProjPos.y; // invert Y
	vRefractPos = (vRefractPos + vProjPos.w) * 0.5f;

	// Reflection transform
	o.vReflectXY_vRefractYX = float4( vReflectPos.x, vReflectPos.y, vRefractPos.y, vRefractPos.x );
	o.W = vProjPos.w;
	
	o.screenCoord = vProjPos.x;

	// Compute fog based on the position
	float3 vWorldPos = mul( v.vPos, cModel[0] );
	o.fogFactorW = o.vFog = CalcFog( vWorldPos, vProjPos, FOGTYPE_RANGE );

	// Eye vector
	float3 vWorldEyeVect = cEyePos - vWorldPos;
	// Transform to the tangent space
	o.vTangentEyeVect.x = dot( vWorldEyeVect, v.vTangentS );
	o.vTangentEyeVect.y = dot( vWorldEyeVect, v.vTangentT );
	o.vTangentEyeVect.z = dot( vWorldEyeVect, vObjNormal );

	// Tranform bump coordinates
	o.vBumpTexCoordXY_vTexCoordXY.x = dot( v.vBaseTexCoord, cBumpTexCoordTransform[0] );
	o.vBumpTexCoordXY_vTexCoordXY.y = dot( v.vBaseTexCoord, cBumpTexCoordTransform[1] );

#if BASETEXTURE
	o.vBumpTexCoordXY_vTexCoordXY.z = dot( v.vBaseTexCoord, cBaseTextureTransform[0] );
	o.vBumpTexCoordXY_vTexCoordXY.w = dot( v.vBaseTexCoord, cBaseTextureTransform[1] );

	o.lightmapTexCoord1And2.xy = v.vLightmapTexCoord + v.vLightmapTexCoordOffset;

	float2 lightmapTexCoord2 = o.lightmapTexCoord1And2.xy + v.vLightmapTexCoordOffset;
	float2 lightmapTexCoord3 = lightmapTexCoord2 + v.vLightmapTexCoordOffset;

	// reversed component order
	o.lightmapTexCoord1And2.w = lightmapTexCoord2.x;
	o.lightmapTexCoord1And2.z = lightmapTexCoord2.y;

	o.lightmapTexCoord3.xy = lightmapTexCoord3;
#else
	o.vBumpTexCoordXY_vTexCoordXY.z = 0.0f;
	o.vBumpTexCoordXY_vTexCoordXY.w = 0.0f;
#endif

	return o;
}


