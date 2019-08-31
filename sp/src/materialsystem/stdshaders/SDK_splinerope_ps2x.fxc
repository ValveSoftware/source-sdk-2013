//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

// STATIC: "SHADER_SRGB_READ"			"0..1"	[XBOX]
// STATIC: "SHADER_SRGB_READ"			"0..0"	[PC]
// STATIC: "SHADOWDEPTH"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"		[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"		[ps20b] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"		[ps20]
// DYNAMIC: "PIXELFOGTYPE"				"0..1"

#include "common_ps_fxc.h"

float4 g_FogParams						: register( c0 );
float3 g_EyePos							: register( c1 );

// VS_OUTPUT in a common file.
#define PIXELSHADER
#include "common_splinerope_fxc.h"

sampler BaseTextureSampler	: register( s0 );
sampler NormalSampler		: register( s1 );

float4 main( PS_INPUT i ) : COLOR
{
	#if ( SHADOWDEPTH == 0 )
	{
		float3 vNormalMapDir = tex2D( NormalSampler, i.texCoord.xy ); // Get the 3-vector from the normal map
		float4 textureColor = tex2D( BaseTextureSampler, i.texCoord.xy );

		//Expand compacted vectors
		vNormalMapDir = ( vNormalMapDir - 0.5 ) * 2.0;
		float3 vLightDir = float3( 0.0f, 0.0f, 1.0f );

		float lightDirDotNormalMap = dot( vNormalMapDir, vLightDir ); //normalMap dot dirLightDir

		// do half-lambert on the dot
		lightDirDotNormalMap = lightDirDotNormalMap * 0.5 + 0.5;
		lightDirDotNormalMap = lightDirDotNormalMap * lightDirDotNormalMap;

		float4 resultColor;
		resultColor.xyz = lightDirDotNormalMap * ( textureColor.rgb * i.argbcolor.rgb );
		resultColor.a = textureColor.a * i.argbcolor.a;

		float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.xyz, i.worldPos_projPosZ.xyz, i.worldPos_projPosZ.w );
		return FinalOutput( resultColor, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, (WRITE_DEPTH_TO_DESTALPHA != 0), i.worldPos_projPosZ.w );
	}
	#else
	{
		return float4( 0.0f, 0.0f, 0.0f, 1.0f );
	}
	#endif
}
