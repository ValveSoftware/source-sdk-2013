// STATIC: "CONVERT_TO_SRGB"			"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"			"0..0"	[= 0] [XBOX]
// STATIC: "BASETEXTURE"				"0..1"
// STATIC: "REFLECT"					"0..1"
// STATIC: "REFRACT"					"0..1"
// STATIC: "ENVMAPMASK"					"0..1"

// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..1"	[ps20b] [PC]
// DYNAMIC: "WRITE_DEPTH_TO_DESTALPHA"	"0..0"	[ps20b] [XBOX]

#if defined( SHADER_MODEL_PS_2_0 )
#	define WRITE_DEPTH_TO_DESTALPHA 0
#endif

#include "common_ps_fxc.h"

sampler RefractSampler			: register( s0 );
sampler BaseTextureSampler		: register( s1 );
sampler ReflectSampler			: register( s2 );
#if BASETEXTURE
sampler LightmapSampler			: register( s3 );
#endif
#if ENVMAPMASK
sampler EnvMapMaskSampler		: register( s6 );
#endif
sampler NormalSampler			: register( s4 );

const HALF4 vRefractTint			: register( c1 );
const float4 g_FresnelConstants		: register( c3 );
const HALF4 vReflectTint			: register( c4 );
const float4 g_ReflectRefractScale	: register( c5 ); // xy - reflect scale, zw - refract scale

const float4 g_PixelFogParams		: register( c8 );


static const bool g_bReflect = REFLECT ? true : false;
static const bool g_bRefract = REFRACT ? true : false;

struct PS_INPUT
{
	float4 vBumpTexCoordXY_vTexCoordXY : TEXCOORD0;
	half3 vTangentEyeVect			: TEXCOORD1;
	float4 vReflectXY_vRefractYX	: TEXCOORD2;
	float W							: TEXCOORD3;
	float4 vProjPos					: TEXCOORD4;
	float screenCoord				: TEXCOORD5;
#if BASETEXTURE
// CENTROID: TEXCOORD6
	HALF4 lightmapTexCoord1And2		: TEXCOORD6;
// CENTROID: TEXCOORD7
	HALF4 lightmapTexCoord3			: TEXCOORD7;
#endif

	float4 fogFactorW				: COLOR1;
};

float4 main( PS_INPUT i ) : COLOR
{
	// Load normal and expand range
	HALF4 vNormalSample = tex2D( NormalSampler, i.vBumpTexCoordXY_vTexCoordXY.xy );
	HALF3 vNormal = normalize( vNormalSample * 2.0 - 1.0 );

	// Perform division by W only once
	float ooW = 1.0f / i.W;

	float2 unwarpedRefractTexCoord = i.vReflectXY_vRefractYX.wz * ooW;

	float4 reflectRefractScale = g_ReflectRefractScale;

	// Compute coordinates for sampling Reflection
	float2 vReflectTexCoord;
	float2 vRefractTexCoord;

	// vectorize the dependent UV calculations (reflect = .xy, refract = .wz)
#ifdef NV3X
	float4 vDependentTexCoords = vNormal.xyxy * vNormalSample.a * reflectRefractScale;
#else
	float4 vN;
	vN.xy = vNormal.xy;
	vN.w = vNormal.x;
	vN.z = vNormal.y;
	float4 vDependentTexCoords = vN * vNormalSample.a * reflectRefractScale;
#endif

	vDependentTexCoords += ( i.vReflectXY_vRefractYX * ooW );
	vReflectTexCoord = vDependentTexCoords.xy;
	vRefractTexCoord = vDependentTexCoords.wz;

	// Sample reflection and refraction
	HALF4 vReflectColor = tex2D( ReflectSampler, vReflectTexCoord );
	HALF4 vRefractColor = tex2D( RefractSampler, vRefractTexCoord );
	vReflectColor *= vReflectTint;
	vRefractColor *= vRefractTint;
	
	half3 vEyeVect;
	vEyeVect = normalize( i.vTangentEyeVect );

	// Fresnel term
	HALF fNdotV = saturate( dot( vEyeVect, vNormal ) );
	HALF fFresnelScalar = g_FresnelConstants.x * pow( 1.0 - fNdotV, g_FresnelConstants.y ) + g_FresnelConstants.z;
	HALF4 fFresnel = HALF4( fFresnelScalar, fFresnelScalar, fFresnelScalar, fFresnelScalar ); 

#if BASETEXTURE
	float4 baseSample = tex2D( BaseTextureSampler, i.vBumpTexCoordXY_vTexCoordXY.zw );
	HALF2 bumpCoord1;
	HALF2 bumpCoord2;
	HALF2 bumpCoord3;
	ComputeBumpedLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy,
		bumpCoord1, bumpCoord2, bumpCoord3 );
	
	HALF4 lightmapSample1 = tex2D( LightmapSampler, bumpCoord1 );
	HALF3 lightmapColor1 = lightmapSample1.rgb;
	HALF3 lightmapColor2 = tex2D( LightmapSampler, bumpCoord2 );
	HALF3 lightmapColor3 = tex2D( LightmapSampler, bumpCoord3 );

	float3 dp;
	dp.x = saturate( dot( vNormal, bumpBasis[0] ) );
	dp.y = saturate( dot( vNormal, bumpBasis[1] ) );
	dp.z = saturate( dot( vNormal, bumpBasis[2] ) );
	dp *= dp;
	
	float3 diffuseLighting = dp.x * lightmapColor1 +
						dp.y * lightmapColor2 +
						dp.z * lightmapColor3;
	float sum = dot( dp, float3( 1.0f, 1.0f, 1.0f ) );
	diffuseLighting *= LIGHT_MAP_SCALE / sum;
	HALF3 diffuseComponent = baseSample.rgb * diffuseLighting;
#endif

	float4 flMask;
#if ENVMAPMASK
	flMask = tex2D( EnvMapMaskSampler, i.vBumpTexCoordXY_vTexCoordXY.zw );
#else
	flMask = float4( 1.0f, 1.0f, 1.0f, 1.0f );
#endif
	
	// NOTE: the BASETEXTURE path hasn't been tested (or really written for that matter, just copied from water)
	// What I think should happen is that the alpha of base texture should be its 'translucency'
	// which should indicate how much refraction to use.
	// We should add an envmapmask to deal with how much reflection to use
	// along with all the focus, etc. features
	float4 result;
	float flAlpha = 1.0f;
	if( g_bReflect && g_bRefract )
	{
		result = lerp( vRefractColor, vReflectColor, fFresnel ) * flMask;
#if BASETEXTURE
		result += float4( diffuseComponent, 1.0f );
		flAlpha = baseSample.a;
#endif
	}
	else if( g_bReflect )
	{
#if BASETEXTURE
		result = float4( diffuseComponent, 1.0f ) + vReflectColor * flMask;
		flAlpha = baseSample.a;
#else
		result = vReflectColor;
#endif
	}
	else if( g_bRefract )
	{
#if BASETEXTURE
		result = float4( diffuseComponent, 1.0f ) + vRefractColor * flMask;
		flAlpha = baseSample.a;
#else
		result = vRefractColor;
#endif
	}
	else
	{
#if BASETEXTURE
		result = float4( diffuseComponent, 1.0f );
		flAlpha = baseSample.a;
#else
		result = float4( 0.0f, 0.0f, 0.0f, 0.0f );
#endif
	}
	
		
#if ( PIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE )
	float fogFactor = CalcRangeFog( i.vProjPos.z, g_PixelFogParams.x, g_PixelFogParams.z, g_PixelFogParams.w );
#else
	float fogFactor = 0;
#endif
	
	return FinalOutput( float4( result.rgb, flAlpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_NONE, (WRITE_DEPTH_TO_DESTALPHA != 0), i.vProjPos.z );
}

