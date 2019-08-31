//========== Copyright (c) Valve Corporation, All rights reserved. ==========//
// DYNAMIC: "DOWATERFOG"	"0..1"

#include "common_vs_fxc.h"
#include "spline_fxc.h"

const float4x3 cModelView			: register(SHADER_SPECIFIC_CONST_0);
const float4x4 cProj				: register(SHADER_SPECIFIC_CONST_3);
const float g_MinPixelSize        : register(SHADER_SPECIFIC_CONST_7);

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vTint            : COLOR;
	float4 vParms           : POSITION;						// T V side_id
	float4 vSplinePt0		: TEXCOORD0;					// x y z rad
	float4 vSplinePt1		: TEXCOORD1;					// x y z rad
	float4 vSplinePt2		: TEXCOORD2;					// x y z rad
	float4 vSplinePt3		: TEXCOORD3;					// x y z rad
};

// VS_OUTPUT in a common file.
#include "common_splinerope_fxc.h"

#define P0 (v.vSplinePt0)
#define P1 (v.vSplinePt1)
#define P2 (v.vSplinePt2)
#define P3 (v.vSplinePt3)

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	// posrad.xyz is worldspace position and posrad.w is worldspace diameter.
	float4 posrad =	CatmullRomSpline( P0, P1, P2, P3, v.vParms.x );

	// calculate projected position here so that we can figure out how much to bloat the diameter to avoid aliasing of the sort where you skip pixels in a segment.
	{
		// PERF FIXME!!  This could be simplified quite a bit if this ever becomes a bottleneck.  I feel dirty.
		// Get the view-space position for two points that are posrad.w units away from each other horizontally.
		float3 viewPos1 = mul4x3( float4( posrad.xyz, 1.0f ), cModelView );
		float3 viewPos2 = viewPos1 + float3( posrad.w, 0.0f, 0.0f );

		// Project both points.
		float4 projPos1 = mul( float4( viewPos1, 1.0f ), cProj );
		float4 projPos2 = mul( float4( viewPos2, 1.0f ), cProj );

		// Get the distance of the two points from each other in normalized screen space.
		float projectedDiameterInPixels = abs( ( projPos1.x / projPos1.w ) - ( projPos2.x / projPos2.w ) );

		// Compare the distance between the two points to the minimum allowed to keep from skipping pixels and causing aliasing.
		if ( projectedDiameterInPixels < g_MinPixelSize )
		{
			// Scale the radius in world space so that it is bigger than the required pixel size in screen space.
			posrad.w *= ( g_MinPixelSize / projectedDiameterInPixels );
		}
	}

	float3 v2p = float3( 0, 0, 1 );
	v2p = posrad.xyz - cEyePos;							// screen aligned

	float3 tangent = DCatmullRomSpline3( P0, P1, P2, P3, v.vParms.x );
	float3 ofs = normalize( cross( v2p, normalize( tangent ) ) );
	posrad.xyz += ofs * ( posrad.w * ( v.vParms.z - .5 ) );
	o.projPos  = mul( float4(posrad.xyz, 1.0f), cViewProj );
	o.worldPos_projPosZ.xyz = posrad.xyz;
	o.worldPos_projPosZ.w = o.projPos.z;
	o.texCoord.xy = float2( 1.0f - v.vParms.z, v.vParms.y );
	o.argbcolor = float4( v.vTint.rgb, v.vTint.a );

	#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	{
		o.fog = CalcFixedFunctionFog( posrad.xyz, DOWATERFOG );
	}
	#endif

	return o;
}
