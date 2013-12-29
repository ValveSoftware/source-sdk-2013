// DYNAMIC: "CMBO_HUDUNDISTORT" "0..1"

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler : register( s0 );
sampler DistortMapTextureSampler : register( s1 );

const float4 DistortBounds : register( c0 );
const int bHudTranslucent : register( c1 );

struct PS_INPUT
{
	float2 vBaseTexCoord : TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float2 vOriginal = i.vBaseTexCoord.xy;


	// The full uv 0->1 range of the base texture here is shifted/scaled so that it maps
	// to the region that would be minUV->maxUV of the base texture in the regular undistort
	// code.  This lets us overlay a higher-resolution inset rectangle directly onto the
	// render target with undistort, which results in a much higher-quality HUD.

	float2 minUV = DistortBounds.xy;
	float2 maxUV = DistortBounds.zw;
	float2 scaleUV = 1.0 / ( maxUV - minUV );


	float2 vGreen;
	float4 vFinal;

	#if ( CMBO_HUDUNDISTORT )
	{
		float4 vRead = tex2D( DistortMapTextureSampler, vOriginal );

		float2 vRed = vRead.xy;
		float2 vBlue = vRead.zw;

		vGreen = ( vRed + vBlue ) / 2.0;

		vRed = ( vRed - minUV ) * scaleUV;
		vGreen = ( vGreen - minUV ) * scaleUV;
		vBlue = ( vBlue - minUV ) * scaleUV;

		vFinal.r = tex2D( BaseTextureSampler, vRed ).r;
		vFinal.ga = tex2D( BaseTextureSampler, vGreen ).ga;
		vFinal.b = tex2D( BaseTextureSampler, vBlue ).b;
	}
	#else
	{
		vGreen = ( vOriginal - minUV ) * scaleUV;
		vFinal = tex2D( BaseTextureSampler, vGreen );
	}
	#endif


	// When the HUD isn't supposed to be rendered as translucent, some of its elements do occasionally have non-unit alpha.
	// We always have blending and alphatest enabled here, so if the hud itself is not supposed to be translucent we need
	// to fix up the alphas.
	vFinal.a = lerp( 1, vFinal.a, bHudTranslucent );


	// Smooth off the edges of the quad.  This also gives (0,0,0,0) in the outer areas, for alpha test and for blackout.

	const float edgeRampFrac = 0.005;
	float2 uvEdgeRamp = smoothstep( float2(-edgeRampFrac,-edgeRampFrac), float2(edgeRampFrac,edgeRampFrac), vGreen ) *
	    ( 1 - smoothstep( float2(1-edgeRampFrac,1-edgeRampFrac), float2(1+edgeRampFrac,1+edgeRampFrac), vGreen ) );

	float edgeRamp = uvEdgeRamp.x * uvEdgeRamp.y;

	vFinal *= edgeRamp;


	return vFinal;
}
