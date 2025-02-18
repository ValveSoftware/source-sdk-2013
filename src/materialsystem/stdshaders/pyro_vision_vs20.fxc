// STATIC: "EFFECT"						"0..3"
// STATIC: "VERTEXCOLOR"				"0..1"
// STATIC: "HALFLAMBERT"				"0..1"
// STATIC: "VERTEX_LIT"					"0..1"
// STATIC: "FULLBRIGHT"					"0..1"
// STATIC: "USE_STATIC_CONTROL_FLOW"	"0..1" [vs20]
// STATIC: "BASETEXTURE2"				"0..1"
// STATIC: "STRIPES"					"0..1"
// STATIC: "STRIPES_USE_NORMAL2"		"0..1"

// DYNAMIC: "COMPRESSED_VERTS"			"0..1"
// DYNAMIC: "SKINNING"					"0..1"
// DYNAMIC: "DYNAMIC_LIGHT"				"0..1"
// DYNAMIC: "STATIC_LIGHT"				"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..2"	[vs20]

// SKIP: ( STRIPES == 0 ) &&  ( STRIPES_USE_NORMAL2 == 1 )
// SKIP: ( EFFECT == 2 || EFFECT == 3 ) && ( VERTEXCOLOR == 1 || HALFLAMBERT == 1 || VERTEX_LIT == 1 || FULLBRIGHT == 1 || USE_STATIC_CONTROL_FLOW == 1 || BASETEXTURE2 == 1 || STRIPES == 1 || STRIPES_USE_NORMAL2 == 1 )
// SKIP: ( EFFECT == 2 || EFFECT == 3 ) && ( SKINNING == 1 || DYNAMIC_LIGHT == 1 || STATIC_LIGHT == 1 || NUM_LIGHTS != 0 )
// SKIP: ( FULLBRIGHT == 1 ) &&  ( VERTEX_LIT == 0 || DYNAMIC_LIGHT == 1 || STATIC_LIGHT == 1 || NUM_LIGHTS != 0 )


#include "common_vs_fxc.h"


static const bool g_bSkinning		= SKINNING ? true : false;
static const bool g_bHalfLambert	= HALFLAMBERT ? true : false;


const float4 g_vPyroParms1		: register( SHADER_SPECIFIC_CONST_0 );
	#define g_vStripeScale			g_vPyroParms1.xyz
	#define g_flUnused1				g_vPyroParms1.w

#if EFFECT == 0 || EFFECT == 1

const float4 g_vPyroParms2		: register( SHADER_SPECIFIC_CONST_1 );
	#define g_vCanvasScale			g_vPyroParms2.xyz
	#define g_flUnused2				g_vPyroParms2.w

#endif

const float4 cBlendMaskTexCoordTransform[ 2 ]     :  register( SHADER_SPECIFIC_CONST_10 );	// not contiguous with the rest!

struct VS_INPUT
{
	float4 vPos							: POSITION;
	float4 vBoneWeights					: BLENDWEIGHT;
	float4 vBoneIndices					: BLENDINDICES;
#if ( VERTEX_LIT == 1 )
	float4 vNormal						: NORMAL;
#endif
	float2 vBaseTexCoord				: TEXCOORD0;
	float2 vLightmapTexCoord			: TEXCOORD1;
	float2 vLightmapTexCoordOffset		: TEXCOORD2;
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL;
	float4 vColor						: COLOR0;
	float3 vSpecular					: COLOR1;
};


struct VS_OUTPUT
{
    float4 vProjPos					: POSITION;	

#if EFFECT == 0 || EFFECT == 1

	float4 vBaseAndSeamlessTexCoord	: TEXCOORD0;
#if ( STRIPES == 1 )
	float2 vStripeSeamlessTexCoord	: TEXCOORD1;
#endif
#if ( VERTEX_LIT == 0 )
	float2 vLightmapBlendTexCoord	: TEXCOORD2;
#endif
#if BASETEXTURE2 == 1
	float3 vBlendFactor				: TEXCOORD3;
#endif
	float4 worldPos_projPosZ		: TEXCOORD4;
	float3 vWorldNormal				: TEXCOORD5;
	float4 vVertexColor				: COLOR;

#else

	float2 vBaseTexCoord			: TEXCOORD0;

#endif

};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT	o;

#if EFFECT == 0 || EFFECT == 1

	float4 vObjPosition = v.vPos;

#if ( VERTEX_LIT == 1 )
	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );
#else
	float3 vObjNormal = float3( 0.0, 0.0, 1.0 );
#endif

	float3 vWorldNormal, vWorldPos;
	SkinPositionAndNormal( g_bSkinning, vObjPosition, vObjNormal, v.vBoneWeights, v.vBoneIndices, vWorldPos, vWorldNormal );

	float4 vProjPos = mul( float4( vWorldPos, 1 ), cViewProj );
	o.vProjPos = vProjPos;
	vProjPos.z = dot( float4( vWorldPos, 1 ), cViewProjZ );
	
	o.worldPos_projPosZ = float4( vWorldPos, vProjPos.z );

#if 0
	if ( !g_bSkinning )
	{
		vWorldNormal = vObjNormal;
	}
#endif
	vWorldNormal = normalize( vWorldNormal );
	o.vWorldNormal = vWorldNormal;
	
	o.vBaseAndSeamlessTexCoord.xy = v.vBaseTexCoord;
	float flZFactor = vWorldPos.z * g_vCanvasScale.z;
	o.vBaseAndSeamlessTexCoord.zw = float2( vWorldPos.x + flZFactor, vWorldPos.y - flZFactor ) * g_vCanvasScale.xy;

#if ( STRIPES == 1 )

	float3 vWorldStripeCoord = vWorldPos * g_vStripeScale;
	o.vStripeSeamlessTexCoord.xy = vWorldStripeCoord.yz + vWorldStripeCoord.xz + vWorldStripeCoord.xy;

#endif


#if VERTEX_LIT == 0

	o.vLightmapBlendTexCoord.xy = v.vLightmapTexCoord;

#endif

#if BASETEXTURE2 == 1

	o.vBlendFactor.x = dot( v.vBaseTexCoord, cBlendMaskTexCoordTransform[ 0 ] ) + cBlendMaskTexCoordTransform[ 0 ].w;
	o.vBlendFactor.y = dot( v.vBaseTexCoord, cBlendMaskTexCoordTransform[ 1 ] ) + cBlendMaskTexCoordTransform[ 1 ].w;
	o.vBlendFactor.z = v.vColor.a;

#endif

#if VERTEXCOLOR == 1 

	o.vVertexColor = v.vColor;

#elif VERTEX_LIT == 1 && FULLBRIGHT == 0

	bool bDynamicLight = DYNAMIC_LIGHT ? true : false;
	bool bStaticLight = STATIC_LIGHT ? true : false;

	#if ( ( USE_STATIC_CONTROL_FLOW ) ||  defined ( SHADER_MODEL_VS_3_0 ) )
	{
		o.vVertexColor.xyz = DoLighting( vWorldPos, vWorldNormal, v.vSpecular, bStaticLight, bDynamicLight, g_bHalfLambert );
	}
	#else
	{
		o.vVertexColor.xyz = DoLightingUnrolled( vWorldPos, vWorldNormal, v.vSpecular, bStaticLight, bDynamicLight, g_bHalfLambert, NUM_LIGHTS );
	}
	#endif

	o.vVertexColor.a = 1.0f;

#else

	o.vVertexColor = 1.0f;

#endif

#else

	o.vProjPos = v.vPos;
	o.vBaseTexCoord = v.vBaseTexCoord;

#endif

	return o;
}
