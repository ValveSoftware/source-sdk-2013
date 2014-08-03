// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"		[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"		[ps20b] [XBOX]

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"

sampler NormalSampler	: register( s0 );
sampler BaseTextureSampler	: register( s1 );

const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

struct PS_INPUT
{
	float2 vTexCoord0	: TEXCOORD0;
	float2 vTexCoord1	: TEXCOORD1;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog

	float4 directionalLightColor : COLOR0;
	float4 fogFactorW	: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	float3 vNormalMapDir = tex2D( NormalSampler, i.vTexCoord0 ); // Get the 3-vector from the normal map
	float4 textureColor = tex2D( BaseTextureSampler, i.vTexCoord1 ); // Interpret tcoord t1 as color data.

	//Expand compacted vectors
	//TODO: find if there's a better way to expand a color normal to a full vector ( _bx2 was used in the assembly code )
	vNormalMapDir = (vNormalMapDir - 0.5) * 2.0;
	float3 vLightDir = float3( 0.0f, 0.0f, 1.0f );
	
	float lightDirDotNormalMap = dot( vNormalMapDir, vLightDir ); //normalMap dot dirLightDir

	// do half-lambert on the dot
	lightDirDotNormalMap = lightDirDotNormalMap * 0.5 + 0.5;
	lightDirDotNormalMap = lightDirDotNormalMap * lightDirDotNormalMap;

	float4 resultColor;
	resultColor.xyz = lightDirDotNormalMap * ( textureColor.rgb * i.directionalLightColor.rgb );
	resultColor.a = textureColor.a * i.directionalLightColor.a;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( resultColor, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, (WRITE_DEPTH_TO_DESTALPHA != 0), i.worldPos_projPosZ.w );
}
