//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//

// STATIC:  "STAGE"				"0..2"

#include "common_ps_fxc.h"

sampler g_tPortalColorSampler	: register( s0 );
sampler g_tPortalMaskSampler	: register( s1 );

const float3 g_vConst1			: register( c1 );
#define g_flPortalOpenAmount   g_vConst1.x
#define g_flPortalActive       g_vConst1.y
#define g_flPortalColorScale   g_vConst1.z

struct PS_INPUT
{
	float2 vUv0	: TEXCOORD0;
	float2 vUv1	: TEXCOORD1;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 result;

	result.rgba = tex2D( g_tPortalColorSampler, i.vUv0.xy );
	result.rgb *= g_flPortalColorScale + g_flPortalColorScale + g_flPortalColorScale + g_flPortalColorScale; // This is divided by 4 in C code

	#if ( STAGE == 2 ) // Color layer
		result.a += tex2D( g_tPortalMaskSampler, i.vUv1.xy ).a * ( 1.0f - g_flPortalActive );
	#endif

	return result;
}
