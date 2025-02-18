//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

//  STATIC: "CONSTANTBASEDMORPH"	"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

struct PS_INPUT
{
	float2 vSrcCoord			: TEXCOORD0;
	float2 vSideSpeedCoord		: TEXCOORD1;
#if CONSTANTBASEDMORPH
	float4 vMorphWeights		: TEXCOORD2;
#else
	float2 vMorphWeightCoord	: TEXCOORD2;
#endif
};

sampler MorphTarget		: register( s0 );
sampler SideSpeedMap	: register( s1 );
#if !CONSTANTBASEDMORPH
sampler MorphWeights	: register( s2 );
#endif

const float4 vMorphScale : register( c0 );

HALF4 main( PS_INPUT i ) : COLOR
{
#if CONSTANTBASEDMORPH
	float4 vMorphWeights = i.vMorphWeights;
#else
	float4 vMorphWeights = tex2D( MorphWeights, i.vMorphWeightCoord );
#endif
	
	float4 delta = tex2D( MorphTarget, i.vSrcCoord );
	float4 sideSpeed = tex2D( SideSpeedMap, i.vSideSpeedCoord );
	
	// NOTE: This is necessary to fixup slight errors in the delta.
	// On the cpu, only the range 0-65534 is used so we can encode -1, 0, and 1 exactly.
	delta *= 65535.0f / 65534.0f;

	// Compute total weight, taking into account side + speed
	float flWeight = lerp( vMorphWeights.y, vMorphWeights.x, sideSpeed.y );
	float flStereoWeight = lerp( vMorphWeights.w, vMorphWeights.z, sideSpeed.y ); 
	float w = lerp( flWeight, flStereoWeight, sideSpeed.x );
	
	// Convert 0-1 -> -1 to 1
	delta *= 2.0f;
	delta -= float4( 1.0f, 1.0f, 1.0f, 1.0f );
	
	// Apply the morph scale
	delta *= vMorphScale;
	
	// Apply weight	
	delta *= w;
	
	return delta;
}

