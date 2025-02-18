//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// STATIC: "STAGE" "0..2"

// Includes
#include "common_vs_fxc.h"

// Globals
//const float g_flTime : register( SHADER_SPECIFIC_CONST_0 );
const float4 cBaseTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_1 );

const float2 g_vConst3 : register( SHADER_SPECIFIC_CONST_3 );
#define g_flPortalOpenAmount g_vConst3.x
#define g_flPortalStaticAmount g_vConst3.y

// Structs
struct VS_INPUT
{
	float4 vPos					: POSITION;		// Position
	float4 vTexCoord0			: TEXCOORD0;	// Base texture coordinates
};

struct VS_OUTPUT
{
	float4 vProjPosition		: POSITION;  // Projection-space position
	float2 vUv0					: TEXCOORD0;
	float2 vUv1					: TEXCOORD1;

	#ifndef _X360
		float  fog				: FOG;
	#endif
};

// Main
VS_OUTPUT main( const VS_INPUT i )
{
	float kFlPortalOuterBorder = 0.075f; // Must match PS!

	VS_OUTPUT o;

	// Transform the position
	o.vProjPosition.xyzw = mul( i.vPos, cModelViewProj );

	// Portal open time
	float flPortalOpenAmount = saturate( g_flPortalOpenAmount + 0.001f ); // 0.001f to avoid divide by zero

	// Texture coordinates
	float2 vBaseUv;
	vBaseUv.x = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[0] );
	vBaseUv.y = dot( i.vTexCoord0.xy, cBaseTexCoordTransform[1] );
	o.vUv0.xy = ( ( vBaseUv.xy - 0.5f ) / ( flPortalOpenAmount * flPortalOpenAmount ) ) + 0.5f;
	o.vUv1.xy = o.vUv0.xy;

	// Fog
	#ifndef _X360
	{
		float3 vWorldPos = mul( i.vPos, cModel[0] );
		o.fog = CalcFog( vWorldPos.xyz, o.vProjPosition, FOGTYPE_RANGE );
	}
	#endif

	return o;
}
