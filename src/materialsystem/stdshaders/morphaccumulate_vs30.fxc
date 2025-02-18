//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======

//  STATIC: "CONSTANTBASEDMORPH"	"0..1"

#include "common_vs_fxc.h"

struct VS_INPUT
{
	float4 vSrcCoord		: TEXCOORD0;
	float vMorphWeightId	: TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 vDestCoord			: POSITION;
	float2 vSrcCoord			: TEXCOORD0;
	float2 vSideSpeedCoord		: TEXCOORD1;
	
#if CONSTANTBASEDMORPH
	float4 vMorphWeights		: TEXCOORD2;
#else
	float2 vMorphWeightCoord	: TEXCOORD2;
#endif
};

#if !CONSTANTBASEDMORPH
const float4 vMorphWeightSubrect	 :  register( SHADER_SPECIFIC_CONST_0 );
const float4 vMorphWeightDimensions	 :  register( SHADER_SPECIFIC_CONST_1 );
#endif

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	// FIXME: Want these to be in pixel centers!
	float2 projSpace = v.vSrcCoord.zw;
	projSpace *= 2.0f;
	projSpace -= 1.0f;
	projSpace.y *= -1.0f;
	
	o.vDestCoord = float4( projSpace.x, projSpace.y, 0.0f, 1.0f );
	o.vSrcCoord = v.vSrcCoord.xy;
	o.vSideSpeedCoord = v.vSrcCoord.xy;

#if CONSTANTBASEDMORPH
	o.vMorphWeights = cFlexWeights[ v.vMorphWeightId ];
#else
	float flColumn = floor( v.vMorphWeightId / vMorphWeightSubrect.w );
	o.vMorphWeightCoord.x = vMorphWeightSubrect.x + flColumn + 0.5f;
	o.vMorphWeightCoord.y = vMorphWeightSubrect.y + v.vMorphWeightId - flColumn * vMorphWeightSubrect.w + 0.5f;
	o.vMorphWeightCoord.xy /= vMorphWeightDimensions.xy;
#endif	
	return o;
}