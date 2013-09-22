//  SKIP: $BUMPMAP2 && $WARPLIGHTING
//  SKIP: $WARPLIGHTING && $DETAILTEXTURE
//	SKIP: $ENVMAPMASK && $BUMPMAP
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $BASEALPHAENVMAPMASK
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $SELFILLUM
//  SKIP: !$FASTPATH && $FASTPATHENVMAPCONTRAST
//  SKIP: !$FASTPATH && $FASTPATHENVMAPTINT
//  SKIP: !$BUMPMAP && $DIFFUSEBUMPMAP
//	SKIP: !$BUMPMAP && $BUMPMAP2
//	SKIP: $ENVMAPMASK && $BUMPMAP2
//	SKIP: $BASETEXTURENOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASETEXTURE2NOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASEALPHAENVMAPMASK && $BUMPMAP
//  SKIP: $PARALLAXMAP && $DETAILTEXTURE
//  SKIP: $SEAMLESS && $RELIEF_MAPPING
//  SKIP: $SEAMLESS && $DETAILTEXTURE
//  SKIP: $SEAMLESS && $MASKEDBLENDING
//  SKIP: $BUMPMASK && ( $SEAMLESS || $DETAILTEXTURE || $SELFILLUM || $BASETEXTURENOENVMAP || $BASETEXTURE2 )
//	SKIP: !$BUMPMAP && ($NORMAL_DECODE_MODE == 1)
//	SKIP: !$BUMPMAP && ($NORMAL_DECODE_MODE == 2)
//	SKIP: !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 1)
//	SKIP: !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 2)
//  NOSKIP: $FANCY_BLENDING && (!$FASTPATH)

// 360 compiler craps out on some combo in this family.  Content doesn't use blendmode 10 anyway
//  SKIP: $FASTPATH && $PIXELFOGTYPE && $BASETEXTURE2 && $DETAILTEXTURE && $CUBEMAP && ($DETAIL_BLEND_MODE == 10 ) [XBOX]

// debug crap:
// NOSKIP: $DETAILTEXTURE
// NOSKIP: $CUBEMAP
// NOSKIP: $ENVMAPMASK
// NOSKIP: $BASEALPHAENVMAPMASK
// NOSKIP: $SELFILLUM

#define USE_32BIT_LIGHTMAPS_ON_360 //uncomment to use 32bit lightmaps, be sure to keep this in sync with the same #define in materialsystem/cmatlightmaps.cpp

#include "common_ps_fxc.h"
#include "common_flashlight_fxc.h"
#include "common_lightmappedgeneric_fxc.h"

#if SEAMLESS
#define USE_FAST_PATH 1
#else
#define USE_FAST_PATH FASTPATH
#endif

const HALF4 g_EnvmapTint : register( c0 );

#if USE_FAST_PATH == 1

#	if FASTPATHENVMAPCONTRAST == 0
static const HALF3 g_EnvmapContrast = { 0.0f, 0.0f, 0.0f };
#	else
static const HALF3 g_EnvmapContrast = { 1.0f, 1.0f, 1.0f };
#	endif
static const HALF3 g_EnvmapSaturation = { 1.0f, 1.0f, 1.0f };
static const HALF g_FresnelReflection = 1.0f;
static const HALF g_OneMinusFresnelReflection = 0.0f;
static const HALF4 g_SelfIllumTint = { 1.0f, 1.0f, 1.0f, 1.0f };
#   if OUTLINE
const float4 g_OutlineParams : register( c2 );
#define OUTLINE_MIN_VALUE0 g_OutlineParams.x
#define OUTLINE_MIN_VALUE1 g_OutlineParams.y
#define OUTLINE_MAX_VALUE0 g_OutlineParams.z
#define OUTLINE_MAX_VALUE1 g_OutlineParams.w

const float4 g_OutlineColor : register( c3 );
#define OUTLINE_COLOR g_OutlineColor

#   endif
#   if SOFTEDGES
const float4 g_EdgeSoftnessParms : register( c4 );
#define SOFT_MASK_MIN g_EdgeSoftnessParms.x
#define SOFT_MASK_MAX g_EdgeSoftnessParms.y
#   endif
#else

const HALF3 g_EnvmapContrast				: register( c2 );
const HALF3 g_EnvmapSaturation				: register( c3 );
const HALF4 g_FresnelReflectionReg			: register( c4 );
#define g_FresnelReflection g_FresnelReflectionReg.a
#define g_OneMinusFresnelReflection g_FresnelReflectionReg.b
const HALF4 g_SelfIllumTint					: register( c7 );
#endif

const float4 g_DetailTint_and_BlendFactor	: register( c8 );
#define g_DetailTint (g_DetailTint_and_BlendFactor.rgb)
#define g_DetailBlendFactor (g_DetailTint_and_BlendFactor.w)

const HALF3 g_EyePos						: register( c10 );
const HALF4 g_FogParams						: register( c11 );
const float4 g_TintValuesAndLightmapScale	: register( c12 );

#define g_flAlpha2 g_TintValuesAndLightmapScale.w

const float4 g_FlashlightAttenuationFactors	: register( c13 );
const float3 g_FlashlightPos				: register( c14 );
const float4x4 g_FlashlightWorldToTexture	: register( c15 ); // through c18
const float4 g_ShadowTweaks					: register( c19 );


sampler BaseTextureSampler		: register( s0 );
sampler LightmapSampler			: register( s1 );
sampler EnvmapSampler			: register( s2 );
#if FANCY_BLENDING
sampler BlendModulationSampler	: register( s3 );
#endif

#if DETAILTEXTURE
sampler DetailSampler			: register( s12 );
#endif

sampler BumpmapSampler			: register( s4 );
#if NORMAL_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
sampler AlphaMapSampler		: register( s9 );	// alpha
#else
#define AlphaMapSampler		BumpmapSampler
#endif

#if BUMPMAP2 == 1
sampler BumpmapSampler2			: register( s5 );
#if NORMAL_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
sampler AlphaMapSampler2		: register( s10 );	// alpha
#else
#define AlphaMapSampler2		BumpmapSampler2
#endif
#else
sampler EnvmapMaskSampler		: register( s5 );
#endif


#if WARPLIGHTING
sampler WarpLightingSampler		: register( s6 );
#endif
sampler BaseTextureSampler2		: register( s7 );

#if BUMPMASK == 1
sampler BumpMaskSampler			: register( s8 );
#if NORMALMASK_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
sampler AlphaMaskSampler		: register( s11 );	// alpha
#else
#define AlphaMaskSampler		BumpMaskSampler
#endif
#endif

#if defined( _X360 ) && FLASHLIGHT
sampler FlashlightSampler		: register( s13 );
sampler ShadowDepthSampler		: register( s14 );
sampler RandRotSampler			: register( s15 );
#endif

struct PS_INPUT
{
#if SEAMLESS
	float3 SeamlessTexCoord         : TEXCOORD0;            // zy xz
	float4 detailOrBumpAndEnvmapMaskTexCoord : TEXCOORD1;   // envmap mask
#else
	HALF2 baseTexCoord				: TEXCOORD0;
	// detail textures and bumpmaps are mutually exclusive so that we have enough texcoords.
#if ( RELIEF_MAPPING == 0 )
	HALF4 detailOrBumpAndEnvmapMaskTexCoord	: TEXCOORD1;
#endif
#endif
// CENTROID: TEXCOORD2
	HALF4 lightmapTexCoord1And2		: TEXCOORD2;
// CENTROID: TEXCOORD3
	HALF4 lightmapTexCoord3			: TEXCOORD3;
	HALF4 worldPos_projPosZ			: TEXCOORD4;
	HALF3x3 tangentSpaceTranspose	: TEXCOORD5;
	// tangentSpaceTranspose		: TEXCOORD6
	// tangentSpaceTranspose		: TEXCOORD7
	HALF4 vertexColor				: COLOR;
	float4 vertexBlendX_fogFactorW	: COLOR1;

	// Extra iterators on 360, used in flashlight combo
#if defined( _X360 ) && FLASHLIGHT
	float4 flashlightSpacePos		: TEXCOORD8;
	float4 vProjPos					: TEXCOORD9;
#endif
};

#if LIGHTING_PREVIEW == 2
LPREVIEW_PS_OUT main( PS_INPUT i ) : COLOR
#else
HALF4 main( PS_INPUT i ) : COLOR
#endif
{
	bool bBaseTexture2 = BASETEXTURE2 ? true : false;
	bool bDetailTexture = DETAILTEXTURE ? true : false;
	bool bBumpmap = BUMPMAP ? true : false;
	bool bDiffuseBumpmap = DIFFUSEBUMPMAP ? true : false;
	bool bCubemap = CUBEMAP ? true : false;
	bool bEnvmapMask = ENVMAPMASK ? true : false;
	bool bBaseAlphaEnvmapMask = BASEALPHAENVMAPMASK ? true : false;
	bool bSelfIllum = SELFILLUM ? true : false;
	bool bNormalMapAlphaEnvmapMask = NORMALMAPALPHAENVMAPMASK ? true : false;
	bool bBaseTextureNoEnvmap = BASETEXTURENOENVMAP ? true : false;
	bool bBaseTexture2NoEnvmap = BASETEXTURE2NOENVMAP ? true : false;

	float4 baseColor = 0.0f;
	float4 baseColor2 = 0.0f;
	float4 vNormal = float4(0, 0, 1, 1);
	float3 baseTexCoords = float3(0,0,0);

#if SEAMLESS
	baseTexCoords = i.SeamlessTexCoord.xyz;
#else
	baseTexCoords.xy = i.baseTexCoord.xy;
#endif

	GetBaseTextureAndNormal( BaseTextureSampler, BaseTextureSampler2, BumpmapSampler, bBaseTexture2, bBumpmap || bNormalMapAlphaEnvmapMask, 
		baseTexCoords, i.vertexColor.rgb, baseColor, baseColor2, vNormal );

#if BUMPMAP == 1	// not ssbump
	vNormal.xyz = vNormal.xyz * 2.0f - 1.0f;					// make signed if we're not ssbump
#endif

	HALF3 lightmapColor1 = HALF3( 1.0f, 1.0f, 1.0f );
	HALF3 lightmapColor2 = HALF3( 1.0f, 1.0f, 1.0f );
	HALF3 lightmapColor3 = HALF3( 1.0f, 1.0f, 1.0f );
#if LIGHTING_PREVIEW == 0
	if( bBumpmap && bDiffuseBumpmap )
	{
		HALF2 bumpCoord1;
		HALF2 bumpCoord2;
		HALF2 bumpCoord3;
		ComputeBumpedLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy,
			bumpCoord1, bumpCoord2, bumpCoord3 );
		
		lightmapColor1 = LightMapSample( LightmapSampler, bumpCoord1 );
		lightmapColor2 = LightMapSample( LightmapSampler, bumpCoord2 );
		lightmapColor3 = LightMapSample( LightmapSampler, bumpCoord3 );
	}
	else
	{
		HALF2 bumpCoord1 = ComputeLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy );
		lightmapColor1 = LightMapSample( LightmapSampler, bumpCoord1 );
	}
#endif

#if RELIEF_MAPPING
	// in the parallax case, all texcoords must be the same in order to free
    // up an iterator for the tangent space view vector
	HALF2 detailTexCoord = i.baseTexCoord.xy;
	HALF2 bumpmapTexCoord = i.baseTexCoord.xy;
	HALF2 envmapMaskTexCoord = i.baseTexCoord.xy;
#else

	#if ( DETAILTEXTURE == 1 )
		HALF2 detailTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
		HALF2 bumpmapTexCoord = i.baseTexCoord.xy;
	#elif ( BUMPMASK == 1 )
		HALF2 detailTexCoord = 0.0f;
		HALF2 bumpmapTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
		HALF2 bumpmap2TexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.wz;
	#else
		HALF2 detailTexCoord = 0.0f;
		HALF2 bumpmapTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
	#endif

	HALF2 envmapMaskTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.wz;
#endif // !RELIEF_MAPPING

	HALF4 detailColor = HALF4( 1.0f, 1.0f, 1.0f, 1.0f );
#if DETAILTEXTURE

#if SHADER_MODEL_PS_2_0
	detailColor = tex2D( DetailSampler, detailTexCoord );
#else
	detailColor = float4( g_DetailTint, 1.0f ) * tex2D( DetailSampler, detailTexCoord );
#endif

#endif

#if ( OUTLINE || SOFTEDGES )
	float distAlphaMask = baseColor.a;

#   if OUTLINE
	if ( ( distAlphaMask >= OUTLINE_MIN_VALUE0 ) &&
		 ( distAlphaMask <= OUTLINE_MAX_VALUE1 ) )
	{
		float oFactor=1.0;
		if ( distAlphaMask <= OUTLINE_MIN_VALUE1 )
		{
			oFactor=smoothstep( OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, distAlphaMask );
		}
		else
		{
			oFactor=smoothstep( OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, distAlphaMask );
		}
		baseColor = lerp( baseColor, OUTLINE_COLOR, oFactor );
	}
#   endif
#   if SOFTEDGES
	baseColor.a *= smoothstep( SOFT_MASK_MAX, SOFT_MASK_MIN, distAlphaMask );
#   else
	baseColor.a *= distAlphaMask >= 0.5;
#   endif
#endif


#if LIGHTING_PREVIEW == 2
	baseColor.xyz=GammaToLinear(baseColor.xyz);
#endif

	float blendedAlpha = baseColor.a;

#if MASKEDBLENDING
	float blendfactor=0.5;
#else
	float blendfactor=i.vertexBlendX_fogFactorW.r;
#endif

	if( bBaseTexture2 )
	{
#if (SELFILLUM == 0) && (NORMALMAPALPHAENVMAPMASK==0) && (PIXELFOGTYPE != PIXEL_FOG_TYPE_HEIGHT) && (FANCY_BLENDING)
		float4 modt=tex2D(BlendModulationSampler,i.lightmapTexCoord3.zw);
#if MASKEDBLENDING
		float minb=modt.g-modt.r;
		float maxb=modt.g+modt.r;
#else
		float minb=max(0,modt.g-modt.r);
		float maxb=min(1,modt.g+modt.r);
#endif
		blendfactor=smoothstep(minb,maxb,blendfactor);
#endif
		baseColor.rgb = lerp( baseColor, baseColor2.rgb, blendfactor );
		blendedAlpha = lerp( baseColor.a, baseColor2.a, blendfactor );
	}

	HALF3 specularFactor = 1.0f;
	float4 vNormalMask = float4(0, 0, 1, 1);
	if( bBumpmap )
	{
		if( bBaseTextureNoEnvmap )
		{
			vNormal.a = 0.0f;
		}

#if ( BUMPMAP2 == 1 )
		{
	#if ( BUMPMASK == 1 )
			HALF2 b2TexCoord = bumpmap2TexCoord;
	#else
			HALF2 b2TexCoord = bumpmapTexCoord;
	#endif

			HALF4 vNormal2;
			if ( BUMPMAP == 2 )
				vNormal2 = tex2D( BumpmapSampler2, b2TexCoord );
			else
				vNormal2 = DecompressNormal( BumpmapSampler2, b2TexCoord, NORMAL_DECODE_MODE, AlphaMapSampler2 );		// Bump 2 coords

			if( bBaseTexture2NoEnvmap )
			{
				vNormal2.a = 0.0f;
			}

	#if ( BUMPMASK == 1 )
			float3 vNormal1 = DecompressNormal( BumpmapSampler, i.detailOrBumpAndEnvmapMaskTexCoord.xy, NORMALMASK_DECODE_MODE, AlphaMapSampler );

			vNormal.xyz = normalize( vNormal1.xyz + vNormal2.xyz );

			// Third normal map...same coords as base
			vNormalMask = DecompressNormal( BumpMaskSampler, i.baseTexCoord.xy, NORMALMASK_DECODE_MODE, AlphaMaskSampler );

			vNormal.xyz = lerp( vNormalMask.xyz, vNormal.xyz, vNormalMask.a );		// Mask out normals from vNormal
			specularFactor = vNormalMask.a;
	#else // BUMPMASK == 0
			vNormal.xyz = lerp( vNormal.xyz, vNormal2.xyz, blendfactor);
	#endif

		}

#endif // BUMPMAP2 == 1

		if( bNormalMapAlphaEnvmapMask )
		{
			specularFactor *= vNormal.a;
		}
	}
	else if ( bNormalMapAlphaEnvmapMask )
	{
		specularFactor *= vNormal.a;
	}

#if ( BUMPMAP2 == 0 )
	if( bEnvmapMask )
	{
		specularFactor *= tex2D( EnvmapMaskSampler, envmapMaskTexCoord ).xyz;	
	}
#endif

	if( bBaseAlphaEnvmapMask )
	{
		specularFactor *= 1.0 - blendedAlpha; // Reversing alpha blows!
	}

	float4 albedo = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	float alpha = 1.0f;
	albedo *= baseColor;
	if( !bBaseAlphaEnvmapMask && !bSelfIllum )
	{
		alpha *= baseColor.a;
	}

	if( bDetailTexture )
	{
		albedo = TextureCombine( albedo, detailColor, DETAIL_BLEND_MODE, g_DetailBlendFactor );
	}

	// The vertex color contains the modulation color + vertex color combined
#if ( SEAMLESS == 0 )
	albedo.xyz *= i.vertexColor;
#endif
	alpha *= i.vertexColor.a * g_flAlpha2; // not sure about this one

	// Save this off for single-pass flashlight, since we'll still need the SSBump vector, not a real normal
	float3 vSSBumpVector = vNormal.xyz;

	HALF3 diffuseLighting;
	if( bBumpmap && bDiffuseBumpmap )
	{

// ssbump
#if ( BUMPMAP == 2 )
		diffuseLighting = vNormal.x * lightmapColor1 +
						  vNormal.y * lightmapColor2 +
						  vNormal.z * lightmapColor3;
		diffuseLighting *= g_TintValuesAndLightmapScale.rgb;

		// now, calculate vNormal for reflection purposes. if vNormal isn't needed, hopefully
		// the compiler will eliminate these calculations
		vNormal.xyz = normalize( bumpBasis[0]*vNormal.x + bumpBasis[1]*vNormal.y + bumpBasis[2]*vNormal.z);
#else
		float3 dp;
		dp.x = saturate( dot( vNormal, bumpBasis[0] ) );
		dp.y = saturate( dot( vNormal, bumpBasis[1] ) );
		dp.z = saturate( dot( vNormal, bumpBasis[2] ) );
		dp *= dp;
		
#if ( DETAIL_BLEND_MODE == TCOMBINE_SSBUMP_BUMP )
		dp *= 2*detailColor;
#endif
		diffuseLighting = dp.x * lightmapColor1 +
						  dp.y * lightmapColor2 +
						  dp.z * lightmapColor3;
		float sum = dot( dp, float3( 1.0f, 1.0f, 1.0f ) );
		diffuseLighting *= g_TintValuesAndLightmapScale.rgb / sum;
#endif
	}
	else
	{
		diffuseLighting = lightmapColor1 * g_TintValuesAndLightmapScale.rgb;
	}

#if WARPLIGHTING && ( SEAMLESS == 0 )
	float len=0.5*length(diffuseLighting);
	// FIXME: 8-bit lookup textures like this need a "nice filtering" VTF option, which converts
	//        them to 16-bit on load or does filtering in the shader (since most hardware - 360
	//        included - interpolates 8-bit textures at 8-bit precision, which causes banding)
	diffuseLighting *= 2.0*tex2D(WarpLightingSampler,float2(len,0));
#endif

#if CUBEMAP || LIGHTING_PREVIEW || ( defined( _X360 ) && FLASHLIGHT )
	float3 worldSpaceNormal = mul( vNormal, i.tangentSpaceTranspose );
#endif

	float3 diffuseComponent = albedo.xyz * diffuseLighting;

#if defined( _X360 ) && FLASHLIGHT

	// ssbump doesn't pass a normal to the flashlight...it computes shadowing a different way
#if ( BUMPMAP == 2 )
	bool bHasNormal = false;

	float3 worldPosToLightVector = g_FlashlightPos - i.worldPos_projPosZ.xyz;

	float3 tangentPosToLightVector;
	tangentPosToLightVector.x = dot( worldPosToLightVector, i.tangentSpaceTranspose[0] );
	tangentPosToLightVector.y = dot( worldPosToLightVector, i.tangentSpaceTranspose[1] );
	tangentPosToLightVector.z = dot( worldPosToLightVector, i.tangentSpaceTranspose[2] );

	tangentPosToLightVector = normalize( tangentPosToLightVector );

	float nDotL = saturate( vSSBumpVector.x*dot( tangentPosToLightVector, bumpBasis[0]) +
							vSSBumpVector.y*dot( tangentPosToLightVector, bumpBasis[1]) +
							vSSBumpVector.z*dot( tangentPosToLightVector, bumpBasis[2]) );
#else
	bool bHasNormal = true;
	float nDotL = 1.0f;
#endif

	float fFlashlight = DoFlashlight( g_FlashlightPos, i.worldPos_projPosZ.xyz, i.flashlightSpacePos,
		worldSpaceNormal, g_FlashlightAttenuationFactors.xyz, 
		g_FlashlightAttenuationFactors.w, FlashlightSampler, ShadowDepthSampler,
		RandRotSampler, 0, true, false, i.vProjPos.xy / i.vProjPos.w, false, g_ShadowTweaks, bHasNormal );

	diffuseComponent = albedo.xyz * ( diffuseLighting + ( fFlashlight * nDotL ) );
#endif

	if( bSelfIllum )
	{
		float3 selfIllumComponent = g_SelfIllumTint * albedo.xyz;
		diffuseComponent = lerp( diffuseComponent, selfIllumComponent, baseColor.a );
	}

	HALF3 specularLighting = HALF3( 0.0f, 0.0f, 0.0f );
#if CUBEMAP
	if( bCubemap )
	{
		float3 worldVertToEyeVector = g_EyePos - i.worldPos_projPosZ.xyz;
		float3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, worldVertToEyeVector );

		// Calc Fresnel factor
		half3 eyeVect = normalize(worldVertToEyeVector);
		HALF fresnel = 1.0 - dot( worldSpaceNormal, eyeVect );
		fresnel = pow( fresnel, 5.0 );
		fresnel = fresnel * g_OneMinusFresnelReflection + g_FresnelReflection;
		
		specularLighting = ENV_MAP_SCALE * texCUBE( EnvmapSampler, reflectVect );
		specularLighting *= specularFactor;
								   
		specularLighting *= g_EnvmapTint;
#if FANCY_BLENDING == 0
		HALF3 specularLightingSquared = specularLighting * specularLighting;
		specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
		HALF3 greyScale = dot( specularLighting, HALF3( 0.299f, 0.587f, 0.114f ) );
		specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
#endif
		specularLighting *= fresnel;
	}
#endif

	HALF3 result = diffuseComponent + specularLighting;
	
#if LIGHTING_PREVIEW
	worldSpaceNormal = mul( vNormal, i.tangentSpaceTranspose );
#	if LIGHTING_PREVIEW == 1
	float dotprod = 0.7+0.25 * dot( worldSpaceNormal, normalize( float3( 1, 2, -.5 ) ) );
	return FinalOutput( HALF4( dotprod*albedo.xyz, alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#	else
	LPREVIEW_PS_OUT ret;
	ret.color = float4( albedo.xyz,alpha );
	ret.normal = float4( worldSpaceNormal,alpha );
	ret.position = float4( i.worldPos_projPosZ.xyz, alpha );
	ret.flags = float4( 1, 1, 1, alpha );

	return FinalOutput( ret, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );	
#	endif
#else // == end LIGHTING_PREVIEW ==

	bool bWriteDepthToAlpha = false;

	// ps_2_b and beyond
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0))
	bWriteDepthToAlpha = ( WRITE_DEPTH_TO_DESTALPHA != 0 ) && ( WRITEWATERFOGTODESTALPHA == 0 );
#endif

	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );

#if WRITEWATERFOGTODESTALPHA && (PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	alpha = fogFactor;
#endif

	return FinalOutput( float4( result.rgb, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, i.worldPos_projPosZ.w );

#endif
}
 
