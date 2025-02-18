// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"
// STATIC: "USEALTERNATEVIEW"	"0..1"

// DYNAMIC: "SKINNING"			"0..1"
// DYNAMIC: "ADDSTATIC"			"0..1" 

#define USESTATICTEXTURE (((ADDSTATIC == 1) && (HASSTATICTEXTURE == 1))?(1):(0))

#include "common_vs_fxc.h"

static const bool g_bSkinning = SKINNING ? true : false;

#if ( USEALTERNATEVIEW == 1 )
const float4x4 g_CustomViewProj	: register( SHADER_SPECIFIC_CONST_0 );
#endif

struct VS_INPUT
{
	float4 vPos							: POSITION;

	float4 vBoneWeights					: BLENDWEIGHT;
	float4 vBoneIndices					: BLENDINDICES;
	float2 vMappingTexCoord				: TEXCOORD0;
};


struct VS_OUTPUT
{
	float4 vProjPos					: POSITION;
	float3 vPortalTexCoord			: TEXCOORD0;

#	if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
		float2 vSecondaryTexCoord		: TEXCOORD1;
#		if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
			float2 vTertiaryTexCoord		: TEXCOORD2;
#		endif
#	endif

#	if !defined( _X360  )
		float vFog						: FOG;
#	endif

	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;
	
	float3 worldPos;
	
	SkinPosition( 
			g_bSkinning, 
			v.vPos,
			v.vBoneWeights, v.vBoneIndices,
			worldPos );
			
	float4 vTextureProjectedPos;
			
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	
#if ( USEALTERNATEVIEW == 1 )
	vTextureProjectedPos = mul( float4( worldPos, 1 ), g_CustomViewProj );	
#else	
	vTextureProjectedPos = o.vProjPos;
#endif
	
#if !defined( _X360 )
	o.vFog = CalcFog( worldPos, o.vProjPos.xyz, FOGTYPE_RANGE );
#endif
	o.worldPos_projPosZ = float4( worldPos.xyz, o.vProjPos.z );
	
	//Screen coordinates mapped back to texture coordinates for the portal texture
	o.vPortalTexCoord.x = vTextureProjectedPos.x;
	o.vPortalTexCoord.y = -vTextureProjectedPos.y; // invert Y
	o.vPortalTexCoord.xy = (o.vPortalTexCoord.xy + vTextureProjectedPos.w) * 0.5f;
	o.vPortalTexCoord.z = vTextureProjectedPos.w;
	
#if ( USEALTERNATEVIEW == 1 )
	o.vPortalTexCoord.xy = saturate( o.vPortalTexCoord.xy / vTextureProjectedPos.w ) * vTextureProjectedPos.w; //stretch instead of clipping.
#endif
	
#	if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
	{
		o.vSecondaryTexCoord = v.vMappingTexCoord.xy;
#		if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
			o.vTertiaryTexCoord = v.vMappingTexCoord.xy;
#		endif
	}
#	endif

	return o;
}

	