//======= Copyright © 1996-2007, Valve Corporation, All rights reserved. ======
// STATIC: "CONVERT_TO_SRGB"			"0..0"
// STATIC: "CUBEMAP"					"0..1"
// STATIC: "SELFILLUM"					"0..1"
// STATIC: "SELFILLUMFRESNEL"			"0..1"
// STATIC: "FLASHLIGHT"					"0..1"
// STATIC: "LIGHTWARPTEXTURE"			"0..1"
// STATIC: "PHONGWARPTEXTURE"			"0..1"
// STATIC: "WRINKLEMAP"					"0..1"
// STATIC: "DETAIL_BLEND_MODE"          "0..6"
// STATIC: "DETAILTEXTURE"				"0..1"
// STATIC: "RIMLIGHT"					"0..1"
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"	[ps20b] [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"	[ps30]  [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..0"	[ps20b] [XBOX]
// STATIC: "FASTPATH_NOBUMP"            "0..1"
// STATIC: "BLENDTINTBYBASEALPHA"       "0..1"

// DYNAMIC: "WRITEWATERFOGTODESTALPHA"  "0..1"
// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "NUM_LIGHTS"				"0..4"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps30]
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"	[ps20b]
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"	[ps30]


// SKIP: ($PIXELFOGTYPE == 0) && ($WRITEWATERFOGTODESTALPHA != 0)

// blend mode doesn't matter if we only have one texture
// SKIP: (! $DETAILTEXTURE) && ( $DETAIL_BLEND_MODE != 0 )

// We don't care about flashlight depth unless the flashlight is on
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )

// Flashlight shadow filter mode is irrelevant if there is no flashlight
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 ) [ps20b]
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 ) [ps30]

// Only need self illum fresnel when self illum enabled
// SKIP: ( $SELFILLUM == 0 ) && ( $SELFILLUMFRESNEL == 1 )
// SKIP: ( $FLASHLIGHT == 1 ) && ( $SELFILLUMFRESNEL == 1 )
// SKIP: ( $FLASHLIGHT == 1 ) && ( $SELFILLUM == 1 )

// BlendTintByBaseAlpha and self illum and are opposing meanings for alpha channel
// SKIP: ( $BLENDTINTBYBASEALPHA ) && ( $SELFILLUM )

// fastpath means:
// no bumpmap
// basealphaenvmapmask (not inverted)
// no spec expmap
// no spectint
// no specwarp
// no rimlight
// no selfillum
// no detail
// no BlendTintByBaseAlpha

// SKIP: $FASTPATH_NOBUMP && ( $RIMLIGHT || $DETAILTEXTURE || $PHONGWARPTEXTURE || $SELFILLUM || $BLENDTINTBYBASEALPHA )



#include "common_flashlight_fxc.h"
#include "shader_constant_register_map.h"

const float4 g_SelfIllumTint_and_DetailBlendFactor	: register( PSREG_SELFILLUMTINT );
#if ( SELFILLUMFRESNEL == 1 )
const float4 g_SelfIllumScaleBiasExpBrightness		: register( PSREG_SELFILLUM_SCALE_BIAS_EXP );
#endif
const float4 g_DiffuseModulation					: register( PSREG_DIFFUSE_MODULATION );
const float4 g_EnvmapTint_ShadowTweaks				: register( PSREG_ENVMAP_TINT__SHADOW_TWEAKS );		// w controls spec mask
const float3 cAmbientCube[6]						: register( PSREG_AMBIENT_CUBE );
const float4 g_EnvMapFresnel						: register( PSREG_ENVMAP_FRESNEL__SELFILLUMMASK );	// x is envmap fresnel ... w is selfillummask control
const float4 g_EyePos_SpecExponent					: register( PSREG_EYEPOS_SPEC_EXPONENT );
const float4 g_FogParams							: register( PSREG_FOG_PARAMS );
const float4 g_FlashlightAttenuationFactors_RimMask	: register( PSREG_FLASHLIGHT_ATTENUATION );			// On non-flashlight pass, x has rim mask control
const float4 g_FlashlightPos_RimBoost				: register( PSREG_FLASHLIGHT_POSITION_RIM_BOOST );
const float4x4 g_FlashlightWorldToTexture			: register( PSREG_FLASHLIGHT_TO_WORLD_TEXTURE );
const float4 g_FresnelSpecParams					: register( PSREG_FRESNEL_SPEC_PARAMS );			// xyz are fresnel, w is specular boost
const float4 g_SpecularRimParams					: register( PSREG_SPEC_RIM_PARAMS );				// xyz are specular tint color, w is rim power
PixelShaderLightInfo cLightInfo[3]					: register( PSREG_LIGHT_INFO_ARRAY );				// 2 registers each - 6 registers total (4th light spread across w's)

// TODO: give this a better name.  For now, I don't want to touch shader_constant_register_map.h since I don't want to trigger a recompile of everything...
const float4 g_ShaderControls						: register( PSREG_CONSTANT_27 );					// x is basemap alpgha phong mask, y is 1 - blendtintbybasealpha, z is tint overlay amount, w controls "INVERTPHONGMASK"
#define g_FlashlightPos					g_FlashlightPos_RimBoost.xyz
#define	g_fRimBoost						g_FlashlightPos_RimBoost.w
#define g_FresnelRanges					g_FresnelSpecParams.xyz
#define g_SpecularBoost					g_FresnelSpecParams.w
#define g_SpecularTint					g_SpecularRimParams.xyz
#define g_RimExponent					g_SpecularRimParams.w
#define g_FlashlightAttenuationFactors	g_FlashlightAttenuationFactors_RimMask
#define g_RimMaskControl				g_FlashlightAttenuationFactors_RimMask.x
#define g_SelfIllumMaskControl			g_EnvMapFresnel.w
#define g_fBaseMapAlphaPhongMask		g_ShaderControls.x
#define g_fTintReplacementControl		g_ShaderControls.z
#define g_fInvertPhongMask				g_ShaderControls.w

sampler BaseTextureSampler		: register( s0 );	// Base map, selfillum in alpha
sampler SpecularWarpSampler		: register( s1 );	// Specular warp sampler (for iridescence etc)
sampler DiffuseWarpSampler		: register( s2 );	// Lighting warp sampler (1D texture for diffuse lighting modification)
sampler NormalMapSampler		: register( s3 );	// Normal map, specular mask in alpha
sampler ShadowDepthSampler		: register( s4 );	// Flashlight shadow depth map sampler
sampler NormalizeRandRotSampler	: register( s5 );	// Normalization / RandomRotation samplers
sampler FlashlightSampler		: register( s6 );	// Flashlight cookie
sampler SpecExponentSampler		: register( s7 );	// Specular exponent map
sampler EnvmapSampler			: register( s8 );	// Cubic environment map

#if WRINKLEMAP
sampler WrinkleSampler			: register( s9 );	// Compression base
sampler StretchSampler			: register( s10 );	// Expansion base
sampler NormalWrinkleSampler	: register( s11 );	// Compression base
sampler NormalStretchSampler	: register( s12 );	// Expansion base
#endif

#if DETAILTEXTURE
sampler DetailSampler			: register( s13 );	// detail texture
#endif

sampler SelfIllumMaskSampler	: register( s14 );	// selfillummask


struct PS_INPUT
{
	float4 baseTexCoordDetailTexCoord							: TEXCOORD0; // xy=base zw=detail
	float3 lightAtten											: TEXCOORD1; // Scalar light attenuation factors for FOUR lights
	float3 worldVertToEyeVectorXYZ_tangentSpaceVertToEyeVectorZ	: TEXCOORD2;
	float3x3 tangentSpaceTranspose								: TEXCOORD3;
	//	     second row											: TEXCOORD4;
	//	     third row											: TEXCOORD5;
	float4 worldPos_atten3										: TEXCOORD6;
	float4 projPos_fWrinkleWeight								: TEXCOORD7;
};



float4 main( PS_INPUT i ) : COLOR
{
	bool bWrinkleMap = WRINKLEMAP ? true : false;
	bool bDoDiffuseWarp = LIGHTWARPTEXTURE ? true : false;
	bool bDoSpecularWarp = PHONGWARPTEXTURE ? true : false;
	bool bDoAmbientOcclusion = false;
	bool bFlashlight = (FLASHLIGHT!=0) ? true : false;
	bool bSelfIllum = SELFILLUM ? true : false;
	bool bDoRimLighting = RIMLIGHT ? true : false;
	bool bCubemap = CUBEMAP ? true : false;
	bool bBlendTintByBaseAlpha = BLENDTINTBYBASEALPHA ? true : false;
	int nNumLights = NUM_LIGHTS;

	// Unpacking for convenience
	float fWrinkleWeight = i.projPos_fWrinkleWeight.w;
	float3 vProjPos = i.projPos_fWrinkleWeight.xyz;
	float3 vWorldPos = i.worldPos_atten3.xyz;
	float  atten3 = i.worldPos_atten3.w;

	float4 vLightAtten = float4( i.lightAtten, atten3 );

#if WRINKLEMAP
	float flWrinkleAmount = saturate( -fWrinkleWeight );					// One of these two is zero
	float flStretchAmount = saturate(  fWrinkleWeight );					// while the other is in the 0..1 range

	float flTextureAmount = 1.0f - flWrinkleAmount - flStretchAmount;		// These should sum to one
#endif

	float4 baseColor = tex2D( BaseTextureSampler, i.baseTexCoordDetailTexCoord.xy );
#if WRINKLEMAP
	float4 wrinkleColor = tex2D( WrinkleSampler, i.baseTexCoordDetailTexCoord.xy );
	float4 stretchColor = tex2D( StretchSampler, i.baseTexCoordDetailTexCoord.xy );

	// Apply wrinkle blend to only RGB.  Alpha comes from the base texture
	baseColor.rgb = flTextureAmount * baseColor + flWrinkleAmount * wrinkleColor +  flStretchAmount * stretchColor;
#endif

#if DETAILTEXTURE
	float4 detailColor = tex2D( DetailSampler, i.baseTexCoordDetailTexCoord.zw );
	baseColor = TextureCombine( baseColor, detailColor, DETAIL_BLEND_MODE, g_SelfIllumTint_and_DetailBlendFactor.w );
#endif

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, vWorldPos.z, vProjPos.z );

	float3 vEyeDir = normalize(i.worldVertToEyeVectorXYZ_tangentSpaceVertToEyeVectorZ.xyz);
	float3 vRimAmbientCubeColor = PixelShaderAmbientLight(vEyeDir, cAmbientCube);

	float3 worldSpaceNormal, tangentSpaceNormal;
	float fSpecMask = 1.0f;
	float4 normalTexel = tex2D( NormalMapSampler, i.baseTexCoordDetailTexCoord.xy );

#if WRINKLEMAP
	float4 wrinkleNormal = tex2D( NormalWrinkleSampler,	i.baseTexCoordDetailTexCoord.xy );
	float4 stretchNormal = tex2D( NormalStretchSampler,	i.baseTexCoordDetailTexCoord.xy );
	normalTexel = flTextureAmount * normalTexel + flWrinkleAmount * wrinkleNormal + flStretchAmount * stretchNormal;
#endif

#if (FASTPATH_NOBUMP == 0 )
	tangentSpaceNormal = lerp( 2.0f * normalTexel.xyz - 1.0f, float3(0, 0, 1), g_fBaseMapAlphaPhongMask );
	fSpecMask = lerp( normalTexel.a, baseColor.a, g_fBaseMapAlphaPhongMask );
#else
	tangentSpaceNormal = float3(0, 0, 1);
	fSpecMask = baseColor.a;
#endif

	// We need a normal if we're doing any lighting
	worldSpaceNormal = normalize( mul( i.tangentSpaceTranspose, tangentSpaceNormal ) );

	float fFresnelRanges = Fresnel( worldSpaceNormal, vEyeDir, g_FresnelRanges );
	float fRimFresnel = Fresnel4( worldSpaceNormal, vEyeDir );

	// Break down reflect so that we can share dot(worldSpaceNormal,vEyeDir) with fresnel terms
	float3 vReflect = 2 * worldSpaceNormal * dot(worldSpaceNormal, vEyeDir) - vEyeDir;

	float3 diffuseLighting = float3( 1.0f, 1.0f, 1.0f );
	float3 envMapColor = float3( 0.0f, 0.0f, 0.0f );
	if( !bFlashlight )
	{
		// Summation of diffuse illumination from all local lights
		diffuseLighting = PixelShaderDoLighting( vWorldPos, worldSpaceNormal,
			float3( 0.0f, 0.0f, 0.0f ), false, true, vLightAtten,
			cAmbientCube, NormalizeRandRotSampler, nNumLights, cLightInfo, true,

			// These parameters aren't passed by generic shaders:
			false, 1.0f,
			bDoDiffuseWarp, DiffuseWarpSampler );

		if( bCubemap )
		{
			// Mask is either normal map alpha or base map alpha
#if ( SELFILLUMFRESNEL == 1 ) // This is to match the 2.0 version of vertexlitgeneric
			float fEnvMapMask = lerp( baseColor.a, g_fInvertPhongMask, g_EnvmapTint_ShadowTweaks.w );
#else
			float fEnvMapMask = lerp( baseColor.a, fSpecMask, g_EnvmapTint_ShadowTweaks.w );
#endif

			envMapColor = (ENV_MAP_SCALE *
							lerp(1, fFresnelRanges, g_EnvMapFresnel.x) *
							lerp(fEnvMapMask, 1-fEnvMapMask, g_fInvertPhongMask)) *
							texCUBE( EnvmapSampler, vReflect ) *
							g_EnvmapTint_ShadowTweaks.xyz;
		}
	}
	
	float3 specularLighting = float3( 0.0f, 0.0f, 0.0f );
	float3 rimLighting = float3( 0.0f, 0.0f, 0.0f );

	float3 vSpecularTint = 1;
	float fRimMask = 0;
	float fSpecExp = 1;

#if ( FASTPATH_NOBUMP == 0 )
	float4 vSpecExpMap = tex2D( SpecExponentSampler, i.baseTexCoordDetailTexCoord.xy );
	
	if ( !bFlashlight )
	{
		fRimMask = lerp( 1.0f, vSpecExpMap.a, g_RimMaskControl );						// Select rim mask
	}

	// If the exponent passed in as a constant is zero, use the value from the map as the exponent
#if defined( _X360 )
	[flatten]
#endif
	
	fSpecExp = (g_EyePos_SpecExponent.w >= 0.0) ? g_EyePos_SpecExponent.w : (1.0f + 149.0f * vSpecExpMap.r);

	// If constant tint is negative, tint with albedo, based upon scalar tint map
#if defined( _X360 )
	[flatten]
#endif
	vSpecularTint = lerp( float3(1.0f, 1.0f, 1.0f), baseColor.rgb, vSpecExpMap.g );
	vSpecularTint = (g_SpecularTint.r >= 0.0) ? g_SpecularTint.rgb : vSpecularTint;
		
#else
	fSpecExp = max(g_EyePos_SpecExponent.w, 0);
#endif

	float3 albedo = baseColor.rgb;

	if ( !bFlashlight )
	{
		// Summation of specular from all local lights besides the flashlight
		PixelShaderDoSpecularLighting( vWorldPos, worldSpaceNormal,
			fSpecExp, vEyeDir, vLightAtten,
			nNumLights, cLightInfo, false, 1.0f, bDoSpecularWarp,
			SpecularWarpSampler, fFresnelRanges, bDoRimLighting, g_RimExponent,

			// Outputs
			specularLighting, rimLighting );
	}
	else
	{
		float4 flashlightSpacePosition = mul( float4( vWorldPos, 1.0f ), g_FlashlightWorldToTexture );

		DoSpecularFlashlight( g_FlashlightPos, vWorldPos, flashlightSpacePosition, worldSpaceNormal, 
			g_FlashlightAttenuationFactors.xyz, g_FlashlightAttenuationFactors.w,
			FlashlightSampler, ShadowDepthSampler, NormalizeRandRotSampler, FLASHLIGHTDEPTHFILTERMODE, FLASHLIGHTSHADOWS, true, vProjPos.xy / vProjPos.z,
			fSpecExp, vEyeDir, bDoSpecularWarp, SpecularWarpSampler, fFresnelRanges, g_EnvmapTint_ShadowTweaks,

			// These two values are output
			diffuseLighting, specularLighting );
	}

	// If we didn't already apply Fresnel to specular warp, modulate the specular
	if ( !bDoSpecularWarp )
		fSpecMask *= fFresnelRanges;

	// Modulate with spec mask, boost and tint
	specularLighting *= fSpecMask * g_SpecularBoost;

	if (bBlendTintByBaseAlpha)
	{
		float3 tintedColor = albedo * g_DiffuseModulation.rgb;
		tintedColor = lerp(tintedColor, g_DiffuseModulation.rgb, g_fTintReplacementControl);
		albedo = lerp(albedo, tintedColor, baseColor.a);
	}
	else
	{
		albedo = albedo * g_DiffuseModulation.rgb;
	}


	float3 diffuseComponent = albedo * diffuseLighting;
	if ( bSelfIllum && !bFlashlight )
	{
#if ( SELFILLUMFRESNEL == 1 ) // To free up the constant register...see top of file
		// This will apply a Fresnel term based on the vertex normal (not the per-pixel normal!) to help fake and internal glow look
		float3 vVertexNormal = normalize( float3( i.tangentSpaceTranspose[0].z, i.tangentSpaceTranspose[1].z, i.tangentSpaceTranspose[2].z ) );
		float flSelfIllumFresnel = ( pow( saturate( dot( vVertexNormal.xyz, vEyeDir.xyz ) ), g_SelfIllumScaleBiasExpBrightness.z ) * g_SelfIllumScaleBiasExpBrightness.x ) + g_SelfIllumScaleBiasExpBrightness.y;
		diffuseComponent = lerp( diffuseComponent, g_SelfIllumTint_and_DetailBlendFactor.rgb * albedo * g_SelfIllumScaleBiasExpBrightness.w, baseColor.a * saturate( flSelfIllumFresnel ) );
#else
		float3 vSelfIllumMask = tex2D( SelfIllumMaskSampler, i.baseTexCoordDetailTexCoord.xy );
		vSelfIllumMask = lerp( baseColor.aaa, vSelfIllumMask, g_SelfIllumMaskControl );
		diffuseComponent = lerp( diffuseComponent, g_SelfIllumTint_and_DetailBlendFactor.rgb * albedo, vSelfIllumMask );
#endif

		diffuseComponent = max( 0.0f, diffuseComponent );
	}

#if DETAILTEXTURE
	diffuseComponent = TextureCombinePostLighting( diffuseComponent, detailColor, 
		DETAIL_BLEND_MODE, g_SelfIllumTint_and_DetailBlendFactor.w );
#endif

	if ( bDoRimLighting && !bFlashlight )
	{
		float fRimMultiply = fRimMask * fRimFresnel; // both unit range: [0, 1]
		
		// Add in rim light modulated with tint, mask and traditional Fresnel (not using Fresnel ranges)
		rimLighting *= fRimMultiply;

		// Fold rim lighting into specular term by using the max so that we don't really add light twice...
		specularLighting = max( specularLighting, rimLighting );

		// Add in view-ray lookup from ambient cube
		specularLighting += (vRimAmbientCubeColor * g_fRimBoost) * saturate(fRimMultiply * worldSpaceNormal.z);
	}

	float3 result = specularLighting*vSpecularTint + envMapColor + diffuseComponent;

#if WRITEWATERFOGTODESTALPHA && ( PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT )
	float alpha = fogFactor;
#else
	float alpha = g_DiffuseModulation.a;
	if ( !bSelfIllum && !bBlendTintByBaseAlpha )
	{
		alpha = lerp( baseColor.a * alpha, alpha, g_fBaseMapAlphaPhongMask );
	}
#endif

	bool bWriteDepthToAlpha = ( WRITE_DEPTH_TO_DESTALPHA != 0 ) && ( WRITEWATERFOGTODESTALPHA == 0 );

	//FIXME: need to take dowaterfog into consideration
	return FinalOutput( float4( result, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, vProjPos.z );
}
