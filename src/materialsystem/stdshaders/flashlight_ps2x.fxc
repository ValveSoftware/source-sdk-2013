//====== Copyright c 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================



// STATIC: "NORMALMAP"					"0..2"
// STATIC: "NORMALMAP2"					"0..1"
// STATIC: "WORLDVERTEXTRANSITION"		"0..1"
// STATIC: "SEAMLESS"               	"0..1"
// STATIC: "DETAILTEXTURE"             	"0..1"
// STATIC: "DETAIL_BLEND_MODE"			"0..1"
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"	[ps20b] [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..0"	[ps20b] [XBOX]

// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"	[ps20b]

// SKIP: !$WORLDVERTEXTRANSITION && $NORMALMAP2
// SKIP: !$NORMALMAP && $NORMALMAP2
// SKIP: !$DETAILTEXTURE && ( $DETAIL_BLEND_MODE != 0 )

#include "shader_constant_register_map.h"
#include "common_flashlight_fxc.h"
#include "common_lightmappedgeneric_fxc.h"

const float4 g_vShadowTweaks				: register( PSREG_ENVMAP_TINT__SHADOW_TWEAKS );
const float4 g_FogParams					: register( PSREG_FOG_PARAMS );
const float4 g_EyePos						: register( PSREG_EYEPOS_SPEC_EXPONENT );
const float4 g_FlashlightAttenuation		: register( PSREG_FLASHLIGHT_ATTENUATION );
const float4 g_DetailConstants				: register( c0 );

sampler SpotSampler							: register( s0 );
sampler BaseTextureSampler					: register( s1 );
sampler NormalizingCubemapSampler			: register( s2 );

// use a normalizing cube map here if we aren't normal mapping
sampler BumpMapSampler						: register( s3 );
sampler BaseTextureSampler2					: register( s4 );

#ifdef WORLDVERTEXTRANSITION
sampler NormalMap2Sampler					: register( s6 );
#endif

#if DETAILTEXTURE
sampler DetailSampler						: register( s8 );
#endif

#if FLASHLIGHTSHADOWS && ( defined( SHADER_MODEL_PS_2_B ) || defined( SHADER_MODEL_PS_3_0 ) )
sampler RandomRotationSampler				: register( s5 );	//  Random rotation sampler
sampler FlashlightDepthSampler				: register( s7 );
#endif

struct PS_INPUT
{
	float4 spotTexCoord						: TEXCOORD0;
#if SEAMLESS
	float3 SeamlessTexCoord					: TEXCOORD1;
#else
	float2 baseTexCoord						: TEXCOORD1;
#endif
#if NORMALMAP
	float3 tangentPosToLightVector			: TEXCOORD2;
	float2 normalMapTexCoord				: TEXCOORD3;
#else
	float3 worldPosToLightVector			: TEXCOORD2;
	float3 normal							: TEXCOORD3;
#endif

	float2 detailCoords						: TEXCOORD4;
	float4 worldPos_worldTransition			: TEXCOORD5;	
	float3 projPos							: TEXCOORD6;
	float4 fogFactorW						: TEXCOORD7;
};



float4 SampleNormal( sampler s, PS_INPUT i )
{
#if SEAMLESS
	float4 szy=tex2D( s, i.SeamlessTexCoord.zy );
	float4 sxz=tex2D( s, i.SeamlessTexCoord.xz );
	float4 syx=tex2D( s, i.SeamlessTexCoord.xy );
	return i.fogFactorW.r*szy + i.fogFactorW.g*sxz + i.fogFactorW.b*syx;
#else
#if NORMALMAP
	return tex2D( s, i.normalMapTexCoord.xy);
#else
	return float4(0,0,1,1);
#endif
#endif

}

float4 main( PS_INPUT i ) : COLOR
{
	bool bBase2 = WORLDVERTEXTRANSITION ? true : false;
	bool bBump  = (NORMALMAP != 0) ? true : false;

	// Do spot stuff early since we can bail out
	float3 spotColor = float3(0,0,0);
	float3 vProjCoords = i.spotTexCoord.xyz / i.spotTexCoord.w;

#if ( defined( _X360 ) )

	float3 ltz = vProjCoords.xyz < float3( 0.0f, 0.0f, 0.0f );
	float3 gto = vProjCoords.xyz > float3( 1.0f, 1.0f, 1.0f );

	[branch]
	if ( dot(ltz + gto, float3(1,1,1)) > 0 )
	{
		clip (-1);
		return float4(0,0,0,0);
	}
	else
	{
		spotColor = tex2D( SpotSampler, vProjCoords );

		[branch]
		if ( dot(spotColor.xyz, float3(1,1,1)) <= 0 )
		{
			clip(-1);
			return float4(0,0,0,0);
		}
		else
		{
#else
	spotColor = tex2D( SpotSampler, vProjCoords );
#endif

	float4 baseColor = 0.0f;
	float4 baseColor2 = 0.0f;
	float4 vNormal = float4(0, 0, 1, 1);
	float3 baseTexCoords = float3(0,0,0);

#if SEAMLESS
	baseTexCoords = i.SeamlessTexCoord.xyz;
#else
	baseTexCoords.xy = i.baseTexCoord.xy;
#endif

	GetBaseTextureAndNormal( BaseTextureSampler, BaseTextureSampler2, BumpMapSampler, bBase2, bBump, baseTexCoords, i.fogFactorW.xyz, baseColor, baseColor2, vNormal );

#if WORLDVERTEXTRANSITION
	float lerpAlpha = 1-i.worldPos_worldTransition.a;
#endif

#if ( NORMALMAP == 0 )
	vNormal.xyz = normalize( i.normal.xyz );
#endif

#if ( NORMALMAP == 1 )
	vNormal.xyz = vNormal.xyz * 2.0f - 1.0f;		// signed

#	if NORMALMAP2
	float3 normal2 = SampleNormal( NormalMap2Sampler, i ) * 2.0f - 1.0f;
	vNormal.xyz = lerp( normal2, vNormal.xyz, lerpAlpha );
#	endif
#endif

// ssbump
#if ( NORMALMAP == 2 )

#	if NORMALMAP2
	float3 normal2 = SampleNormal( NormalMap2Sampler, i );
	vNormal.xyz = lerp( normal2, vNormal.xyz, lerpAlpha );
#	endif
#else
	// Normalize normal after all of the lerps above (including the tri/bilinear texel fetches)
	vNormal.xyz = normalize( vNormal.xyz );
#endif

	spotColor.rgb *= cFlashlightColor.rgb;

	// Compute per-pixel distance attenuation
	float3 delta = g_EyePos.xyz - i.worldPos_worldTransition.xyz;
	float distSquared = dot( delta, delta );
	float dist = sqrt( distSquared );
	float farZ = g_FlashlightAttenuation.w;
	float endFalloffFactor = RemapValClamped( dist, farZ, 0.6f * farZ, 0.0f, 1.0f );
	float flAtten = saturate(endFalloffFactor * dot( g_FlashlightAttenuation.xyz, float3( 1.0f, 1.0f/dist, 1.0f/distSquared ) ) );

#if FLASHLIGHTSHADOWS && ( defined( SHADER_MODEL_PS_2_B ) || defined( SHADER_MODEL_PS_3_0 ) )
	float flShadow = DoFlashlightShadow( FlashlightDepthSampler, RandomRotationSampler, vProjCoords, i.projPos.xy / i.projPos.z, FLASHLIGHTDEPTHFILTERMODE, g_vShadowTweaks, false );
	float flAttenuated = lerp( flShadow, 1.0f, g_vShadowTweaks.y );	// Blend between fully attenuated and not attenuated
	flShadow = saturate(lerp( flAttenuated, flShadow, flAtten ));	// Blend between shadow and above, according to light attenuation
	spotColor *= flShadow;
#endif

#if WORLDVERTEXTRANSITION
	baseColor.xyz = lerp( baseColor2.xyz, baseColor.xyz, lerpAlpha );
#endif

#if DETAILTEXTURE
	float4 detailColor = float4( g_DetailConstants.xyz, 1.0f ) * tex2D( DetailSampler, i.detailCoords );
	float4 vBase = TextureCombine( float4(baseColor.xyz, 1.0f), detailColor, DETAIL_BLEND_MODE, g_DetailConstants.w );
	baseColor.xyz = vBase.xyz;
#endif

#if NORMALMAP == 0
	float3 worldPosToLightVector = texCUBE( NormalizingCubemapSampler, i.worldPosToLightVector ) * 2.0f - 1.0f;
	float nDotL = dot( worldPosToLightVector, vNormal.xyz );
#endif

#if NORMALMAP == 1
	// flashlightfixme: wrap this!
	float3 tangentPosToLightVector = texCUBE( NormalizingCubemapSampler, i.tangentPosToLightVector ) * 2.0f - 1.0f;
	float nDotL = dot( tangentPosToLightVector, vNormal.xyz );
#endif

#if NORMALMAP == 2
	float3 tangentPosToLightVector = normalize( i.tangentPosToLightVector );

	float nDotL = 
		vNormal.x*dot( tangentPosToLightVector, bumpBasis[0]) +
		vNormal.y*dot( tangentPosToLightVector, bumpBasis[1]) +
		vNormal.z*dot( tangentPosToLightVector, bumpBasis[2]);
#endif

	float3 outColor;
	outColor = spotColor * baseColor.xyz * saturate( nDotL );
	outColor *= flAtten;

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_worldTransition.z, i.projPos.z );
	return FinalOutput( float4(outColor, baseColor.a) , fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );

	// so we can jump over all of the above
#if ( defined( _X360 ) )
		}
	}
#endif

}
