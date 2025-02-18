//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. ===========================

// STATIC: "FLASHLIGHT"					"0..1"
// STATIC: "LIGHTWARPTEXTURE"			"0..1"

// STATIC: "SPHERETEXKILLCOMBO"			"0..1"					[ps20b]
// STATIC: "SPHERETEXKILLCOMBO"			"0..1"					[ps30]

// STATIC: "RAYTRACESPHERE"				"0..1"					[ps20b]
// STATIC: "RAYTRACESPHERE"				"0..1"					[ps30]

// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"					[ps20b] [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..2"					[ps30]  [PC]
// STATIC: "FLASHLIGHTDEPTHFILTERMODE"	"0..0"					[ps20b] [XBOX]

// DYNAMIC: "NUM_LIGHTS"				"0..2"					[ps20]
// DYNAMIC: "NUM_LIGHTS"				"0..4"					[ps20b]
// DYNAMIC: "NUM_LIGHTS"				"0..4"					[ps30]

// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"					[ps20b]
// DYNAMIC: "FLASHLIGHTSHADOWS"			"0..1"					[ps30]

// We don't use other lights when doing the flashlight
// SKIP: ( $FLASHLIGHT != 0 ) && ( $NUM_LIGHTS > 0 )

// We don't care about flashlight depth unless the flashlight is on
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )	[ps20b]
// SKIP: ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )	[ps30]

// SKIP: ( $RAYTRACESPHERE == 0 ) && ( $SPHERETEXKILLCOMBO == 1 )	[ps30]
// SKIP: ( $RAYTRACESPHERE == 0 ) && ( $SPHERETEXKILLCOMBO == 1 )	[ps20b]

// Debug 2.0 shader locally
//#ifdef SHADER_MODEL_PS_2_B
//#undef SHADER_MODEL_PS_2_B
//#define SHADER_MODEL_PS_2_0
//#endif


// Includes =======================================================================================
#include "common_flashlight_fxc.h"
#include "shader_constant_register_map.h"

// Texture Samplers ===============================================================================
sampler g_tCorneaSampler				: register( s0 );
sampler g_tIrisSampler					: register( s1 );
sampler g_tEyeReflectionCubemapSampler	: register( s2 );
sampler g_tEyeAmbientOcclSampler		: register( s3 );
sampler g_tLightwarpSampler				: register( s4 ); // 1D texture for TF NPR lighting

sampler g_tFlashlightCookieSampler		: register( s5 );
sampler g_tFlashlightDepthSampler		: register( s6 );
sampler g_tRandomRotationSampler		: register( s7 );

// Shaders Constants and Globals ==================================================================
const float4 g_vPackedConst0			: register( c0 );
#define g_flDilationFactor				g_vPackedConst0.x
#define g_flGlossiness					g_vPackedConst0.y
#define g_flAverageAmbient				g_vPackedConst0.z
#define g_flCorneaBumpStrength			g_vPackedConst0.w

const float3 g_vEyeOrigin				: register( c1 );
const float4 g_vIrisProjectionU			: register( c2 );
const float4 g_vIrisProjectionV			: register( c3 );
const float4 g_vCameraPosition			: register( c4 );
const float3 g_cAmbientOcclColor		: register( c5 );

const float4 g_vPackedConst6			: register( c6 );
#define g_flEyeballRadius    g_vPackedConst6.y //0.51f
//#define g_bRaytraceSphere    g_vPackedConst6.z //1.0f
#define g_flParallaxStrength g_vPackedConst6.w //0.25f

// Flashlight constants
const float4 g_vFlashlightAttenuationFactors	: register( c7 ); // FarZ in w
const float3 g_vFlashlightPos					: register( c8 );
const float4 g_vShadowTweaks					: register( c9 );
const float4 g_ShaderControls					: register( c10 );
#define g_fPixelFogType							g_ShaderControls.x

const float4 g_FogParams						: register( PSREG_FOG_PARAMS );

PixelShaderLightInfo g_sLightInfo[3]			: register( PSREG_LIGHT_INFO_ARRAY ); // 2 registers each - 6 registers total

// Interpolated values ============================================================================
struct PS_INPUT
{
	float4 vAmbientOcclUv_fallbackCorneaUv	: TEXCOORD0;
	float4 cVertexLight						: TEXCOORD1; // w is used for the flashlight pass
	float4 vTangentViewVector				: TEXCOORD2; // Tangent view vector (Note: w is used for flashlight pass)
	float4 vWorldPosition_ProjPosZ			: TEXCOORD3;
	float3 vWorldNormal						: TEXCOORD4; // World-space normal
	float3 vWorldTangent					: TEXCOORD5; // World-space tangent
	float4 vLightFalloffCosine01			: TEXCOORD6; // Light falloff and cosine terms for first two local lights
	float4 vLightFalloffCosine23			: TEXCOORD7; // Light falloff and cosine terms for next two local lights

	float3 vWorldBinormal					: COLOR0;	// World-space normal
};

// Ray sphere intersect returns distance along ray to intersection ================================
float IntersectRaySphere ( float3 cameraPos, float3 ray, float3 sphereCenter, float sphereRadius)
{
	float3 dst = cameraPos.xyz - sphereCenter.xyz;
	float B = dot(dst, ray);
	float C = dot(dst, dst) - (sphereRadius * sphereRadius);
	float D = B*B - C;
	return (D > 0) ? (-B - sqrt(D)) : 0;
}

// Calculate both types of Fog and lerp to get result
float CalcPixelFogFactorConst( float fPixelFogType, const float4 fogParams, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ )
{
	float fRangeFog = CalcRangeFog( flProjPosZ, fogParams.x, fogParams.z, fogParams.w );
	float fHeightFog = CalcWaterFogAlpha( fogParams.y, flEyePosZ, flWorldPosZ, flProjPosZ, fogParams.w );
	return lerp( fRangeFog, fHeightFog, fPixelFogType );
}

// Blend both types of Fog and lerp to get result
float3 BlendPixelFogConst( const float3 vShaderColor, float pixelFogFactor, const float3 vFogColor, float fPixelFogType )
{
	pixelFogFactor = saturate( pixelFogFactor );
	float3 fRangeResult = lerp( vShaderColor.rgb, vFogColor.rgb, pixelFogFactor * pixelFogFactor ); //squaring the factor will get the middle range mixing closer to hardware fog
	float3 fHeightResult = lerp( vShaderColor.rgb, vFogColor.rgb, saturate( pixelFogFactor ) );
	return lerp( fRangeResult, fHeightResult, fPixelFogType );
}

float4 FinalOutputConst( const float4 vShaderColor, float pixelFogFactor, float fPixelFogType, const int iTONEMAP_SCALE_TYPE )
{
	float4 result = vShaderColor;
	if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_LINEAR )
	{
		result.rgb *= LINEAR_LIGHT_SCALE;
	}
	else if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_GAMMA )
	{
		result.rgb *= GAMMA_LIGHT_SCALE;
	}

	result.rgb = BlendPixelFogConst( result.rgb, pixelFogFactor, g_LinearFogColor.rgb, fPixelFogType );
	result.rgb = SRGBOutput( result.rgb ); //SRGB in pixel shader conversion

	return result;
}


// Main ===========================================================================================
float4 main( PS_INPUT i ) : COLOR
{
	// Set bools to compile out code
	bool bFlashlight = ( FLASHLIGHT != 0 ) ? true : false;
	bool bDoDiffuseWarp = LIGHTWARPTEXTURE ? true : false;
	int nNumLights = FLASHLIGHT ? 1 : NUM_LIGHTS;	// Flashlight is considered one light, otherwise, use numlights combo

#if !defined( SHADER_MODEL_PS_2_0 )
	bool bRayCast = RAYTRACESPHERE ? true : false;
	bool bRayCastTexKill = SPHERETEXKILLCOMBO ? true : false;
#endif

	float flFlashlightNDotL = i.vTangentViewVector.w;
	float4 vFlashlightTexCoord = { 0.0f, 0.0f, 0.0f, 0.0f };
	if ( bFlashlight )
	{
		vFlashlightTexCoord.xyzw = i.cVertexLight.xyzw; // This was hidden in this interpolator
		i.cVertexLight.rgba = float4( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	// Interpolated vectors
	float3 vWorldNormal = i.vWorldNormal.xyz;
	float3 vWorldTangent = i.vWorldTangent.xyz;
	float3 vWorldBinormal = ( i.vWorldBinormal.xyz * 2.0f ) - 1.0f; // normalize( cross( vWorldNormal.xyz, vWorldTangent.xyz ) );

	float3 vTangentViewVector = i.vTangentViewVector.xyz;

	// World position
	float3 vWorldPosition = i.vWorldPosition_ProjPosZ.xyz;

	// World view vector to pixel
	float3 vWorldViewVector = normalize( vWorldPosition.xyz - g_vCameraPosition.xyz );

	//=================//
	// TF NPR lighting //
	//=================//
	if ( bDoDiffuseWarp )
	{
		// Replace the interpolated vertex light
		if ( bFlashlight == true )
		{
			// Deal with this below in the flashlight section
		}
		else
		{
			if ( nNumLights > 0 )
			{
				float3 cWarpedLight = 2.0f * tex1D( g_tLightwarpSampler, i.vLightFalloffCosine01.z ).rgb;
				i.cVertexLight.rgb += i.vLightFalloffCosine01.x * PixelShaderGetLightColor( g_sLightInfo, 0 ) * cWarpedLight.rgb;
			}

			if ( nNumLights > 1 )
			{
				float3 cWarpedLight = 2.0f * tex1D( g_tLightwarpSampler, i.vLightFalloffCosine01.w ).rgb;
				i.cVertexLight.rgb += i.vLightFalloffCosine01.y * PixelShaderGetLightColor( g_sLightInfo, 1 ) * cWarpedLight.rgb;
			}

			if ( nNumLights > 2 )
			{
				float3 cWarpedLight = 2.0f * tex1D( g_tLightwarpSampler, i.vLightFalloffCosine23.z ).rgb;
				i.cVertexLight.rgb += i.vLightFalloffCosine23.x * PixelShaderGetLightColor( g_sLightInfo, 2 ) * cWarpedLight.rgb;
			}

			if ( nNumLights > 3 )
			{
				float3 cWarpedLight = 2.0f * tex1D( g_tLightwarpSampler, i.vLightFalloffCosine23.w ).rgb;
				i.cVertexLight.rgb += i.vLightFalloffCosine23.y * PixelShaderGetLightColor( g_sLightInfo, 3 ) * cWarpedLight.rgb;
			}
		}
	}

	//==========================================================================================================//
	// Ray cast against sphere representing eyeball to reduce artifacts from non-spherical morphed eye geometry //
	//==========================================================================================================//
#if !defined( SHADER_MODEL_PS_2_0 )
	if ( bRayCast )
	{
		float fSphereRayCastDistance = IntersectRaySphere( g_vCameraPosition.xyz, vWorldViewVector.xyz, g_vEyeOrigin.xyz, g_flEyeballRadius );
		vWorldPosition.xyz = g_vCameraPosition.xyz + ( vWorldViewVector.xyz * fSphereRayCastDistance );
		if (fSphereRayCastDistance == 0)
		{
			if ( bRayCastTexKill )
				clip(-1); // texkill to get a better silhouette
			vWorldPosition.xyz = g_vEyeOrigin.xyz + ( vWorldNormal.xyz * g_flEyeballRadius );
		}
	}
#endif

	//=================================//
	// Generate sphere and cornea uv's //
	//=================================//
#if !defined( SHADER_MODEL_PS_2_0 )
	float2 vCorneaUv; // Note: Cornea texture is a cropped version of the iris texture
	vCorneaUv.x = dot( g_vIrisProjectionU, float4( vWorldPosition, 1.0f ) );
	vCorneaUv.y = dot( g_vIrisProjectionV, float4( vWorldPosition, 1.0f ) );
	float2 vSphereUv = ( vCorneaUv.xy * 0.5f ) + 0.25f;
#else // ps_20
	float2 vCorneaUv = i.vAmbientOcclUv_fallbackCorneaUv.wz; // Note: Cornea texture is a cropped version of the iris texture
	float2 vSphereUv = ( vCorneaUv.xy * 0.5f ) + 0.25f;
#endif

	//=================================//
	// Hacked parallax mapping on iris //
	//=================================//
	float fIrisOffset = tex2D( g_tCorneaSampler, vCorneaUv.xy ).b;

#if !defined( SHADER_MODEL_PS_2_0 )
	float2 vParallaxVector = ( ( vTangentViewVector.xy * fIrisOffset * g_flParallaxStrength ) / ( 1.0f - vTangentViewVector.z ) ); // Note: 0.25 is a magic number
	vParallaxVector.x = -vParallaxVector.x; //Need to flip x...not sure why.
	//vParallaxVector.x *= -1.0; //Need to flip x...not sure why.
	//vParallaxVector = 0.0f; //Disable parallax for debugging
#else // Disable parallax effect in 2.0 version
	float2 vParallaxVector = { 0.0f, 0.0f };
#endif

	float2 vIrisUv = vSphereUv.xy - vParallaxVector.xy;

	// Note: We fetch from this texture twice right now with different uv's for the color and alpha
	float2 vCorneaNoiseUv = vSphereUv.xy + ( vParallaxVector.xy * 0.5 );
	float fCorneaNoise = tex2D( g_tIrisSampler, vCorneaNoiseUv.xy ).a;

	//===============//
	// Cornea normal //
	//===============//
	// Sample 2D normal from texture
	float3 vCorneaTangentNormal = { 0.0, 0.0, 1.0 };
	float4 vCorneaSample = tex2D( g_tCorneaSampler, vCorneaUv.xy );
	vCorneaTangentNormal.xy = vCorneaSample.rg - 0.5f; // Note: This scales the bump to 50% strength

	// Scale strength of normal
	vCorneaTangentNormal.xy *= g_flCorneaBumpStrength;

	// Add in surface noise and imperfections (NOTE: This should be baked into the normal map!)
	vCorneaTangentNormal.xy += fCorneaNoise * 0.1f;

	// Normalize tangent vector
#if !defined( SHADER_MODEL_PS_2_0 )
	// Since this isn't used later in 2.0, skip the normalize to save shader instructions
	vCorneaTangentNormal.xyz = normalize( vCorneaTangentNormal.xyz );
#endif

	// Transform into world space
	float3 vCorneaWorldNormal = Vec3TangentToWorldNormalized( vCorneaTangentNormal.xyz, vWorldNormal.xyz, vWorldTangent.xyz, vWorldBinormal.xyz );

	//============//
	// Flashlight //
	//============//
	float3 vFlashlightVector = { 0.0f, 0.0f, 0.0f };
	float3 cFlashlightColorFalloff = { 0.0f, 0.0f, 0.0f };
	if ( bFlashlight == true )
	{
		// Flashlight vector
		vFlashlightVector.xyz = normalize( g_vFlashlightPos.xyz - i.vWorldPosition_ProjPosZ.xyz );

		// Distance attenuation for flashlight and to fade out shadow over distance
		float3 vDelta = g_vFlashlightPos.xyz - i.vWorldPosition_ProjPosZ.xyz;
		float flDistSquared = dot( vDelta, vDelta );
		float flDist = sqrt( flDistSquared );
		float flFlashlightAttenuation = dot( g_vFlashlightAttenuationFactors.xyz, float3( 1.0f, 1.0f/flDist, 1.0f/flDistSquared ) );

		// Flashlight cookie
#if !defined( SHADER_MODEL_PS_2_0 )
		float3 vProjCoords = vFlashlightTexCoord.xyz / vFlashlightTexCoord.w;
		float3 cFlashlightCookieColor = tex2D( g_tFlashlightCookieSampler, vProjCoords );
#else 
		float3 cFlashlightCookieColor = tex2Dproj( g_tFlashlightCookieSampler, vFlashlightTexCoord.xyzw );
#endif

		// Shadow depth map
#if FLASHLIGHTSHADOWS && !defined( SHADER_MODEL_PS_2_0 )
		int nShadowLevel = FLASHLIGHTDEPTHFILTERMODE;
		float flShadow = DoFlashlightShadow( g_tFlashlightDepthSampler, g_tRandomRotationSampler, vProjCoords, float2(0,0), nShadowLevel, g_vShadowTweaks, false );
		float flAttenuated = lerp( flShadow, 1.0f, g_vShadowTweaks.y );		// Blend between fully attenuated and not attenuated
		flShadow = lerp( flAttenuated, flShadow, flFlashlightAttenuation ); // Blend between shadow and above, according to light attenuation
		cFlashlightCookieColor *= flShadow; // Apply shadow term to cookie color
#endif

		// Flashlight color intensity (needs to be multiplied by global flashlight color later)
		cFlashlightColorFalloff.rgb = flFlashlightAttenuation * cFlashlightCookieColor.rgb;

		// Add this into the interpolated lighting
		if ( bDoDiffuseWarp )
		{
			//float3 cWarpedLight = 2.0f * tex1D( g_tLightwarpSampler, flFlashlightNDotL ).rgb;
			//i.cVertexLight.rgb += cFlashlightColorFalloff.rgb * cFlashlightColor.rgb * cWarpedLight.rgb;
			i.cVertexLight.rgb += cFlashlightColorFalloff.rgb * cFlashlightColor.rgb * flFlashlightNDotL; // No light warp for now
		}
		else
		{
			i.cVertexLight.rgb += cFlashlightColorFalloff.rgb * cFlashlightColor.rgb * flFlashlightNDotL;
		}
	}

	//==============//
	// Dilate pupil //
	//==============//
#if !defined( SHADER_MODEL_PS_2_0 )
	vIrisUv.xy -= 0.5f; // Center around (0,0)
	float fPupilCenterToBorder = saturate( length( vIrisUv.xy ) / 0.2f ); //Note: 0.2 is the uv radius of the iris
	float fPupilDilateFactor = g_flDilationFactor; // This value should be between 0-1
	vIrisUv.xy *= lerp (1.0f, fPupilCenterToBorder, saturate( fPupilDilateFactor ) * 2.5f - 1.25f );
	vIrisUv.xy += 0.5f;
#endif

	//============//
	// Iris color //
	//============//
	float4 cIrisColor = tex2D( g_tIrisSampler, vIrisUv.xy );

	//==========================//
	// Iris lighting highlights //
	//==========================//
	float3 cIrisLighting = float3( 0.0f, 0.0f, 0.0f );

#if !defined( SHADER_MODEL_PS_2_0 )
	// Mask off everything but the iris pixels
	float fIrisHighlightMask = tex2D( g_tCorneaSampler, vCorneaUv.xy ).a;

	// Generate the normal
	float3 vIrisTangentNormal = vCorneaTangentNormal.xyz;
	vIrisTangentNormal.xy *= -2.5f; // I'm not normalizing on purpose

	for ( int j=0; j < nNumLights; j++ )
	{
		// World light vector
		float3 vWorldLightVector;
		if ( ( j == 0 ) && ( bFlashlight == true ) )
			vWorldLightVector = vFlashlightVector.xyz;
		else
			vWorldLightVector = PixelShaderGetLightVector( i.vWorldPosition_ProjPosZ.xyz, g_sLightInfo, j );

		// Tangent light vector
		float3 vTangentLightVector = Vec3WorldToTangent( vWorldLightVector.xyz, vWorldNormal.xyz, vWorldTangent.xyz, vWorldBinormal.xyz );

		// Adjust the tangent light vector to generate the iris lighting
		float3 tmpv = -vTangentLightVector.xyz;
		tmpv.xy *= -0.5f; //Flatten tangent view
		tmpv.z = max( tmpv.z, 0.5f ); //Clamp z of tangent view to help maintain highlight
		tmpv.xyz = normalize( tmpv.xyz );

		// Core iris lighting math
		float fIrisFacing = pow( abs( dot( vIrisTangentNormal.xyz, tmpv.xyz ) ), 6.0f ) * 0.5f; // Yes, 6.0 and 0.5 are magic numbers

		// Cone of darkness to darken iris highlights when light falls behind eyeball past a certain point
		float flConeOfDarkness = pow( 1.0f - saturate( ( -vTangentLightVector.z - 0.25f ) / 0.75f ), 4.0f );
		//float flConeOfDarkness = pow( 1.0f - saturate( ( -dot( vIrisTangentNormal.xyz, vTangentLightVector.xyz ) - 0.15f ) / 0.85f ), 8.0f );

		// Tint by iris color and cone of darkness
		float3 cIrisLightingTmp = fIrisFacing * fIrisHighlightMask * flConeOfDarkness;

		// Attenuate by light color and light falloff
		if ( ( j == 0 ) && ( bFlashlight == true ) )
			cIrisLightingTmp.rgb *= cFlashlightColorFalloff.rgb * cFlashlightColor.rgb;
		else if ( j == 0 )
			cIrisLightingTmp.rgb *= i.vLightFalloffCosine01.x * PixelShaderGetLightColor( g_sLightInfo, 0 );
		else if ( j == 1 )
			cIrisLightingTmp.rgb *= i.vLightFalloffCosine01.y * PixelShaderGetLightColor( g_sLightInfo, 1 );
		else if ( j == 2 )
			cIrisLightingTmp.rgb *= i.vLightFalloffCosine23.x * PixelShaderGetLightColor( g_sLightInfo, 2 );
		else
			cIrisLightingTmp.rgb *= i.vLightFalloffCosine23.y * PixelShaderGetLightColor( g_sLightInfo, 3 );

		// Sum into final variable
		cIrisLighting.rgb += cIrisLightingTmp.rgb;
	}

	// Add slight view dependent iris lighting based on ambient light intensity to enhance situations with no local lights (0.5f is to help keep it subtle)
	cIrisLighting.rgb += saturate( dot( vIrisTangentNormal.xyz, -vTangentViewVector.xyz ) ) * g_flAverageAmbient * fIrisHighlightMask * 0.5f;
#else
	// Else, intensify light over cornea to simulate the brightening that happens above
	cIrisLighting.rgb += i.cVertexLight.rgb * vCorneaSample.a;
#endif

	//===================//
	// Ambient occlusion //
	//===================//
	float3 cAmbientOcclFromTexture = tex2D( g_tEyeAmbientOcclSampler, i.vAmbientOcclUv_fallbackCorneaUv.xy ).rgb;
	float3 cAmbientOcclColor = lerp( g_cAmbientOcclColor, 1.0f, cAmbientOcclFromTexture.rgb ); // Color the ambient occlusion
	i.cVertexLight.rgb *= cAmbientOcclColor.rgb;

	//==========================//
	// Reflection from cube map //
	//==========================//
	float3 vCorneaReflectionVector = reflect ( vWorldViewVector.xyz, vCorneaWorldNormal.xyz );

	//float3 cReflection = ENV_MAP_SCALE * texCUBE( g_tEyeReflectionCubemapSampler, vCorneaReflectionVector.xyz ).rgb;
	float3 cReflection = g_flGlossiness * texCUBE( g_tEyeReflectionCubemapSampler, vCorneaReflectionVector.xyz ).rgb;

	// Hack: Only add in half of the env map for the flashlight pass. This looks reasonable.
	if ( bFlashlight )
	{
		cReflection.rgb *= 0.5f;
	}

	//===========================//
	// Glint specular highlights //
	//===========================//
	float3 cSpecularHighlights = 0.0f;
	if ( bFlashlight )
	{
		cSpecularHighlights.rgb += pow( saturate( dot( vCorneaReflectionVector.xyz, vFlashlightVector.xyz ) ), 128.0f ) * cFlashlightColorFalloff.rgb * cFlashlightColor.rgb;
	}
	else // no flashlight
	{
		if ( nNumLights > 0 )
			cSpecularHighlights.rgb += pow( saturate( dot( vCorneaReflectionVector.xyz, PixelShaderGetLightVector( i.vWorldPosition_ProjPosZ.xyz, g_sLightInfo, 0 ) ) ), 128.0f ) * i.vLightFalloffCosine01.x * PixelShaderGetLightColor( g_sLightInfo, 0 );

		if ( nNumLights > 1 )
			cSpecularHighlights.rgb += pow( saturate( dot( vCorneaReflectionVector.xyz, PixelShaderGetLightVector( i.vWorldPosition_ProjPosZ.xyz, g_sLightInfo, 1 ) ) ), 128.0f ) * i.vLightFalloffCosine01.y * PixelShaderGetLightColor( g_sLightInfo, 1 );

		if ( nNumLights > 2 )
			cSpecularHighlights.rgb += pow( saturate( dot( vCorneaReflectionVector.xyz, PixelShaderGetLightVector( i.vWorldPosition_ProjPosZ.xyz, g_sLightInfo, 2 ) ) ), 128.0f ) * i.vLightFalloffCosine23.x * PixelShaderGetLightColor( g_sLightInfo, 2 );

		if ( nNumLights > 3 )
			cSpecularHighlights.rgb += pow( saturate( dot( vCorneaReflectionVector.xyz, PixelShaderGetLightVector( i.vWorldPosition_ProjPosZ.xyz, g_sLightInfo, 3 ) ) ), 128.0f ) * i.vLightFalloffCosine23.y * PixelShaderGetLightColor( g_sLightInfo, 3 );
	}

	//===============//
	// Combine terms //
	//===============//
	float4 result;

	// Unlit iris, pupil, and sclera color
	result.rgb = cIrisColor.rgb;

	// Add in slight cornea noise to help define raised cornea layer for close-ups
	result.rgb += fCorneaNoise * 0.1f;

	// Diffuse light (Vertex lighting + extra iris caustic lighting)
	result.rgb *= i.cVertexLight.rgb + cIrisLighting.rgb;

	// Environment map
	result.rgb += cReflection.rgb * i.cVertexLight.rgb;

	// Local light glints
	result.rgb += cSpecularHighlights.rgb;

	// Set alpha to 1.0 by default
	result.a = 1.0;

#if !defined( SHADER_MODEL_PS_2_0 )
	float fogFactor = CalcPixelFogFactorConst( g_fPixelFogType, g_FogParams, g_vCameraPosition.z, i.vWorldPosition_ProjPosZ.z, i.vWorldPosition_ProjPosZ.w );
	return FinalOutputConst( result, fogFactor, g_fPixelFogType, TONEMAP_SCALE_LINEAR );
#else
	float fogFactor = CalcPixelFogFactor( PIXEL_FOG_TYPE_NONE, g_FogParams, g_vCameraPosition.z, i.vWorldPosition_ProjPosZ.z, i.vWorldPosition_ProjPosZ.w );
	return FinalOutput( result, fogFactor, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_LINEAR );
#endif

}
