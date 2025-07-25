//==================================================================================================
//
// Physically Based Rendering vertex shader for brushes and models
//
//==================================================================================================

//  DYNAMIC: "COMPRESSED_VERTS"         "0..1"
//  DYNAMIC: "DOWATERFOG"               "0..1"
//  DYNAMIC: "SKINNING"                 "0..1"
//  DYNAMIC: "LIGHTING_PREVIEW"         "0..1"
//  DYNAMIC: "NUM_LIGHTS"               "0..4"

#include "common_vs_fxc.h"

static const bool g_bSkinning           = SKINNING ? true : false;
static const int g_FogType              = DOWATERFOG;
const float4 cBaseTexCoordTransform[2]  : register(SHADER_SPECIFIC_CONST_0);

//-----------------------------------------------------------------------------
// Input vertex format
//-----------------------------------------------------------------------------
struct VS_INPUT
{
    // This is all of the stuff that we ever use
    float4 vPos                     : POSITION;
    float4 vBoneWeights             : BLENDWEIGHT;
    float4 vBoneIndices             : BLENDINDICES;
    float4 vNormal                  : NORMAL;
    float2 vTexCoord0               : TEXCOORD0;
    float4 vLightmapTexCoord        : TEXCOORD1;
    float4 vLightmapTexCoordOffset  : TEXCOORD2;
};

struct VS_OUTPUT
{
    // Stuff that isn't seen by the pixel shader
    float4 projPosSetup             : POSITION;
    float  fog                      : FOG;
    // Stuff that is seen by the pixel shader
    float2 baseTexCoord             : TEXCOORD0;
    float4 lightAtten               : TEXCOORD1;
    float3 worldNormal              : TEXCOORD2;
    float3 worldPos                 : TEXCOORD3;
    float3 projPos                  : TEXCOORD4;
    float4 lightmapTexCoord1And2    : TEXCOORD5;
    float4 lightmapTexCoord3        : TEXCOORD6;
};

//-----------------------------------------------------------------------------
// Main shader entry point
//-----------------------------------------------------------------------------
VS_OUTPUT main( const VS_INPUT v )
{
    VS_OUTPUT o = ( VS_OUTPUT )0;

    o.lightmapTexCoord3.z = dot(v.vTexCoord0, cBaseTexCoordTransform[0]) + cBaseTexCoordTransform[0].w;
    o.lightmapTexCoord3.w = dot(v.vTexCoord0, cBaseTexCoordTransform[1]) + cBaseTexCoordTransform[1].w;
    o.lightmapTexCoord1And2.xy = v.vLightmapTexCoord + v.vLightmapTexCoordOffset;

    float2 lightmapTexCoord2 = o.lightmapTexCoord1And2.xy + v.vLightmapTexCoordOffset;
    float2 lightmapTexCoord3 = lightmapTexCoord2 + v.vLightmapTexCoordOffset;

    // Reversed component order
    o.lightmapTexCoord1And2.w = lightmapTexCoord2.x;
    o.lightmapTexCoord1And2.z = lightmapTexCoord2.y;

    o.lightmapTexCoord3.xy = lightmapTexCoord3;

    float3 vNormal;
    DecompressVertex_Normal(v.vNormal, vNormal);

    float3 worldNormal, worldPos;
    SkinPositionAndNormal(g_bSkinning, v.vPos, vNormal, v.vBoneWeights, v.vBoneIndices, worldPos, worldNormal);

    // Transform into projection space
    float4 vProjPos = mul(float4(worldPos, 1), cViewProj);
    o.projPosSetup = vProjPos;
    vProjPos.z = dot(float4(worldPos, 1), cViewProjZ);

    o.projPos = vProjPos.xyz;
    o.fog = CalcFog(worldPos, vProjPos.xyz, g_FogType);

    // Needed for water fog alpha and diffuse lighting 
    o.worldPos = worldPos;
    o.worldNormal = normalize(worldNormal);

    // Scalar attenuations for four lights
    o.lightAtten = float4(0, 0, 0, 0);

    #if (NUM_LIGHTS > 0)
        o.lightAtten.x = GetVertexAttenForLight(worldPos, 0, false);
    #endif

    #if (NUM_LIGHTS > 1)
        o.lightAtten.y = GetVertexAttenForLight(worldPos, 1, false);
    #endif

    #if (NUM_LIGHTS > 2)
        o.lightAtten.z = GetVertexAttenForLight(worldPos, 2, false);
    #endif

    #if (NUM_LIGHTS > 3)
        o.lightAtten.w = GetVertexAttenForLight(worldPos, 3, false);
    #endif

    // Base texture coordinate transform
    o.baseTexCoord.x = dot(v.vTexCoord0, cBaseTexCoordTransform[0]);
    o.baseTexCoord.y = dot(v.vTexCoord0, cBaseTexCoordTransform[1]);

    return o;
}
