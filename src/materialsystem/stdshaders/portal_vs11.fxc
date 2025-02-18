// STATIC: "MAXTEXTURESTAGES"	"0..2"
// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"
// STATIC: "USEALTERNATEVIEW"	"0..1"

// DYNAMIC: "SKINNING"			"0..1"
// DYNAMIC: "ADDSTATIC"			"0..1" 
//in multipass configurations, this specifies whether we're adding static on this pass

#define TEXTURESTAGES (MAXTEXTURESTAGES + 1)
#define USESTATICTEXTURE (((ADDSTATIC == 1) && (HASSTATICTEXTURE == 1))?(1):(0))

#include "common_vs_fxc.h"

static const int g_bSkinning		= SKINNING ? true : false;

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
	float2 vPrimaryTexCoord			: TEXCOORD0; //either the portal cutout, or static

#	if( TEXTURESTAGES == 3 )
#		if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
			float2 vSecondaryTexCoord		: TEXCOORD1;
#			if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
				float2 vTertiaryTexCoord		: TEXCOORD2;
#			endif
#		endif
#	elif( TEXTURESTAGES > 1 && HASALPHAMASK == 1 )
		float2 vSecondaryTexCoord		: TEXCOORD1;
#	endif

#	if( !defined( _X360 ) )
		float vFog						: FOG;
#	endif
};

float2 GetPortalTextureCoordinate( float3 worldPos, float4 projPos )
{
	float2 result;
	
	float4 vTextureProjectedPos;
#if ( USEALTERNATEVIEW == 1 )
	vTextureProjectedPos = mul( float4( worldPos, 1 ), g_CustomViewProj );	
#else	
	vTextureProjectedPos = projPos;
#endif

	//Screen coordinates mapped back to texture coordinates for the portal texture
	result.x = vTextureProjectedPos.x;
	result.y = -vTextureProjectedPos.y; // invert Y
	result.xy = (result.xy + vTextureProjectedPos.w) * 0.5f;
	result.xy = result.xy / vTextureProjectedPos.w;
	
#if ( USEALTERNATEVIEW == 1 )
	result.xy = saturate( result.xy ); //stretch instead of clipping.
#endif

	return result;
}

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 worldPos;	
	SkinPosition( 
			g_bSkinning, 
			v.vPos,
			v.vBoneWeights, v.vBoneIndices,
			worldPos );	
			
	o.vProjPos = mul( float4( worldPos, 1 ), cViewProj );
	
	//Because we're pairing this with ps11, we can't divide the portal cutout texture coordinates in the pixel shader by w
	//So, we had to divide it by w here in the vertex shader. Unfortunately that causes some interpolation inconsistencies
	//between the position and the texture coordinate. So we also divide the position by w here. Causing the exact same projection
	//errors for both position and texture coordinate, thereby eliminating the difference between the two that cause the "warping" look.
	o.vProjPos.xyz = o.vProjPos.xyz * (1.0f / o.vProjPos.w);
	o.vProjPos.w = 1.0f;
	
#if !defined( _X360 )
	o.vFog = CalcFog( worldPos, o.vProjPos.xyz, FOGTYPE_RANGE );
#endif
			
#	if( TEXTURESTAGES == 3 )//single pass configuration
	{
		o.vPrimaryTexCoord = GetPortalTextureCoordinate( worldPos, o.vProjPos );
		
#		if( HASALPHAMASK == 1 )
		{
			o.vSecondaryTexCoord = v.vMappingTexCoord.xy;
#			if( USESTATICTEXTURE == 1 )
				o.vTertiaryTexCoord = v.vMappingTexCoord.xy;
#			endif
		}
#		else
		{
#			if( USESTATICTEXTURE == 1 )
				o.vSecondaryTexCoord = v.vMappingTexCoord.xy;
#			endif
		}
#		endif
	}
#	else //multipass configuration 
	{
#		if( ADDSTATIC == 0 ) //if addstatic is 0, we're rendering the cutout on this pass, if it's 1, we're rendering static on this pass
		{
			o.vPrimaryTexCoord = GetPortalTextureCoordinate( worldPos, o.vProjPos );
		}
#		else
		{
			o.vPrimaryTexCoord = v.vMappingTexCoord.xy;
		}
#		endif

	
#		if( (TEXTURESTAGES > 1) && (HASALPHAMASK == 1) ) //supports alpha mask as well
			o.vSecondaryTexCoord = v.vMappingTexCoord.xy;
#		endif
	}
#	endif


	return o;
}

	