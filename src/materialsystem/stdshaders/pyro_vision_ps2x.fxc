// STATIC: "EFFECT"						"0..3"
// STATIC: "VERTEX_LIT"					"0..1"
// STATIC: "BASETEXTURE2"				"0..1"
// STATIC: "FANCY_BLENDING"				"0..1"
// STATIC: "SELFILLUM"					"0..1"
// STATIC: "COLOR_BAR"					"0..1"
// STATIC: "STRIPES"					"0..1"
// STATIC: "STRIPES_USE_NORMAL2"		"0..1"

// DYNAMIC: "PIXELFOGTYPE"				"0..1"
// DYNAMIC: "VISUALIZE_DOF"				"0..1"
// DYNAMIC: "HEATHAZE"					"0..1"

// SKIP: ( STRIPES == 0 ) &&  ( STRIPES_USE_NORMAL2 == 1 )
// SKIP: ( EFFECT != 1 ) &&  ( COLOR_BAR == 1 )
// SKIP: ( EFFECT != 2 ) &&  ( VISUALIZE_DOF == 1 )
// SKIP: ( EFFECT == 2 || EFFECT == 3 ) && ( STRIPES == 1 || STRIPES_USE_NORMAL2 == 1 || COLOR_BAR == 1 || BASETEXTURE2 == 1 || FANCY_BLENDING == 1 || VERTEX_LIT == 1 || PIXELFOGTYPE == 1 || SELFILLUM == 1 )
// SKIP: ( EFFECT != 3 ) && ( HEATHAZE == 1 )

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler		: register( s0 );

#if BASETEXTURE2 == 1

sampler BaseTexture2Sampler		: register( s4 );

#if FANCY_BLENDING == 1

sampler BlendModulationSampler	: register( s3 );

#endif

#endif

#if STRIPES

sampler StripeSampler			: register( s5 );

const float4 g_vPyroParms6		: register( c5 );
	#define g_vStripeColor			g_vPyroParms6.rgb
	#define g_flStripeLMScale		g_vPyroParms6.a

const float4 g_vPyroParms7		: register( c6 );
	#define g_vStripeNormal1		g_vPyroParms6.xyz
	#define g_flUnused6				g_vPyroParms6.w

#if ( STRIPES_USE_NORMAL2 == 1 )

const float4 g_vPyroParms8		: register( c7 );
	#define g_vStripeNormal2		g_vPyroParms7.xyz
	#define g_flUnused7				g_vPyroParms7.w

#endif

#endif



#if EFFECT == 0

sampler LightmapSampler			: register( s1 );
sampler CanvasSampler			: register( s2 );

const float4 g_vPyroParms1		: register( c0 );
	#define g_vBaseStepRange		g_vPyroParms1.xy
	#define g_vLightmapStepRange	g_vPyroParms1.zw

const float4 g_vPyroParms2		: register( c1 );
	#define g_vColorModulation		g_vPyroParms2.rgb
	#define g_flUnused1				g_vPyroParms2.w

const float4 g_vPyroParms3		: register( c2 );
	#define g_vCanvasStepRange		g_vPyroParms3.xy
	#define g_vUnused2				g_vPyroParms3.zw

const float4 g_vPyroParms4		: register( c3 );
	#define g_vCanvasColorStart		g_vPyroParms4.rgb
	#define g_flUnused3				g_vPyroParms4.w

const float4 g_vPyroParms5		: register( c4 );
	#define g_vCanvasColorEnd		g_vPyroParms5.rgb
	#define g_flUnused4				g_vPyroParms5.w

#elif EFFECT == 1

sampler LightmapSampler			: register( s1 );
sampler ColorBarSampler			: register( s2 );

const float4 g_vPyroParms1		: register( c0 );
	#define g_flGrayPower			g_vPyroParms1.x
	#define g_flGrayStep			g_vPyroParms1.yz
	#define g_flLightMapGradients	g_vPyroParms1.w

const float4 g_vPyroParms2		: register( c1 );
	#define g_vColorModulation		g_vPyroParms2.rgb
	#define g_flDiffuseLighting		g_vPyroParms2.w

const float4 g_vPyroParms3		: register( c2 );
	#define g_flDiffuseBase			g_vPyroParms3.x
	#define g_vSelfIllumTint		g_vPyroParms3.yzw

#elif EFFECT == 2

sampler BlurredFrameSampler		: register( s4 );

const float4 g_vPyroParms1		: register( c0 );
	#define g_flDoFStartDistance	g_vPyroParms1.x
	#define g_flDoFPower			g_vPyroParms1.y
	#define g_flDoFMax				g_vPyroParms1.z
	#define g_flUnused1				g_vPyroParms1.w

#elif EFFECT == 3

sampler NoiseSampler			: register( s3 );
sampler WarpFrameSampler		: register( s4 );
sampler VignetteSampler			: register( s5 );
sampler VignetteTileSampler		: register( s6 );

const float4 g_vPyroParms1		: register( c0 );
	#define g_flNoiseScale			g_vPyroParms1.x	// 0.2
	#define g_flTimeScale			g_vPyroParms1.y // 0.020
	#define g_flHeatHazeScale		g_vPyroParms1.z	// 0.2
	#define g_flUnused1				g_vPyroParms1.w

#endif

const HALF3 g_EyePos						: register( c10 );
const HALF4 g_FogParams						: register( c11 );
const float4 g_vGeneralPyroParms1			: register( c12 );
	#define g_fWriteDepthToAlpha	g_vGeneralPyroParms1.x
	#define g_flTime				g_vGeneralPyroParms1.y
	#define g_flGeneralUnused2		g_vGeneralPyroParms1.z
	#define g_flGeneralUnused3		g_vGeneralPyroParms1.w



struct PS_INPUT
{
#if EFFECT == 0 || EFFECT == 1

	float4 vBaseAndSeamlessTexCoord	: TEXCOORD0;
#if ( STRIPES == 1 )
	float2 vStripeSeamlessTexCoord	: TEXCOORD1;
#endif
#if ( VERTEX_LIT == 0 )
	float2 vLightmapBlendTexCoord	: TEXCOORD2;
#endif
#if BASETEXTURE2 == 1
	float3 vBlendFactor				: TEXCOORD3;
#endif
	float4 worldPos_projPosZ		: TEXCOORD4;
	float3 vWorldNormal				: TEXCOORD5;
	float4 vVertexColor				: COLOR;

#else

	float2 vBaseTexCoord			: TEXCOORD0;

#endif

};


float ToGray( const float3 vColor )
{
	return 	dot( vColor, float3( 0.30f, 0.59f, 0.11f ) );
}


void HandleBlending( const float2 vBaseTextureCoord, const float3 vBlendFactor, inout float4 vBaseColor )
{
#if BASETEXTURE2 == 1

	float4	vBaseColor2 = tex2D( BaseTexture2Sampler, vBaseTextureCoord );
	float	flBlendFactor = vBlendFactor.z;

#if FANCY_BLENDING == 1

	float4	modt = tex2D( BlendModulationSampler, vBlendFactor.xy );
	float	minb = saturate( modt.g - modt.r );
	float	maxb = saturate( modt.g + modt.r );
	flBlendFactor = smoothstep( minb, maxb, flBlendFactor );

#endif

	vBaseColor = lerp( vBaseColor, vBaseColor2, flBlendFactor );

#endif
}


#if STRIPES

void CalculateStripe( const float2 vStripeSeamlessTexCoord, const float3 vWorldNormal, const float flLMScale, inout float3 vResult )
{
	float4	vStripeColor = tex2D( StripeSampler, vStripeSeamlessTexCoord );

	vStripeColor.rgb *= g_vStripeColor;
	vStripeColor.rgb *= lerp( float3( 1.0, 1.0, 1.0 ), flLMScale, g_flStripeLMScale );

#if VERTEX_LIT == 1 && 0

	float flAngle = saturate( dot( vWorldNormal, g_vStripeNormal1 ) );
	vResult.rgb = ( vWorldNormal * 0.5 ) + 0.5;
	return;

	flAngle = 1 - flAngle;
	vStripeColor.a *= flAngle;

#endif

	vResult = lerp( vResult, vStripeColor.rgb, vStripeColor.a );
}

#endif


#if EFFECT == -1

float4 main( PS_INPUT i ) : COLOR
{
	float4	vBaseColor = tex2D( BaseTextureSampler, i.vBaseAndSeamlessTexCoord.xy );

#if BASETEXTURE2 == 1
	HandleBlending( i.vBaseAndSeamlessTexCoord.xy, i.vBlendFactor, vBaseColor );
#endif

//	return vBaseColor;

	vBaseColor *= i.vVertexColor;

#if VERTEX_LIT == 0
	float4	vLightmapSample = tex2D( LightmapSampler, i.vLightmapBlendTexCoord.xy ) * float4( g_vColorModulation, 1.0 );
#endif
	float4	vCanvas = tex2D( CanvasSampler, i.vBaseAndSeamlessTexCoord.zw );

	float	flFogFactor = CalcPixelFogFactor( 0, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );

//	return saturate( min( g_FogParams.z, (i.worldPos_projPosZ.w * g_FogParams.w) - g_FogParams.x ) );

//	return float4( g_FogParams.x, g_FogParams.z, g_FogParams.w, 1 );

//	return g_LinearFogColor;
//	flFogFactor = 1;

	float4	vResult;

#if 0
	vResult = vBaseColor * vLightmapSample;
#else
	float	flBaseGray = dot( vBaseColor.rgb, float3( 0.30f, 0.59f, 0.11f ) );
#if VERTEX_LIT == 0
	float	flLightmapGray = dot( vLightmapSample.rgb, float3( 0.30f, 0.59f, 0.11f ) );
#endif

	flBaseGray = smoothstep( g_vBaseStepRange.x, g_vBaseStepRange.y, flBaseGray );
	vResult = float4( flBaseGray.rrr, vBaseColor.a );

#if VERTEX_LIT == 0
//	flLightmapGray = pow( flLightmapGray, 0.01 );
	flLightmapGray = smoothstep( g_vLightmapStepRange.x, g_vLightmapStepRange.y, flLightmapGray );
	vResult *= flLightmapGray;
#endif

	float	flCanvasGray = dot( vCanvas.rgb, float3( 0.30f, 0.59f, 0.11f ) );
	flCanvasGray = smoothstep( g_vCanvasStepRange.x, g_vCanvasStepRange.y, flCanvasGray );

	vCanvas.rgb = lerp( g_vCanvasColorStart, g_vCanvasColorEnd, flCanvasGray );

	vResult *= vCanvas;

#endif	

	return FinalOutput( vResult, flFogFactor, 0, TONEMAP_SCALE_LINEAR, true, i.worldPos_projPosZ.w );
}

#elif EFFECT == 0

float4 main( PS_INPUT i ) : COLOR
{
	float4	vBaseColor = tex2D( BaseTextureSampler, i.vBaseAndSeamlessTexCoord.xy );

#if BASETEXTURE2 == 1
	HandleBlending( i.vBaseAndSeamlessTexCoord.xy, i.vBlendFactor, vBaseColor );
#endif

	vBaseColor.rgb *= i.vVertexColor.rgb;
	vBaseColor.rgb = ceil( vBaseColor.rgb * 16.0 ) / 16.0;

#if VERTEX_LIT == 0
	float4	vLightmapSample = tex2D( LightmapSampler, i.vLightmapBlendTexCoord.xy ) * float4( g_vColorModulation, 1.0 );
#endif
	float4	vCanvas = tex2D( CanvasSampler, i.vBaseAndSeamlessTexCoord.zw );

	float	flFogFactor = CalcPixelFogFactor( 0, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );

	float4	vResult;

	float	flBaseGray = dot( vBaseColor.rgb, float3( 0.30f, 0.59f, 0.11f ) );
#if VERTEX_LIT == 0
	float	flLightmapGray = dot( vLightmapSample.rgb, float3( 0.30f, 0.59f, 0.11f ) );
#endif

#ifdef GRAY_DIFFUSE
	flBaseGray = smoothstep( g_vBaseStepRange.x, g_vBaseStepRange.y, flBaseGray );
	vResult = float4( flBaseGray.rrr, vBaseColor.a );
#else
	vResult.rgb = smoothstep( g_vBaseStepRange.xxx, g_vBaseStepRange.yyy, vBaseColor.rgb );
	vResult.a = vBaseColor.a;
#endif


#if VERTEX_LIT == 0

#ifdef GRAY_LM

	flLightmapGray = pow( flLightmapGray, 0.01 );
	flLightmapGray = smoothstep( g_vLightmapStepRange.x, g_vLightmapStepRange.y, flLightmapGray );
	vResult.rgb *= flLightmapGray;

#else

	vLightmapSample.rgb = smoothstep( g_vLightmapStepRange.xxx, g_vLightmapStepRange.yyy, vLightmapSample.rgb );
//	vLightmapSample.rgb = lerp( float3( 0.0, 0.0, 0.0 ), float3( 3.1, 1.1, 5.1 ), vLightmapSample.rgb );
	vResult.rgb *= vLightmapSample.rgb;

#endif

#endif

	float	flCanvasGray = dot( vCanvas.rgb, float3( 0.30f, 0.59f, 0.11f ) );
	flCanvasGray = smoothstep( g_vCanvasStepRange.x, g_vCanvasStepRange.y, flCanvasGray );

	vCanvas.rgb = lerp( g_vCanvasColorStart, g_vCanvasColorEnd, flCanvasGray );
//	vCanvas.rgb = pow( vCanvas.rgb, 0.6 );

	vResult *= vCanvas;

	return FinalOutput( vResult, flFogFactor, 0, TONEMAP_SCALE_LINEAR, true, i.worldPos_projPosZ.w );
}

#elif EFFECT == 1

#define GRAY_DIFFUSE 1
#define GRAY_LM	1


float4 main( PS_INPUT i ) : COLOR
{
	float4	vResult;

	float4	vBaseColor = tex2D( BaseTextureSampler, i.vBaseAndSeamlessTexCoord.xy );

#if BASETEXTURE2 == 1
	HandleBlending( i.vBaseAndSeamlessTexCoord.xy, i.vBlendFactor, vBaseColor );
#endif

	float	flFogFactor = CalcPixelFogFactor( 0, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );

	vBaseColor.rgb = lerp( vBaseColor.rgb, 1.0, g_flDiffuseBase );
	vResult = vBaseColor * i.vVertexColor;

#if VERTEX_LIT == 0
	float4	vLightmapSample = tex2D( LightmapSampler, i.vLightmapBlendTexCoord.xy ) * float4( g_vColorModulation, 1.0 );
	vResult.rgb = lerp( vResult.rgb, 0.5, g_flDiffuseLighting );
	vResult.rgb *= vLightmapSample.rgb;
#endif

#if SELFILLUM

	float3 vSelfIllumComponent = g_vSelfIllumTint * vBaseColor.rgb;
	vResult.rgb = lerp( vResult.rgb, vSelfIllumComponent, vBaseColor.a );

#endif

	float flGray =  dot( vResult.rgb, float3( 0.30f, 0.59f, 0.11f ) );

	flGray = pow( flGray, g_flGrayPower );
	flGray = smoothstep( g_flGrayStep.x, g_flGrayStep.y, flGray );
	
	flGray = ceil( flGray * g_flLightMapGradients ) / g_flLightMapGradients;

#if COLOR_BAR == 1

	float4	vCanvas = tex1D( ColorBarSampler, flGray );
	vResult.rgb = vCanvas.rgb * flGray;

#endif

#if STRIPES

	CalculateStripe( i.vStripeSeamlessTexCoord, i.vWorldNormal, flGray, vResult.rgb );

#endif

	return FinalOutput( vResult, flFogFactor, 0, TONEMAP_SCALE_LINEAR, false, i.worldPos_projPosZ.w );
}

#elif EFFECT == 2

float4 main( PS_INPUT i ) : COLOR
{
	float	flDepth = tex2D( BaseTextureSampler, i.vBaseTexCoord.xy ).r;
	float4	vBaseColor = tex2D( BlurredFrameSampler, i.vBaseTexCoord.xy );


	float flAmount;
	flAmount = flDepth;
	
	flAmount = saturate( flDepth - g_flDoFStartDistance );
	flAmount = pow( flAmount, g_flDoFPower );
	flAmount *= g_flDoFMax;

#if VISUALIZE_DOF == 1

	return float4( flAmount.rrr, 1 );

#else

	return float4( vBaseColor.rgb, flAmount );

#endif
}

#elif EFFECT == 3

float4 main( PS_INPUT i ) : COLOR
{
	float	flVignetteAmount = tex2D( VignetteSampler, i.vBaseTexCoord.xy );
	float4	vVignetteTile = tex2D( VignetteTileSampler, i.vBaseTexCoord.xy * 50.0f );

#if HEATHAZE == 1
	float2  vOffset = tex2D( WarpFrameSampler, i.vBaseTexCoord.xy ).aa;
	float	flDynamicOffset = tex2D( NoiseSampler, float2( i.vBaseTexCoord.x, ( i.vBaseTexCoord.y * g_flNoiseScale ) + ( g_flTime * g_flTimeScale ) ) ).r;

//	return float4( vOffset.yyy, 1 );
//	return float4( flDynamicOffset.xxx, 1.0 );
	
//	vOffset = ( vOffset - 0.5 );
	vOffset = smoothstep( 0.1, 1.0, vOffset );
	vOffset *= flDynamicOffset * g_flHeatHazeScale;
	float4	vOriginal = tex2D( BaseTextureSampler, i.vBaseTexCoord.xy + float2( 0, vOffset.y ) );

#else

	float4	vOriginal = tex2D( BaseTextureSampler, i.vBaseTexCoord.xy );

#endif

	float flDelta = flVignetteAmount.x - vVignetteTile.a;

	float flFinal = flDelta <= 0.0f ? 0.0f : lerp( flVignetteAmount.x, 1.0, flDelta / flVignetteAmount.x );
	vOriginal.rgb = lerp( vOriginal.rgb, vVignetteTile.rgb, flFinal * flVignetteAmount.x );

	return float4( vOriginal );
}

#endif
