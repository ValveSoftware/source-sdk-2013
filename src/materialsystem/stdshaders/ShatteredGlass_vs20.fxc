//	DYNAMIC: "DOWATERFOG"				"0..1"
//	STATIC: "ENVMAP_MASK"			"0..1"
#include "common_vs_fxc.h"

static const int g_FogType					= DOWATERFOG;
static const bool g_UseSeparateEnvmapMask	= ENVMAP_MASK;

const float4 cBaseTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cDetailTexCoordTransform[2]		:  register( SHADER_SPECIFIC_CONST_2 );

struct VS_INPUT
{
	float3 vPos							: POSITION;
	float4 vNormal						: NORMAL;
	float2 vBaseTexCoord				: TEXCOORD0;
	float2 vLightmapTexCoord			: TEXCOORD1;
	float2 vDetailTexCoord				: TEXCOORD2;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float2 baseTexCoord				: TEXCOORD0;
	float2 detailTexCoord			: TEXCOORD1;
	float2 lightmapTexCoord			: TEXCOORD2;
	float2 envmapMaskTexCoord		: TEXCOORD3;
	float4 worldPos_projPosZ		: TEXCOORD4;
	float3 worldNormal				: TEXCOORD5;
	float4 vertexColor				: COLOR;
	float4 fogFactorW				: COLOR1;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	float4 projPos;
	projPos = mul( float4( v.vPos, 1 ), cModelViewProj );
	o.projPos = projPos;
	projPos.z = dot( float4( v.vPos, 1 ), cModelViewProjZ );
	
	o.worldPos_projPosZ.w = projPos.z;
	o.worldPos_projPosZ.xyz = mul( float4( v.vPos, 1 ), cModel[0] );
	o.worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
	o.baseTexCoord.x = dot( v.vBaseTexCoord, cBaseTexCoordTransform[0] ) + cBaseTexCoordTransform[0].w;
	o.baseTexCoord.y = dot( v.vBaseTexCoord, cBaseTexCoordTransform[1] ) + cBaseTexCoordTransform[1].w;
	o.detailTexCoord.x = dot( v.vDetailTexCoord, cDetailTexCoordTransform[0] ) + cDetailTexCoordTransform[0].w;
	o.detailTexCoord.y = dot( v.vDetailTexCoord, cDetailTexCoordTransform[1] ) + cDetailTexCoordTransform[1].w;
	o.envmapMaskTexCoord.x = dot( v.vDetailTexCoord, cDetailTexCoordTransform[0] ) + cDetailTexCoordTransform[0].w;
	o.envmapMaskTexCoord.y = dot( v.vDetailTexCoord, cDetailTexCoordTransform[1] ) + cDetailTexCoordTransform[1].w;
	o.lightmapTexCoord = v.vLightmapTexCoord;
	
	

	o.fogFactorW = CalcFog( o.worldPos_projPosZ.xyz, projPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW;
#endif	
	o.vertexColor = cModulationColor;

	return o;
}


