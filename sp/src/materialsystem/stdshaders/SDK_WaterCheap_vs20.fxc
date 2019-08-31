// STATIC: "BLEND"		"0..1"
#include "common_vs_fxc.h"

struct VS_INPUT
{
	float4 vPos							: POSITION;
	float4 vNormal						: NORMAL;
	float2 vNormalMapCoord				: TEXCOORD0;
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float2 normalMapTexCoord		: TEXCOORD0;
	float3 worldVertToEyeVector		: TEXCOORD1;
	float3x3 tangentSpaceTranspose	: TEXCOORD2;
	float4 vRefract_W_ProjZ			: TEXCOORD5;
	float4 vExtraBumpTexCoord       : TEXCOORD6;
	float3 vWorldPos				: TEXCOORD7;
	float4 fogFactorW				: COLOR1;
};

const float4 cNormalMapTransform[2]	:  register( SHADER_SPECIFIC_CONST_0 );
const float4 TexOffsets	:  register( SHADER_SPECIFIC_CONST_3 );

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	float4 projPos;
	float3 worldPos;

	projPos = mul( v.vPos, cModelViewProj );
	o.projPos = projPos;

#if BLEND
	// Map projected position to the reflection texture
	o.vRefract_W_ProjZ.x = projPos.x;
	o.vRefract_W_ProjZ.y = -projPos.y; // invert Y
	o.vRefract_W_ProjZ.xy = (o.vRefract_W_ProjZ + projPos.w) * 0.5f;
	o.vRefract_W_ProjZ.z = projPos.w;
#endif
	
	o.vRefract_W_ProjZ.w = projPos.z;
	
	worldPos = mul( v.vPos, cModel[0] );

	float3 worldTangentS = mul( v.vTangentS, ( const float3x3 )cModel[0] );
	float3 worldTangentT = mul( v.vTangentT, ( const float3x3 )cModel[0] );
	float3 worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
	o.tangentSpaceTranspose[0] = worldTangentS;
	o.tangentSpaceTranspose[1] = worldTangentT;
	o.tangentSpaceTranspose[2] = worldNormal;

	float3 worldVertToEyeVector = VSHADER_VECT_SCALE * (cEyePos - worldPos);
	o.worldVertToEyeVector = worldVertToEyeVector;

	// FIXME: need to add a normalMapTransform to all of the water shaders.
	//o.normalMapTexCoord.x = dot( v.vNormalMapCoord, cNormalMapTransform[0] ) + cNormalMapTransform[0].w;
	//o.normalMapTexCoord.y = dot( v.vNormalMapCoord, cNormalMapTransform[1] ) + cNormalMapTransform[1].w;
	o.normalMapTexCoord = v.vNormalMapCoord;

	float f45x=v.vNormalMapCoord.x+v.vNormalMapCoord.y;
	float f45y=v.vNormalMapCoord.y-v.vNormalMapCoord.x;
	o.vExtraBumpTexCoord.x=f45x*0.1+TexOffsets.x;
	o.vExtraBumpTexCoord.y=f45y*0.1+TexOffsets.y;
	o.vExtraBumpTexCoord.z=v.vNormalMapCoord.y*0.45+TexOffsets.z;
	o.vExtraBumpTexCoord.w=v.vNormalMapCoord.x*0.45+TexOffsets.w;

	o.fogFactorW = CalcFog( worldPos, projPos, FOGTYPE_RANGE );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif
	o.vWorldPos = worldPos;

	return o;
}


