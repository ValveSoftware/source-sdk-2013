//	STATIC: "MODEL"					"0..1"

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;
static const bool g_bModel			= MODEL ? true : false;

const float4 cBumpTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );

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
};

struct VS_OUTPUT
{
    float4 vProjPos_POSITION		: POSITION;	
	float  vFog						: FOG;
	float2 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentEyeVect			: TEXCOORD1;
	float3x3 tangentSpaceTranspose	: TEXCOORD2;
	float3 vRefractXYW				: TEXCOORD5;
	float4 projNormal_screenCoordW	: TEXCOORD6;
	float4 worldPos_projPosZ		: TEXCOORD7;
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

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


	// Projected position
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.vProjPos_POSITION = vProjPos;
	o.projNormal_screenCoordW.xyz = mul( worldNormal, cViewProj );
	
	o.worldPos_projPosZ = float4( worldPos.xyz, vProjPos.z );

	// Map projected position to the refraction texture
	float2 vRefractPos;
	vRefractPos.x = vProjPos.x;
	vRefractPos.y = -vProjPos.y; // invert Y
	vRefractPos = (vRefractPos + vProjPos.w) * 0.5f;

	// Refraction transform
	o.vRefractXYW = float3(vRefractPos.x, vRefractPos.y, vProjPos.w);

	// Compute fog based on the position
	float3 vWorldPos = mul( v.vPos, cModel[0] );
	o.fogFactorW = o.vFog = CalcFog( vWorldPos, vProjPos, FOGTYPE_RANGE );

	// Eye vector
	float3 vWorldEyeVect = cEyePos - vWorldPos;
	// Transform to the tangent space
	o.vTangentEyeVect.x = dot( vWorldEyeVect, worldTangentS );
	o.vTangentEyeVect.y = dot( vWorldEyeVect, worldTangentT );
	o.vTangentEyeVect.z = dot( vWorldEyeVect, worldNormal );

	// Tranform bump coordinates
	o.vBumpTexCoord.x = dot( v.vBaseTexCoord, cBumpTexCoordTransform[0] );
	o.vBumpTexCoord.y = dot( v.vBaseTexCoord, cBumpTexCoordTransform[1] );

	o.tangentSpaceTranspose[0] = worldTangentS;
	o.tangentSpaceTranspose[1] = worldTangentT;
	o.tangentSpaceTranspose[2] = worldNormal;

	return o;
}
