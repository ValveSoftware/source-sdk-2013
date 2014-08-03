//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"  "0..1"	[ps30]
// DYNAMIC: "PIXELFOGTYPE"				"0..1"

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

sampler BaseTextureSampler			: register( s0 );
sampler IrisSampler					: register( s1 );
sampler GlintSampler				: register( s2 );
const float4 cEyeScalars			: register( c0 );	// { Dilation, ambient, x, x }

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
	float2 irisTexCoord				: TEXCOORD1;
	float2 glintTexCoord			: TEXCOORD2;
	float3 vertAtten				: TEXCOORD3;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

#define fDilationFactor	cEyeScalars.x
#define fGlintDamping	cEyeScalars.y

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseSample  = tex2D( BaseTextureSampler, i.baseTexCoord  );
	float4 glintSample = tex2D( GlintSampler,       i.glintTexCoord );
/*
	// Dilate the pupil/iris texture (1 is max dilation, 0 is none)
	float2 biasedCoords = i.irisTexCoord * 2.0f - 1.0f;								// -1 to +1 range
	float fDilatability = saturate(0.8f - sqrt(dot(biasedCoords, biasedCoords) ));	// 1 in the center, fading out to 0 at 0.8 from center, since irises are inset into maps
	float2 scaledCoords = biasedCoords * (1 + fDilatability);						// Maximal dilation
	
	// Blend undilated and maximally dilated based upon dilation factor
	float2 dilatedCoords = lerp( scaledCoords, biasedCoords, 1.0f-saturate(cDilationFactor.x));	
	dilatedCoords = dilatedCoords * 0.5f + 0.5f;									// Back to 0..1 range
*/

	float4 irisSample = tex2D( IrisSampler, i.irisTexCoord );						// Sample the iris map using dilated coordinates

	float4 result;
	result.rgb = lerp( baseSample.rgb, irisSample.rgb, irisSample.a );
	result.rgb *= i.vertAtten;
	result.rgb += glintSample.rgb * fGlintDamping;
	result.a = baseSample.a;
	
	bool bWriteDepthToAlpha = false;

	// ps_2_b and beyond
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0))
	bWriteDepthToAlpha = WRITE_DEPTH_TO_DESTALPHA != 0;
#endif

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, i.worldPos_projPosZ.w  );
}
