//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

//  STATIC: "MODEL"					"0..1"
//  STATIC: "COLORMODULATE"         "0..1"

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;
static const bool g_bModel			= MODEL ? true : false;

const float4 cBumpTexCoordTransform[4]	:  register( SHADER_SPECIFIC_CONST_1 );

const float g_flTime : register( SHADER_SPECIFIC_CONST_5 );

struct VS_INPUT
{
	float4 vPos							: POSITION;
	float4 vBoneWeights					: BLENDWEIGHT;
	float4 vBoneIndices					: BLENDINDICES;
	float4 vNormal						: NORMAL;
	float4 vBaseTexCoord				: TEXCOORD0;
#if !MODEL
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL0;
#else
	float4 vUserData					: TANGENT;
#endif
#if COLORMODULATE
	float4 vColor                       : COLOR0;
#endif
};

struct VS_OUTPUT
{
    float4 vProjPos_POSITION		: POSITION;
#if !defined( _X360 )
	float  vFog						: FOG;
#endif
	float4 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentEyeVect			: TEXCOORD1;
	float3 vWorldNormal				: TEXCOORD2; 
	float3 vWorldTangent			: TEXCOORD3; 
	float3 vWorldBinormal			: TEXCOORD4; 
	float3 vRefractXYW				: TEXCOORD5;
	float3 vWorldViewVector			: TEXCOORD6;
#if COLORMODULATE
	float4 vColor                   : COLOR0;
#endif
	float4 fogFactorW				: COLOR1;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

#if COLORMODULATE
	o.vColor = v.vColor;
#endif

	float3 worldNormal, worldPos, worldTangentS, worldTangentT;

	float3 vObjNormal;
#if MODEL
	float4 vObjTangent;
	DecompressVertex_NormalTangent( v.vNormal, v.vUserData, vObjNormal, vObjTangent );

	SkinPositionNormalAndTangentSpace( 
			g_bSkinning, 
			v.vPos, vObjNormal, vObjTangent,
			v.vBoneWeights, v.vBoneIndices,
			worldPos, worldNormal, worldTangentS, worldTangentT );
#else
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	worldPos = mul( v.vPos, cModel[0] );
	worldTangentS = mul( v.vTangentS, ( const float3x3 )cModel[0] );
	worldTangentT = mul( v.vTangentT, ( const float3x3 )cModel[0] );
	worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
#endif

	// World normal
	o.vWorldNormal.xyz = normalize( worldNormal.xyz );

	// Projected position
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.vProjPos_POSITION = vProjPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );
	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );
	//o.projNormal.xyz = mul( worldNormal, cViewProj );

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPos.x;
	vRefractPos.y = -vProjPos.y; // invert Y
	vRefractPos = (vRefractPos + vProjPos.w) * 0.5f;

	// Refraction transform
	o.vRefractXYW = float3(vRefractPos.x, vRefractPos.y, vProjPos.w);

	// Compute fog based on the position
	float3 vWorldPos = mul( v.vPos, cModel[0] );
	o.fogFactorW = CalcFog( vWorldPos, vProjPos, FOGTYPE_RANGE );
#if !defined( _X360 )
	o.vFog = o.fogFactorW;
#endif

	// Eye vector
	float3 vWorldEyeVect = normalize( cEyePos - vWorldPos );
	o.vWorldViewVector.xyz = -vWorldEyeVect.xyz;

	// Transform to the tangent space
	o.vTangentEyeVect.x = dot( vWorldEyeVect, worldTangentS );
	o.vTangentEyeVect.y = dot( vWorldEyeVect, worldTangentT );
	o.vTangentEyeVect.z = dot( vWorldEyeVect, worldNormal );

	// Tranform bump coordinates
	o.vBumpTexCoord.x = dot( v.vBaseTexCoord, cBumpTexCoordTransform[0] );
	o.vBumpTexCoord.y = dot( v.vBaseTexCoord, cBumpTexCoordTransform[1] );

	// Tranform bump coordinates (note wz, not zw)
	o.vBumpTexCoord.w = dot( v.vBaseTexCoord, cBumpTexCoordTransform[2] );
	o.vBumpTexCoord.z = dot( v.vBaseTexCoord, cBumpTexCoordTransform[3] );


	// Tangent space transform
	o.vWorldNormal.xyz = normalize( worldNormal.xyz );
	o.vWorldTangent.xyz = worldTangentS.xyz;
	o.vWorldBinormal.xyz = worldTangentT.xyz;

	return o;
}
