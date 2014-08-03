//======= Copyright © 1996-2007, Valve Corporation, All rights reserved. ======

//  STATIC: "VERTEXCOLOR"				"0..1"
//	STATIC: "CUBEMAP"					"0..1"
//  STATIC: "HALFLAMBERT"				"0..1"
//  STATIC: "FLASHLIGHT"				"0..1"
//  STATIC: "SEAMLESS_BASE"         	"0..1"
//  STATIC: "SEAMLESS_DETAIL"       	"0..1"
//  STATIC: "SEPARATE_DETAIL_UVS"   	"0..1"
//  STATIC: "DECAL"						"0..1" [vs30]
//  STATIC: "USE_STATIC_CONTROL_FLOW"	"0..1" [vs20]
//  STATIC: "DONT_GAMMA_CONVERT_VERTEX_COLOR" "0..1"
//  DYNAMIC: "COMPRESSED_VERTS"			"0..1"
//	DYNAMIC: "DYNAMIC_LIGHT"			"0..1"
//	DYNAMIC: "STATIC_LIGHT"				"0..1"
//	DYNAMIC: "DOWATERFOG"				"0..1"
//	DYNAMIC: "SKINNING"					"0..1"
//  DYNAMIC: "LIGHTING_PREVIEW"			"0..1"	[PC]
//  DYNAMIC: "LIGHTING_PREVIEW"			"0..0"	[XBOX]
//  DYNAMIC: "MORPHING"					"0..1"  [vs30]
//  DYNAMIC: "NUM_LIGHTS"				"0..2"	[vs20]

// If using static control flow on Direct3D, we should use the NUM_LIGHTS=0 combo
// SKIP: $USE_STATIC_CONTROL_FLOW && ( $NUM_LIGHTS > 0 ) [vs20]
// SKIP: ($SEPARATE_DETAIL_UVS) && ($SEAMLESS_DETAIL)
// SKIP: ($DONT_GAMMA_CONVERT_VERTEX_COLOR && ( ! $VERTEXCOLOR ) )
#include "common_vs_fxc.h"
 
static const bool g_bSkinning		= SKINNING ? true : false;
static const int  g_FogType			= DOWATERFOG;
static const bool g_bVertexColor	= VERTEXCOLOR ? true : false;
static const bool g_bCubemap		= CUBEMAP ? true : false;
static const bool g_bFlashlight		= FLASHLIGHT ? true : false;
static const bool g_bHalfLambert	= HALFLAMBERT ? true : false;
#if (defined( SHADER_MODEL_VS_3_0 ) && MORPHING && DECAL)
static const bool g_bDecalOffset	= true;
#else
static const bool g_bDecalOffset	= false;
#endif

const float4 cBaseTexCoordTransform[2]		:  register( SHADER_SPECIFIC_CONST_0 );
#if SEAMLESS_DETAIL || SEAMLESS_BASE
const float cSeamlessScale					: register( SHADER_SPECIFIC_CONST_2);
#define SEAMLESS_SCALE cSeamlessScale.x
#endif

const float4 cDetailTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_4 );

#if defined ( _X360 )
const float4x4 g_FlashlightWorldToTexture :  register( SHADER_SPECIFIC_CONST_6 ); // 6, 7, 8, 9
#endif

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim			: register( SHADER_SPECIFIC_CONST_10 );
const float4 cMorphSubrect					: register( SHADER_SPECIFIC_CONST_11 );
sampler2D morphSampler						: register( D3DVERTEXTEXTURESAMPLER0, s0 );
#endif

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vPos				: POSITION;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
	float4 vNormal			: NORMAL;
	float4 vColor			: COLOR0;
	float3 vSpecular		: COLOR1;
	// make these float2's and stick the [n n 0 1] in the dot math.
	float4 vTexCoord0		: TEXCOORD0;
	float4 vTexCoord1		: TEXCOORD1;
	float4 vTexCoord2		: TEXCOORD2;
	float4 vTexCoord3		: TEXCOORD3;

	// Position and normal/tangent deltas
	float3 vPosFlex			: POSITION1;
	float3 vNormalFlex		: NORMAL1;
#ifdef SHADER_MODEL_VS_3_0
	float vVertexID			: POSITION2;
#endif
};


struct VS_OUTPUT
{
    float4 projPos					: POSITION;			// Projection-space position	
#if !defined( _X360 )
	float  fog						: FOG;
#endif

#if SEAMLESS_BASE
	HALF3 SeamlessTexCoord			: TEXCOORD0;		// Base texture x/y/z (indexed by swizzle)
#else
	HALF2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate
#endif
#if SEAMLESS_DETAIL
	HALF3 SeamlessDetailTexCoord			: TEXCOORD1;		// Detail texture coordinate
#else
	HALF2 detailTexCoord			: TEXCOORD1;		// Detail texture coordinate
#endif
	float4 color					: TEXCOORD2;		// Vertex color (from lighting or unlit)

#if CUBEMAP || _X360
	float3 worldVertToEyeVector		: TEXCOORD3;		// Necessary for cubemaps
#endif

	float3 worldSpaceNormal			: TEXCOORD4;		// Necessary for cubemaps and flashlight

#if defined ( _X360 ) && FLASHLIGHT
	float4 flashlightSpacePos		: TEXCOORD5;
#endif

	float4 vProjPos					: TEXCOORD6;
	float4 worldPos_ProjPosZ		: TEXCOORD7;
	float4 fogFactorW				: COLOR1;
#if SEAMLESS_DETAIL || SEAMLESS_BASE
	float3 SeamlessWeights          : COLOR0;				// x y z projection weights
#endif

};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	bool bDynamicLight = DYNAMIC_LIGHT ? true : false;
	bool bStaticLight = STATIC_LIGHT ? true : false;
	bool bDoLighting = !g_bVertexColor && (bDynamicLight || bStaticLight);

	float4 vPosition = v.vPos;
	float3 vNormal = 0;
	if ( bDoLighting || FLASHLIGHT || SEAMLESS_BASE || SEAMLESS_DETAIL || LIGHTING_PREVIEW || g_bDecalOffset || CUBEMAP )
	{
		// The vertex only contains valid normals if they are actually needed (fetching them when absent makes D3D complain)
		DecompressVertex_Normal( v.vNormal, vNormal );
	}

#if SEAMLESS_BASE || SEAMLESS_DETAIL
	// compute blend weights in rgb
	float3 NNormal=normalize( vNormal );
	o.SeamlessWeights.xyz = NNormal * NNormal;				// sums to 1.
#endif
	
#if !defined( SHADER_MODEL_VS_3_0 ) || !MORPHING
	ApplyMorph( v.vPosFlex, v.vNormalFlex, vPosition.xyz, vNormal );
#else
	ApplyMorph( morphSampler, cMorphTargetTextureDim, cMorphSubrect, 
		v.vVertexID, v.vTexCoord2, vPosition.xyz, vNormal );
#endif
	  
	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPositionAndNormal( 
		g_bSkinning, 
		vPosition, vNormal,
		v.vBoneWeights, v.vBoneIndices,
		worldPos, worldNormal );

	if ( !g_bVertexColor )
	{
		worldNormal = normalize( worldNormal );
	}

#if defined( SHADER_MODEL_VS_3_0 ) && MORPHING && DECAL
	// Avoid z precision errors
	worldPos += worldNormal * 0.05f * v.vTexCoord2.z;
#endif
	
	o.worldSpaceNormal = worldNormal;

	// Transform into projection space
	float4 vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	o.projPos = vProjPos;
	vProjPos.z = dot( float4( worldPos, 1 ), cViewProjZ );

	o.vProjPos = vProjPos;
	o.fogFactorW.w = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.fogFactorW.w;
#endif	
	o.worldPos_ProjPosZ.xyz = worldPos.xyz;
	o.worldPos_ProjPosZ.w = vProjPos.z;

	// Needed for cubemaps 
#if CUBEMAP
	o.worldVertToEyeVector.xyz = VSHADER_VECT_SCALE * (cEyePos - worldPos);
#endif

#if !defined (_X360) && FLASHLIGHT
	o.color = float4( 0.0f, 0.0f, 0.0f, 0.0f );
#else
	if ( g_bVertexColor )
	{
		// Assume that this is unlitgeneric if you are using vertex color.
		o.color.rgb = ( DONT_GAMMA_CONVERT_VERTEX_COLOR ) ? v.vColor.rgb : GammaToLinear( v.vColor.rgb );
		o.color.a = v.vColor.a;
	}
	else
	{
		#if ( ( USE_STATIC_CONTROL_FLOW ) ||  defined ( SHADER_MODEL_VS_3_0 ) )
		{
			o.color.xyz = DoLighting( worldPos, worldNormal, v.vSpecular, bStaticLight, bDynamicLight, g_bHalfLambert );
		}
		#else
		{
			o.color.xyz = DoLightingUnrolled( worldPos, worldNormal, v.vSpecular, bStaticLight, bDynamicLight, g_bHalfLambert, NUM_LIGHTS );
		}
		#endif
	}
#endif
	

#if SEAMLESS_BASE
	o.SeamlessTexCoord.xyz = SEAMLESS_SCALE * v.vPos.xyz;
#else
	// Base texture coordinates
	o.baseTexCoord.x = dot( v.vTexCoord0, cBaseTexCoordTransform[0] );
	o.baseTexCoord.y = dot( v.vTexCoord0, cBaseTexCoordTransform[1] );
#endif

#if SEAMLESS_DETAIL
	// FIXME: detail texcoord as a 2d xform doesn't make much sense here, so I just do enough so
	// that scale works. More smartness could allow 3d xform.
	o.SeamlessDetailTexCoord.xyz = (SEAMLESS_SCALE*cDetailTexCoordTransform[0].x) * v.vPos.xyz;
#else
	// Detail texture coordinates
	// FIXME: This shouldn't have to be computed all the time.
	o.detailTexCoord.x = dot( v.vTexCoord0, cDetailTexCoordTransform[0] );
	o.detailTexCoord.y = dot( v.vTexCoord0, cDetailTexCoordTransform[1] );
#endif

#if SEPARATE_DETAIL_UVS
	o.detailTexCoord.xy = v.vTexCoord1.xy;
#endif

#if LIGHTING_PREVIEW
	float dot=0.5+0.5*worldNormal*float3(0.7071,0.7071,0);
	o.color.xyz=float3(dot,dot,dot);
#endif

#if defined ( _X360 ) && FLASHLIGHT
	o.flashlightSpacePos = mul( float4( worldPos, 1.0f ), g_FlashlightWorldToTexture );
#endif

	return o;
}


