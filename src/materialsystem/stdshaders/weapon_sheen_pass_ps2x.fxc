//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps30][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]
// STATIC: "BUMPMAP" "0..1"

// Includes =======================================================================================
#include "common_vertexlitgeneric_dx9.h"

// Texture Samplers ===============================================================================
sampler g_tRefractionSampler		: register( s0 );
#if BUMPMAP
	sampler g_tBumpSampler			: register( s1 );
#endif

sampler EnvmapSampler			: register( s2 );
sampler EnvmapMaskSampler		: register( s3 );

// Shaders Constants and Globals ==================================================================
const float4 g_mViewProj0			: register( c0 );	// 1st row of matrix
const float4 g_mViewProj1			: register( c1 );	// 2nd row of matrix

const float4 g_vCameraPosition		: register( c5 );
const float4 g_vPackedConst6		: register( c6 );
const float4 g_vPackedConst7		: register( c7 );
const float4 g_cCloakColorTint		: register( c8 );

#define g_flSheenMapMaskScaleX   g_vPackedConst6.x // Default = 1.0f
#define g_flSheenMapMaskScaleY   g_vPackedConst6.y // Default = 1.0f
#define g_flSheenMapMaskOffsetX  g_vPackedConst6.z // Default = 0.0f
#define g_flSheenMapMaskOffsetY  g_vPackedConst6.w // Default = 0.0f

#define g_flSheenDirection		g_vPackedConst7.x // 0,1,2 -> XYZ
#define g_flEffectIndex			g_vPackedConst7.y // W

// 8 2D Poisson offsets (designed to use .xy and .wz swizzles (not .zw)
static const float4 g_vPoissonOffset[4] = {	float4 (-0.0876f,  0.9703f,  0.5651f,  0.4802f ),
											float4 ( 0.1851f,  0.1580f, -0.0617f, -0.2616f ),
											float4 (-0.5477f, -0.6603f,  0.0711f, -0.5325f ),
											float4 (-0.0751f, -0.8954f,  0.4054f,  0.6384f ) };

// Interpolated values ============================================================================
struct PS_INPUT
{
	float3 vWorldNormal			: TEXCOORD0; // World-space normal
	float3 vProjPosForRefract	: TEXCOORD1;
	float3 vWorldViewVector		: TEXCOORD2;
	float3x3 mTangentSpaceTranspose : TEXCOORD3;
	//	     second row				: TEXCOORD4;
	//	     third row				: TEXCOORD5;
	float2 vTexCoord0				: TEXCOORD6;

	float4 vModelSpacePos				: TEXCOORD7;
};

// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	float3 vWorldNormal = normalize( i.vWorldNormal.xyz );

	#if BUMPMAP
		float4 vBumpTexel = tex2D( g_tBumpSampler, i.vTexCoord0.xy );
		float3 vTangentNormal = ( 2.0f * vBumpTexel ) - 1.0f;
		vWorldNormal.xyz = mul( i.mTangentSpaceTranspose, vTangentNormal.xyz );
	#endif

	float4 result;

	// Staging test for weapon patterns
	if ( g_flEffectIndex == 2 )
	{
		float3 ppos = i.vModelSpacePos;
		float2 temp = 0;

		// 'Scaling' of texture to get it to map to the weapons
		// No Skewing, just move left to right
		if ( g_flSheenDirection == 0 )
		{
			temp.x = ppos.z;
			temp.y = ppos.y;
		}
		else if ( g_flSheenDirection == 1 )
		{
			temp.x = ppos.z;
			temp.y = ppos.x;
		}
		else
		{
			temp.x = ppos.y;
			temp.y = ppos.x;
		}

		temp.x -= ( g_flSheenMapMaskOffsetX );		// offset
		temp.y -= ( g_flSheenMapMaskOffsetY );		// offset

		temp.x /= g_flSheenMapMaskScaleX;	// scale
		temp.y /= g_flSheenMapMaskScaleY;

		temp.y = 1.0 - temp.y;

		// Sample Texture
		// Sample Mask
		//float4 patternTexel = tex2D( EnvmapSampler, temp );
		float4 patternTexel = tex2D( EnvmapSampler, i.vTexCoord0.xy );
		float4 maskTexel = tex2D( EnvmapMaskSampler, i.vTexCoord0.xy );

		//result.rgba = float4( patternTexel.xyz * 1.0, maskTexel.x * 1.0);		// 0.3 is a hack to preserve rimlight
		//result.rgba = float4( patternTexel.xyz, 1.0);		// 0.3 is a hack to preserve rimlight
		result.rgba = float4( 0.0,0.0,0.0,0.0);		// 0.3 is a hack to preserve rimlight
	}
	else
	{
		// generate a hard reflection in to the cube map
		float3 vEyeDir = -normalize(i.vWorldViewVector.xyz);
		float3 worldSpaceNormal, tangentSpaceNormal;
	
		tangentSpaceNormal = float3(0, 0, 1);
		worldSpaceNormal = normalize( mul( i.mTangentSpaceTranspose, tangentSpaceNormal ) );
	
		float3 vReflect = 2 * worldSpaceNormal * dot(worldSpaceNormal, vEyeDir) - vEyeDir;

		float3 envMapColor = float3( 0.0f, 0.0f, 0.0f );
		envMapColor = ENV_MAP_SCALE * texCUBE( EnvmapSampler, vReflect ) * g_cCloakColorTint.xyz;
		envMapColor *= 10.0f;

		// Sample the Mask
		float4 envmapMaskTexel;
		float2 temp = 0;
		float3 ppos = i.vModelSpacePos;

		//
		// skew the sampling based on sheen direction
		//if ( g_flSheenDirection == 0 )
		//{
		//	temp.x = ppos.z - ppos.y;
		//	temp.y = (ppos.x + ppos.y);
		//}
		//else if ( g_flSheenDirection == 1 )
		//{
		//	temp.x = ppos.x - ppos.z;
		//	temp.y = (ppos.y + ppos.z);
		//}
		//else
		//{
		//	temp.x = ppos.y - ppos.x;
		//	temp.y = (ppos.z + ppos.x);
		//}

		// No Skewing, just move left to right
		if ( g_flSheenDirection == 0 )
		{
			temp.x = ppos.z;
			temp.y = ppos.y;
		}
		else if ( g_flSheenDirection == 1 )
		{
			temp.x = ppos.z;
			temp.y = ppos.x;
		}
		else
		{
			temp.x = ppos.y;
			temp.y = ppos.x;
		}

		temp.x -= ( g_flSheenMapMaskOffsetX );		// offset
		temp.y -= ( g_flSheenMapMaskOffsetY );		// offset

		temp.x /= g_flSheenMapMaskScaleX;	// scale
		temp.y /= g_flSheenMapMaskScaleY;

		temp.y = 1.0 - temp.y;

		envmapMaskTexel = tex2D( EnvmapMaskSampler, temp );
	
		// Build result, only have alpha if there was value in the mask.
		// High alpha (white) will override the underlying texture while low to none will show model underneath
		//float4 result;
		//result.rgba = float4( envMapColor, envmapMaskTexel.x * g_cCloakColorTint.w );

		if ( g_flEffectIndex == 1 )
		{
			//float alpha = max( max( envmapMaskTexel.x, envmapMaskTexel.y), envmapMaskTexel.z );
			//result.rgba = float4( envMapColor.xyz * envmapMaskTexel.xyz, alpha );
		
			float alpha = max( max( envMapColor.x, envMapColor.y), envMapColor.z );
			result.rgba = float4( envMapColor.xyz * envmapMaskTexel.xyz, alpha * envmapMaskTexel.x );
			result.rgba = result.rgba * 1.8f;
		}
		else
		{
			float alpha = max( max( envMapColor.x, envMapColor.y), envMapColor.z );
			result.rgba = float4( envMapColor.xyz * envmapMaskTexel.xyz, alpha * envmapMaskTexel.x );
		}
	}

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
