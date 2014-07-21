//	DYNAMIC: "DOWATERFOG"			"0..1"
//	DYNAMIC: "SKINNING"				"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;
static const int  g_FogType			= DOWATERFOG;

const float4 cShadowTextureMatrix[3]			:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cTexOrigin							:  register( SHADER_SPECIFIC_CONST_3 );
const float4 cTexScale							:  register( SHADER_SPECIFIC_CONST_4 );

// { Shadow falloff offset, 1/Shadow distance, Shadow scale, 0 }
const float3 cShadowConstants					:  register( SHADER_SPECIFIC_CONST_5 );
#define flShadowFalloffOffset	cShadowConstants.x
#define flOneOverShadowDist		cShadowConstants.y
#define flShadowScale			cShadowConstants.z


struct VS_INPUT
{
	float4 vPos				: POSITION;
	float3 vNormal			: NORMAL;
	float4 vBoneWeights		: BLENDWEIGHT;
	float4 vBoneIndices		: BLENDINDICES;
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;			// Projection-space position	
	float3 T0				: TEXCOORD0;		// PS wants this to be 4D but VS doesn't? (see original asm sources)
	float3 T1				: TEXCOORD1;
	float3 T2				: TEXCOORD2;
	float  T3				: TEXCOORD3;
	float4 vColor			: COLOR0;
	float  fog				: FOG;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;
	
	// Perform skinning
	float3 worldNormal, worldPos;
	SkinPositionAndNormal( g_bSkinning, v.vPos, v.vNormal, v.vBoneWeights, v.vBoneIndices, worldPos, worldNormal );

	// Transform into projection space
	o.projPos = mul( float4( worldPos, 1 ), cViewProj );

	// Compute fog
	o.fog = CalcFog( worldPos, o.projPos, g_FogType );

	// Transform position into texture space (from 0 to 1)
	float3 vTexturePos;
	vTexturePos.x = dot( worldPos.xyz, cShadowTextureMatrix[0].xyz );
	vTexturePos.y = dot( worldPos.xyz, cShadowTextureMatrix[1].xyz );
	vTexturePos.z = dot( worldPos.xyz, cShadowTextureMatrix[2].xyz );

	// Figure out the shadow fade amount
	float flShadowFade = ( vTexturePos.z - flShadowFalloffOffset ) * flOneOverShadowDist;

	// Offset it into the texture
	o.T0 = vTexturePos * cTexScale + cTexOrigin;

	// We're doing clipping by using texkill
	o.T1.xyz = vTexturePos.xyz;		// Also clips when shadow z < 0 !
	o.T2.xyz = float3( 1.0f, 1.0f, 1.0f ) - vTexturePos.xyz;
	o.T2.z = 1.0f - flShadowFade;	// Clips when shadow z > shadow distance	

	// We're doing backface culling by using texkill also (wow yucky)
	// --------------------------------------------------------------
	// Transform z component of normal in texture space
	// If it's negative, then don't draw the pixel
	o.T3 = dot( worldNormal, -cShadowTextureMatrix[2] );

	// Shadow color, falloff
	o.vColor.xyz = cModulationColor.xyz;
	o.vColor.w = flShadowFade * flShadowScale;

	return o;
}
