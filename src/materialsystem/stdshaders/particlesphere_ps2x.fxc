// DYNAMIC: "PIXELFOGTYPE"		"0..1"

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "DEPTHBLEND"			"0..1"	[ps20b]

#include "shader_constant_register_map.h"

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );


const float4 g_DepthFeatheringConstants : register( c0 );

sampler BumpmapSampler		: register( s0 );
sampler DepthSampler		: register( s1 );

struct PS_INPUT
{
	float2 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentSpaceLightDir	: TEXCOORD1;
	float3 vAmbientColor			: TEXCOORD2;
#if defined( _X360 )
	float4 vScreenPos_ReverseZ		: TEXCOORD3;
#else
	float4 vScreenPos				: TEXCOORD3;
#endif
	
	float4 vDirLightScale	: COLOR0;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BumpmapSampler, i.vBumpTexCoord );
	
	// Dot the bump normal and the light vector.
	float4 vBumpMapNormal = (baseColor - 0.5);				// The format of the sphere map is 0 to 1, 
															// so this is now -0.5 to 0.5.

	float3 vTangentSpaceLightDir = (i.vTangentSpaceLightDir - 0.5) * 2;	// This is  -1  to 1
	float4 vOutput = dot( vBumpMapNormal, vTangentSpaceLightDir ) + 0.5;
	
	// Scale by the light color outputted by the vertex shader (ie: based on distance).
	vOutput *= i.vDirLightScale;

	// Add ambient.
	vOutput += float4( i.vAmbientColor.x, i.vAmbientColor.y, i.vAmbientColor.z, 0 );

	// Alpha = normal map alpha * vertex alpha
	vOutput.a = baseColor.a * i.vDirLightScale.a;	
	
	//Soft Particles FTW
#	if (DEPTHBLEND == 1)
#		if defined( _X360 )
			vOutput.a *= DepthFeathering( DepthSampler, i.vScreenPos_ReverseZ.xy / i.vScreenPos_ReverseZ.w, i.vScreenPos_ReverseZ.z, i.vScreenPos_ReverseZ.w, g_DepthFeatheringConstants );
#		else
			vOutput.a *= DepthFeathering( DepthSampler, i.vScreenPos.xy / i.vScreenPos.w, i.vScreenPos.z, i.vScreenPos.w, g_DepthFeatheringConstants );
#		endif
#	endif
	
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( vOutput, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}


