//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps30][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// STATIC:  "LIGHTWARPTEXTURE"			"0..1"
// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITEWATERFOGTODESTALPHA"	"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..2"	[ps20]
// DYNAMIC: "NUM_LIGHTS"				"0..4"	[ps20b]
// DYNAMIC: "NUM_LIGHTS"				"0..4"	[ps30]


#include "shader_constant_register_map.h"

sampler BaseSampler					: register( s0 );	// Base map
sampler DiffuseWarpSampler			: register( s1 );	// 1D texture for diffuse lighting modification
sampler RefractSampler				: register( s2 );	// Refraction map copied from back buffer
sampler NormalSampler				: register( s3 );	// Normal map
sampler SpecExponentSampler			: register( s4 );	// Flashlight cookie
sampler NormalizeSampler			: register( s5 );	// Normalization cube map

const float3x3 g_ViewProj			: register( c0 );	// 1st row of Projection matrix
//                                              c1		// 2nd row
//                                              c2		// 4th row
const float2 g_CloakControl			: register( c3 );	// { refract amount, cloak, ?, ? }
const float3 cAmbientCube[6]		: register( PSREG_AMBIENT_CUBE );
const float4 g_EyePos_SpecExponent	: register( PSREG_EYEPOS_SPEC_EXPONENT );
const float4 g_FogParams			: register( PSREG_FOG_PARAMS );
const float4 g_FlashlightAttenuationFactors_RimMask	: register( PSREG_FLASHLIGHT_ATTENUATION );			// On non-flashlight pass, x has rim mask control
const float4 g_RimBoost				: register( PSREG_FLASHLIGHT_POSITION_RIM_BOOST );
const float4 g_FresnelSpecParams	: register( PSREG_FRESNEL_SPEC_PARAMS );		// xyz are fresnel, w is specular boost
const float4 g_SpecularRimParams	: register( PSREG_SPEC_RIM_PARAMS );			// xyz are specular tint color, w is rim power
PixelShaderLightInfo cLightInfo[3]	: register( PSREG_LIGHT_INFO_ARRAY );			// 2 registers each - 6 registers total

#define	g_fRimBoost						g_RimBoost.w
#define g_FresnelRanges					g_FresnelSpecParams.xyz
#define g_SpecularBoost					g_FresnelSpecParams.w
#define g_SpecularTint					g_SpecularRimParams.xyz
#define g_RimExponent					g_SpecularRimParams.w
#define g_FlashlightAttenuationFactors	g_FlashlightAttenuationFactors_RimMask
#define g_RimMaskControl				g_FlashlightAttenuationFactors_RimMask.x

// 8 2D Poisson offsets (designed to use .xy and .wz swizzles (not .zw)
static const float4 gPoissonOffset[4] = {	float4 (-0.0876f,  0.9703f,  0.5651f,  0.4802f ),
											float4 ( 0.1851f,  0.1580f, -0.0617f, -0.2616f ),
											float4 (-0.5477f, -0.6603f,  0.0711f, -0.5325f ),
											float4 (-0.0751f, -0.8954f,  0.4054f,  0.6384f ) };

struct PS_INPUT
{
	float2 vBaseTexCoord			: TEXCOORD0;
	float3x3 tangentSpaceTranspose	: TEXCOORD1;
	//	     second row				: TEXCOORD2;
	//	     third row				: TEXCOORD3;
	float3 worldPos					: TEXCOORD4;
	float3 projPos					: TEXCOORD5;
	float4 lightAtten				: TEXCOORD6;
};

float4 main( PS_INPUT i ) : COLOR
{
	float3 vSpecular = float3( 0.0f, 0.0f, 0.0f );
	bool bDoDiffuseWarp = LIGHTWARPTEXTURE ? true : false;
	int nNumLights = NUM_LIGHTS;

	// Base color
	float4 albedo = tex2D( BaseSampler, i.vBaseTexCoord );

	// Load normal and expand range
	float4 vNormalSample = tex2D( NormalSampler, i.vBaseTexCoord );
	float3 tangentSpaceNormal = 2.0f * vNormalSample.xyz - 1.0f;

	// We need a world space normal if we're doing any lighting
	float3 vWorldNormal = normalize( mul( i.tangentSpaceTranspose, tangentSpaceNormal ) );
	float3 vWorldEyeDir = normalize( g_EyePos_SpecExponent.xyz - i.worldPos );

	// Vanilla 1-(N.V) fresnel term used later in transition lerp
	float fresnel = 1-saturate( dot( vWorldNormal, vWorldEyeDir ) );	

	// Summation of diffuse illumination from all local lights
	float3 diffuseLighting = PixelShaderDoLighting( i.worldPos, vWorldNormal,
													float3( 0.0f, 0.0f, 0.0f ), false,
													true, i.lightAtten, cAmbientCube, NormalizeSampler,
													nNumLights, cLightInfo, true, false, 1.0f,
													bDoDiffuseWarp, DiffuseWarpSampler );





	// Transform world space normal into clip space and project
	float2 vProjNormal;
	vProjNormal.x = dot( vWorldNormal, g_ViewProj[0] );	// 1st row
	vProjNormal.y = dot( vWorldNormal, g_ViewProj[1] );	// 2nd row

	// Compute coordinates for sampling refraction
	float2 vRefractTexCoordNoWarp = i.projPos.xy / i.projPos.z;
	float2 vRefractTexCoord = vProjNormal.xy;
	float scale = lerp( g_CloakControl.x, 0.0f, g_CloakControl.y );
	vRefractTexCoord *= scale;
	vRefractTexCoord += vRefractTexCoordNoWarp;

#ifdef SHADER_MODEL_PS_2_0
	float3 vRefract = tex2D( RefractSampler, vRefractTexCoordNoWarp );
#endif

	// Extra refraction rays, specular, rim etc are only done on ps_2_b
#if defined( SHADER_MODEL_PS_2_B ) || defined( SHADER_MODEL_PS_3_0 )
	// Blur by scalable Poisson filter
	float fBlurAmount = lerp( 0.05f, 0.0f, g_CloakControl.y );
	float3 vRefract = tex2D( RefractSampler, vRefractTexCoord );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[0].xy * fBlurAmount );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[0].wz * fBlurAmount );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[1].xy * fBlurAmount );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[1].wz * fBlurAmount );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[2].xy * fBlurAmount );
	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[2].wz * fBlurAmount );

// We're right at the hairy edge of constant register usage and hence have to drop these taps...
//	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[3].xy * fBlurAmount );
//	vRefract += tex2D( RefractSampler, vRefractTexCoord + gPoissonOffset[3].wz * fBlurAmount );
	vRefract /= 7.0f;

	float3 rimLighting = float3( 0.0f, 0.0f, 0.0f );
	float3 specularLighting = float3( 0.0f, 0.0f, 0.0f );
	float fSpecExp = g_EyePos_SpecExponent.w;
	float fSpecMask = vNormalSample.a;

	float4 vSpecExpMap = tex2D( SpecExponentSampler, i.vBaseTexCoord );
	float fSpecExpMap = vSpecExpMap.r;
	float fRimMask = 1.0f;//lerp( 1.0f, vSpecExpMap.a, g_RimMaskControl );						// Select rim mask
	float3 vSpecularTint;

	// If the exponent passed in as a constant is zero, use the value from the map as the exponent
	if ( fSpecExp == 0 )
		fSpecExp = 1.0f - fSpecExpMap + 150.0f * fSpecExpMap;

	// If constant tint is negative, tint with albedo, based upon scalar tint map
	if ( g_SpecularTint.r == -1 )
		vSpecularTint = lerp( float3(1.0f, 1.0f, 1.0f), albedo, vSpecExpMap.g );
	else
		vSpecularTint = g_SpecularTint.rgb;

	// Fresnel to match regular specular lighting
	float fFresnelRanges = Fresnel( vWorldNormal, vWorldEyeDir, g_FresnelRanges );

	// Summation of specular from all local lights besides the flashlight
	PixelShaderDoSpecularLighting( i.worldPos, vWorldNormal, fSpecExp, vWorldEyeDir,
								   i.lightAtten, nNumLights, cLightInfo, false, 1.0f, false,
								   NormalizeSampler, 1.0f, true, g_RimExponent,

								   // Outputs
								   specularLighting, rimLighting );

	// Modulate with spec mask, boost, tint and fresnel ranges
	specularLighting *= fSpecMask * g_SpecularBoost * fFresnelRanges * vSpecularTint;

	float fRimFresnel = Fresnel4( vWorldNormal, vWorldEyeDir );

	// Add in rim light modulated with tint, mask and traditional Fresnel (not using Fresnel ranges)
	rimLighting *= vSpecularTint * fRimMask * fRimFresnel;

	// Fold rim lighting into specular term by using the max so that we don't really add light twice...
	specularLighting = max (specularLighting, rimLighting);

	// Add in view-ray lookup from ambient cube
	specularLighting += fRimFresnel * fRimMask * vSpecularTint /* g_fRimBoost */ * PixelShaderAmbientLight( vWorldEyeDir, cAmbientCube) * saturate(dot(vWorldNormal, float3(0, 0 , 1)) );

	float tintLerpFactor = saturate(lerp( 1, fresnel-1.1, saturate(g_CloakControl.y)));
	tintLerpFactor = smoothstep( 0.4f, 0.425f, tintLerpFactor );
	float3 vTintedRefract = lerp( vRefract, albedo * vRefract, 0.7f );
	vRefract = lerp( vRefract, vTintedRefract, tintLerpFactor );

	vSpecular = specularLighting * smoothstep( 0.98, 0.8, saturate(g_CloakControl.y ));
#endif

	// Blend refraction component with diffusely lit model
	float diffuseLerpFactor = saturate(lerp( 1, fresnel - 1.35, saturate(g_CloakControl.y)));
	diffuseLerpFactor = smoothstep( 0.4f, 0.425f, diffuseLerpFactor );

	float3 fDiffuse = lerp( vRefract, albedo * diffuseLighting, diffuseLerpFactor );
	float3 result =  fDiffuse + vSpecular;

	float alpha = 1.0f;

	// Emulate LinearColorToHDROutput() when uncloaked
	result = lerp( result.xyz * LINEAR_LIGHT_SCALE, result, saturate(g_CloakControl.y) );

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos.z, i.projPos.z );
	
#if WRITEWATERFOGTODESTALPHA && (PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	alpha = fogFactor;
#endif

	return FinalOutput( float4( result, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE );
}
