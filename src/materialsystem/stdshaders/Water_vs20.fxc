// STATIC: "BASETEXTURE"				"0..1"
// STATIC: "MULTITEXTURE"				"0..1"

// SKIP: $MULTITEXTURE && $BASETEXTURE

#include "common_vs_fxc.h"

const float4 cBumpTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );
const float4 TexOffsets	:  register( SHADER_SPECIFIC_CONST_3 );

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
#if !defined( _X360 )
	float  vFog						: FOG;
#endif
	float2 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentEyeVect			: TEXCOORD1;
	float4 vReflectXY_vRefractYX	: TEXCOORD2;
	float4 vWorldPos_projPosW		: TEXCOORD3;
	float4 vProjPos					: TEXCOORD4;
	float  screenCoord				: TEXCOORD5;
#if MULTITEXTURE
	float4 vExtraBumpTexCoord       : TEXCOORD6;
#endif
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
	vReflectPos = (vProjPos.xy + vProjPos.w) * 0.5f;

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPos.x;
	vRefractPos.y = -vProjPos.y; // invert Y
	vRefractPos = (vRefractPos + vProjPos.w) * 0.5f;

	// Reflection transform
	o.vReflectXY_vRefractYX = float4( vReflectPos.x, vReflectPos.y, vRefractPos.y, vRefractPos.x );
	o.vWorldPos_projPosW.w = vProjPos.w;
	
	o.screenCoord = vProjPos.x;

	// Compute fog based on the position
	float3 vWorldPos = mul( v.vPos, cModel[0] );
	o.fogFactorW = CalcFog( vWorldPos, vProjPos, FOGTYPE_RANGE );
#if !defined( _X360 )
	o.vFog = o.fogFactorW;
#endif
	o.vWorldPos_projPosW.xyz = vWorldPos;

	// Eye vector
	float3 vWorldEyeVect = cEyePos - vWorldPos;
	// Transform to the tangent space
	o.vTangentEyeVect.x = dot( vWorldEyeVect, v.vTangentS );
	o.vTangentEyeVect.y = dot( vWorldEyeVect, v.vTangentT );
	o.vTangentEyeVect.z = dot( vWorldEyeVect, vObjNormal );

	// Tranform bump coordinates
	o.vBumpTexCoord.x = dot( v.vBaseTexCoord, cBumpTexCoordTransform[0] );
	o.vBumpTexCoord.y = dot( v.vBaseTexCoord, cBumpTexCoordTransform[1] );
	float f45x=v.vBaseTexCoord.x+v.vBaseTexCoord.y;
	float f45y=v.vBaseTexCoord.y-v.vBaseTexCoord.x;
#if MULTITEXTURE
	o.vExtraBumpTexCoord.x=f45x*0.1+TexOffsets.x;
	o.vExtraBumpTexCoord.y=f45y*0.1+TexOffsets.y;
	o.vExtraBumpTexCoord.z=v.vBaseTexCoord.y*0.45+TexOffsets.z;
	o.vExtraBumpTexCoord.w=v.vBaseTexCoord.x*0.45+TexOffsets.w;
#endif

#if BASETEXTURE
	o.lightmapTexCoord1And2.xy = v.vLightmapTexCoord + v.vLightmapTexCoordOffset;

	float2 lightmapTexCoord2 = o.lightmapTexCoord1And2.xy + v.vLightmapTexCoordOffset;
	float2 lightmapTexCoord3 = lightmapTexCoord2 + v.vLightmapTexCoordOffset;

	// reversed component order
	o.lightmapTexCoord1And2.w = lightmapTexCoord2.x;
	o.lightmapTexCoord1And2.z = lightmapTexCoord2.y;

	o.lightmapTexCoord3.xy = lightmapTexCoord3;
#endif

	return o;
}


