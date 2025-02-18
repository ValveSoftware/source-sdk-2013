// DYNAMIC: "MODE" "0..9"

#include "common_ps_fxc.h"

const float g_Alpha : register( c0 );

sampler BaseTextureSampler	: register( s0 );
sampler BaseTextureSampler2	: register( s1 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
	float2 baseTexCoord2			: TEXCOORD1;
};

HALF Grey( HALF3 input )
{
	return dot( ( float3 )( 1.0f / 3.0f ), input );
}

HALF4 main( PS_INPUT i ) : COLOR
{
	float3 scene = tex2D( BaseTextureSampler, i.baseTexCoord );
	float3 gman = tex2D( BaseTextureSampler2, i.baseTexCoord2 );

#if MODE == 0
	// negative greyscale of scene * gman
	float scale = 1.0f / 3.0f;
	scene.xyz = dot( float3( scale, scale, scale), scene.xyz );
	scene = 1.0f - scene;

	return float4( scene * gman, g_Alpha );
#endif
	
#if MODE == 1
	if( Grey( gman ) < 0.3 )
	{
		return float4( 1.0f - gman, g_Alpha );
	}
	else
	{
		return float4( ( 1.0f - gman ) * scene, g_Alpha );
	}
#endif

#if MODE == 2
	return float4( lerp( scene, gman, g_Alpha ), g_Alpha );
#endif

#if MODE == 3
	return float4( lerp( scene, Grey( gman ), Grey( gman ) ), g_Alpha );
#endif

#if MODE == 4
	return float4( lerp( scene, gman, g_Alpha ), g_Alpha );
#endif

#if MODE == 5
	float sceneLum = scene.r;
	if( sceneLum > 0.0f )
	{
		return float4( scene, g_Alpha );
	}
	else
	{
		return float4( gman, g_Alpha );
	}
#endif

#if MODE == 6
	return float4( scene + gman, g_Alpha );
#endif

#if MODE == 7
	return float4( scene, g_Alpha );
#endif

#if MODE == 8
	return float4( lerp( scene, gman, g_Alpha ), g_Alpha );
#endif

#if MODE == 9
	/*
	float3 cGammaLayer1 = scene;
	float3 cGammaLayer2 = gman;

	float flLayer1Brightness = saturate( dot( cGammaLayer1.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );

	float3 cGammaOverlayResult;
	if ( flLayer1Brightness < 0.5f )
	{
		cGammaOverlayResult.rgb = ( 2.0f * cGammaLayer1.rgb * cGammaLayer2.rgb );
	}
	else
	{
		cGammaOverlayResult.rgb = ( 1.0f - ( 2.0f * ( 1.0f - cGammaLayer1.rgb ) * ( 1.0f - cGammaLayer2.rgb ) ) );
	}
	//*/

	float3 cLayer1 = scene;
	float3 cLayer2 = gman;

	float flLayer1Brightness = saturate( dot( cLayer1.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );

	// Modify layer 1 to be more contrasty.
	cLayer1.rgb = saturate( cLayer1.rgb * cLayer1.rgb * 2.0f );
	float3 cGammaOverlayResult = cLayer1.rgb + cLayer2.rgb * saturate( 1.0f - flLayer1Brightness * 2.0f );

	return float4( cGammaOverlayResult.rgb, g_Alpha );
#endif
}
