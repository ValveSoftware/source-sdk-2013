//	STATIC: "ONLY_PROJECT_POSITION"	"0..1" [XBOX]
//	STATIC: "ONLY_PROJECT_POSITION"	"0..0" [PC]
//  STATIC: "COLOR_DEPTH"			"0..1"

//	DYNAMIC: "COMPRESSED_VERTS"		"0..1"
//	DYNAMIC: "SKINNING"				"0..1"
//  DYNAMIC: "MORPHING"				"0..1"  [vs30]

#include "common_vs_fxc.h"

static const bool g_bSkinning = SKINNING ? true : false;

#ifdef SHADER_MODEL_VS_3_0
// NOTE: cMorphTargetTextureDim.xy = target dimensions,
//		 cMorphTargetTextureDim.z = 4tuples/morph
const float3 cMorphTargetTextureDim			: register( SHADER_SPECIFIC_CONST_6 );
const float4 cMorphSubrect					: register( SHADER_SPECIFIC_CONST_7 );

sampler2D morphSampler : register( D3DVERTEXTEXTURESAMPLER0, s0 );
#endif


struct VS_INPUT
{
	float4 vPos					: POSITION;
	float2 vTexCoord			: TEXCOORD0;
	float4 vBoneWeights			: BLENDWEIGHT;
	float4 vBoneIndices			: BLENDINDICES;

	// Position delta stream
	float3 vPosFlex				: POSITION1;

#ifdef SHADER_MODEL_VS_3_0
	float vVertexID				: POSITION2;
#endif
};

struct VS_OUTPUT
{
    float4 vProjPos				: POSITION;

#if (ONLY_PROJECT_POSITION == 0) //360 sometimes runs without the pixel shader component, but has to patch this output if it does.
	float2 texCoord				: TEXCOORD0;
#endif

#if COLOR_DEPTH
	float4 vWorldPos_projPosZ	: TEXCOORD1;
#endif

};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;
	float3 vWorldPos;
	float4 vPosition = v.vPos;

#if !defined( SHADER_MODEL_VS_3_0 ) || !MORPHING
	ApplyMorph( v.vPosFlex, vPosition.xyz );
#else
	ApplyMorph( morphSampler, cMorphTargetTextureDim, cMorphSubrect, 
		v.vVertexID, float3(0, 0, 0), vPosition.xyz );
#endif

	SkinPosition( g_bSkinning, vPosition, v.vBoneWeights, v.vBoneIndices, vWorldPos );

	float4 vProjPos = mul( float4( vWorldPos, 1.0f ), cViewProj );

	o.vProjPos    = vProjPos;

#if (ONLY_PROJECT_POSITION == 0)
	o.texCoord   = v.vTexCoord;
#endif

#if ( COLOR_DEPTH && !ONLY_PROJECT_POSITION )
	o.vWorldPos_projPosZ.z = vProjPos.z;
	o.vWorldPos_projPosZ.w = vProjPos.w;
#endif

	return o;
}


