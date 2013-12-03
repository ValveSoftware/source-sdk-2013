//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Common pixel shader code
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_PS_FXC_H_
#define COMMON_PS_FXC_H_

#include "common_fxc.h"

// Put global skip commands here. . make sure and check that the appropriate vars are defined
// so these aren't used on the wrong shaders!

// --------------------------------------------------------------------------------
// HDR should never be enabled if we don't aren't running in float or integer HDR mode.
// SKIP: defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// --------------------------------------------------------------------------------
// We don't ever write water fog to dest alpha if we aren't doing water fog.
// SKIP: defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// --------------------------------------------------------------------------------
// We don't need fog in the pixel shader if we aren't in float fog mode2
// NOSKIP: defined $HDRTYPE && defined $HDRENABLED && defined $PIXELFOGTYPE && $HDRTYPE != HDR_TYPE_FLOAT && $FOGTYPE != 0
// --------------------------------------------------------------------------------
// We don't do HDR and LIGHTING_PREVIEW at the same time since it's running LDR in hammer.
//  SKIP: defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// --------------------------------------------------------------------------------
// Ditch all fastpath attempts if we are doing LIGHTING_PREVIEW.
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// --------------------------------------------------------------------------------
// Ditch flashlight depth when flashlight is disabled
//  SKIP: ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// --------------------------------------------------------------------------------

// System defined pixel shader constants

#if defined( _X360 )
const bool g_bHighQualityShadows : register( b0 );
#endif

// NOTE: w == 1.0f / (Dest alpha compressed depth range).
const float4 g_LinearFogColor : register( c29 );
#define OO_DESTALPHA_DEPTH_RANGE (g_LinearFogColor.w)

// Linear and gamma light scale values
const float4 cLightScale : register( c30 );
#define LINEAR_LIGHT_SCALE (cLightScale.x)
#define LIGHT_MAP_SCALE (cLightScale.y)
#define ENV_MAP_SCALE (cLightScale.z)
#define GAMMA_LIGHT_SCALE (cLightScale.w)

// Flashlight constants
#if defined(SHADER_MODEL_PS_2_0) || defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)
 const float4 cFlashlightColor       : register( c28 );
 const float4 cFlashlightScreenScale : register( c31 ); // .zw are currently unused
 #define flFlashlightNoLambertValue cFlashlightColor.w // This is either 0.0 or 2.0
#endif

#define HDR_INPUT_MAP_SCALE 16.0f

#define TONEMAP_SCALE_NONE 0
#define TONEMAP_SCALE_LINEAR 1
#define TONEMAP_SCALE_GAMMA 2

#define PIXEL_FOG_TYPE_NONE -1 //MATERIAL_FOG_NONE is handled by PIXEL_FOG_TYPE_RANGE, this is for explicitly disabling fog in the shader
#define PIXEL_FOG_TYPE_RANGE 0 //range+none packed together in ps2b. Simply none in ps20 (instruction limits)
#define PIXEL_FOG_TYPE_HEIGHT 1

// If you change these, make the corresponding change in hardwareconfig.cpp
#define NVIDIA_PCF_POISSON	0
#define ATI_NOPCF			1
#define ATI_NO_PCF_FETCH4	2

struct LPREVIEW_PS_OUT
{
	float4 color : COLOR0;
	float4 normal : COLOR1;
	float4 position : COLOR2;
	float4 flags : COLOR3;
};

/*
// unused
HALF Luminance( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f), HALF_CONSTANT(0.59f), HALF_CONSTANT(0.11f) ) );
}
*/

/*
// unused
HALF LuminanceScaled( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.59f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.11f) / MAX_HDR_OVERBRIGHT ) );
}
*/

/*
// unused
HALF AvgColor( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f) ) );
}
*/

/*
// unused
HALF4 DiffuseBump( sampler lightmapSampler,
                   float2  lightmapTexCoord1,
                   float2  lightmapTexCoord2,
                   float2  lightmapTexCoord3,
                   HALF3   normal )
{
	HALF3 lightmapColor1 = tex2D( lightmapSampler, lightmapTexCoord1 );
	HALF3 lightmapColor2 = tex2D( lightmapSampler, lightmapTexCoord2 );
	HALF3 lightmapColor3 = tex2D( lightmapSampler, lightmapTexCoord3 );

	HALF3 diffuseLighting;
	diffuseLighting = saturate( dot( normal, bumpBasis[0] ) ) * lightmapColor1 +
					  saturate( dot( normal, bumpBasis[1] ) ) * lightmapColor2 +
					  saturate( dot( normal, bumpBasis[2] ) ) * lightmapColor3;

	return HALF4( diffuseLighting, LuminanceScaled( diffuseLighting ) );
}
*/


/*
// unused
HALF Fresnel( HALF3 normal,
              HALF3 eye,
              HALF2 scaleBias )
{
	HALF fresnel = HALF_CONSTANT(1.0f) - dot( normal, eye );
	fresnel = pow( fresnel, HALF_CONSTANT(5.0f) );

	return fresnel * scaleBias.x + scaleBias.y;
}
*/

/*
// unused
HALF4 GetNormal( sampler normalSampler,
                 float2 normalTexCoord )
{
	HALF4 normal = tex2D( normalSampler, normalTexCoord );
	normal.rgb = HALF_CONSTANT(2.0f) * normal.rgb - HALF_CONSTANT(1.0f);

	return normal;
}
*/

// Needs to match NormalDecodeMode_t enum in imaterialsystem.h
#define NORM_DECODE_NONE			0
#define NORM_DECODE_ATI2N			1
#define NORM_DECODE_ATI2N_ALPHA		2

float4 DecompressNormal( sampler NormalSampler, float2 tc, int nDecompressionMode, sampler AlphaSampler )
{
	float4 normalTexel = tex2D( NormalSampler, tc );
	float4 result;

	if ( nDecompressionMode == NORM_DECODE_NONE )
	{
		result = float4(normalTexel.xyz * 2.0f - 1.0f, normalTexel.a );
	}
	else if ( nDecompressionMode == NORM_DECODE_ATI2N )
	{
		result.xy = normalTexel.xy * 2.0f - 1.0f;
		result.z = sqrt( 1.0f - dot(result.xy, result.xy) );
		result.a = 1.0f;
	}
	else // ATI2N plus ATI1N for alpha
	{
		result.xy = normalTexel.xy * 2.0f - 1.0f;
		result.z = sqrt( 1.0f - dot(result.xy, result.xy) );
		result.a = tex2D( AlphaSampler, tc ).x;					// Note that this comes in on the X channel
	}

	return result;
}

float4 DecompressNormal( sampler NormalSampler, float2 tc, int nDecompressionMode )
{
	return DecompressNormal( NormalSampler, tc, nDecompressionMode, NormalSampler );
}


HALF3 NormalizeWithCubemap( sampler normalizeSampler, HALF3 input )
{
//	return texCUBE( normalizeSampler, input ) * 2.0f - 1.0f;
	return texCUBE( normalizeSampler, input );
}

/*
HALF4 EnvReflect( sampler envmapSampler,
				 sampler normalizeSampler,
				 HALF3 normal,
				 float3 eye,
				 HALF2 fresnelScaleBias )
{
	HALF3 normEye = NormalizeWithCubemap( normalizeSampler, eye );
	HALF fresnel = Fresnel( normal, normEye, fresnelScaleBias );
	HALF3 reflect = CalcReflectionVectorUnnormalized( normal, eye );
	return texCUBE( envmapSampler, reflect );
}
*/

float CalcWaterFogAlpha( const float flWaterZ, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ, const float flFogOORange )
{
//	float flDepthFromWater = flWaterZ - flWorldPosZ + 2.0f; // hackity hack . .this is for the DF_FUDGE_UP in view_scene.cpp
	float flDepthFromWater = flWaterZ - flWorldPosZ;

	// if flDepthFromWater < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	// We'll do this with the saturate at the end instead.
//	flDepthFromWater = max( 0.0f, flDepthFromWater );

	// Calculate the ratio of water fog to regular fog (ie. how much of the distance from the viewer
	// to the vert is actually underwater.
	float flDepthFromEye = flEyePosZ - flWorldPosZ;
	float f = saturate(flDepthFromWater * (1.0/flDepthFromEye));

	// $tmp.w is now the distance that we see through water.
	return saturate(f * flProjPosZ * flFogOORange);
}

float CalcRangeFog( const float flProjPosZ, const float flFogStartOverRange, const float flFogMaxDensity, const float flFogOORange )
{
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) //Minimum requirement of ps2b
	return saturate( min( flFogMaxDensity, (flProjPosZ * flFogOORange) - flFogStartOverRange ) );
#else
	return 0.0f; //ps20 shaders will never have range fog enabled because too many ran out of slots.
#endif
}

float CalcPixelFogFactor( int iPIXELFOGTYPE, const float4 fogParams, const float flEyePosZ, const float flWorldPosZ, const float flProjPosZ )
{
	float retVal;
	if ( iPIXELFOGTYPE == PIXEL_FOG_TYPE_NONE )
	{
		retVal = 0.0f;
	}
	if ( iPIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE ) //range fog, or no fog depending on fog parameters
	{
		retVal = CalcRangeFog( flProjPosZ, fogParams.x, fogParams.z, fogParams.w );
	}
	else if ( iPIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT ) //height fog
	{
		retVal = CalcWaterFogAlpha( fogParams.y, flEyePosZ, flWorldPosZ, flProjPosZ, fogParams.w );
	}

	return retVal;
}

//g_FogParams not defined by default, but this is the same layout for every shader that does define it
#define g_FogEndOverRange	g_FogParams.x
#define g_WaterZ			g_FogParams.y
#define g_FogMaxDensity		g_FogParams.z
#define g_FogOORange		g_FogParams.w

float3 BlendPixelFog( const float3 vShaderColor, float pixelFogFactor, const float3 vFogColor, const int iPIXELFOGTYPE )
{
	if( iPIXELFOGTYPE == PIXEL_FOG_TYPE_RANGE ) //either range fog or no fog depending on fog parameters and whether this is ps20 or ps2b
	{
#	if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) //Minimum requirement of ps2b
		pixelFogFactor = saturate( pixelFogFactor );
		return lerp( vShaderColor.rgb, vFogColor.rgb, pixelFogFactor * pixelFogFactor ); //squaring the factor will get the middle range mixing closer to hardware fog
#	else
		return vShaderColor;
#	endif
	}
	else if( iPIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT )
	{
		return lerp( vShaderColor.rgb, vFogColor.rgb, saturate( pixelFogFactor ) );
	}
	else if( iPIXELFOGTYPE == PIXEL_FOG_TYPE_NONE )
	{
		return vShaderColor;
	}
}


#if ((defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0)) && ( CONVERT_TO_SRGB != 0 ) )
sampler1D GammaTableSampler : register( s15 );

float3 SRGBOutput( const float3 vShaderColor )
{	
	//On ps2b capable hardware we always have the linear->gamma conversion table texture in sampler s15.
	float3 result;
	result.r = tex1D( GammaTableSampler, vShaderColor.r ).r;
	result.g = tex1D( GammaTableSampler, vShaderColor.g ).r;
	result.b = tex1D( GammaTableSampler, vShaderColor.b ).r;
	return result;	
}

#else

float3 SRGBOutput( const float3 vShaderColor )
{
	return vShaderColor; //ps 1.1, 1.4, and 2.0 never do srgb conversion in the pixel shader
}

#endif


float SoftParticleDepth( float flDepth )
{
	return flDepth * OO_DESTALPHA_DEPTH_RANGE;
}


float DepthToDestAlpha( const float flProjZ )
{
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) //Minimum requirement of ps2b
	return SoftParticleDepth( flProjZ );
#else
	return 1.0f;
#endif
}


float4 FinalOutput( const float4 vShaderColor, float pixelFogFactor, const int iPIXELFOGTYPE, const int iTONEMAP_SCALE_TYPE, const bool bWriteDepthToDestAlpha = false, const float flProjZ = 1.0f )
{
	float4 result;
	if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_LINEAR )
	{
		result.rgb = vShaderColor.rgb * LINEAR_LIGHT_SCALE;
	}
	else if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_GAMMA )
	{
		result.rgb = vShaderColor.rgb * GAMMA_LIGHT_SCALE;
	}
	else if( iTONEMAP_SCALE_TYPE == TONEMAP_SCALE_NONE )
	{
		result.rgb = vShaderColor.rgb;
	}
	
	if( bWriteDepthToDestAlpha )
		result.a = DepthToDestAlpha( flProjZ );
	else
		result.a = vShaderColor.a;

	result.rgb = BlendPixelFog( result.rgb, pixelFogFactor, g_LinearFogColor.rgb, iPIXELFOGTYPE );
	
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) //Minimum requirement of ps2b
	result.rgb = SRGBOutput( result.rgb ); //SRGB in pixel shader conversion
#endif

	return result;
}

LPREVIEW_PS_OUT FinalOutput( const LPREVIEW_PS_OUT vShaderColor, float pixelFogFactor, const int iPIXELFOGTYPE, const int iTONEMAP_SCALE_TYPE )
{
	LPREVIEW_PS_OUT result;
	result.color = FinalOutput( vShaderColor.color, pixelFogFactor, iPIXELFOGTYPE, iTONEMAP_SCALE_TYPE );
	result.normal.rgb = SRGBOutput( vShaderColor.normal.rgb );
	result.normal.a = vShaderColor.normal.a;

	result.position.rgb = SRGBOutput( vShaderColor.position.rgb );
	result.position.a = vShaderColor.position.a;

	result.flags.rgb = SRGBOutput( vShaderColor.flags.rgb );	
	result.flags.a = vShaderColor.flags.a;

	return result;
}




float RemapValClamped( float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = saturate( cVal );

	return C + (D - C) * cVal;
}


//===================================================================================//
// This is based on Natasha Tatarchuk's Parallax Occlusion Mapping (ATI)
//===================================================================================//
// INPUT:
//		inTexCoord: 
//			the texcoord for the height/displacement map before parallaxing
//
//		vParallax:
//			Compute initial parallax displacement direction:
//			float2 vParallaxDirection = normalize( vViewTS.xy );
//			float fLength = length( vViewTS );
//			float fParallaxLength = sqrt( fLength * fLength - vViewTS.z * vViewTS.z ) / vViewTS.z; 
//			Out.vParallax = vParallaxDirection * fParallaxLength * fProjectedBumpHeight;
//
//		vNormal:
//			tangent space normal
//
//		vViewW: 
//			float3 vViewW = /*normalize*/(mul( matViewInverse, float4( 0, 0, 0, 1)) - inPosition );
//
// OUTPUT:
//		the new texcoord after parallaxing
float2 CalcParallaxedTexCoord( float2 inTexCoord, float2 vParallax, float3 vNormal, 
							   float3 vViewW, sampler HeightMapSampler )
{
	const int nMinSamples = 8;
	const int nMaxSamples = 50;

   //  Normalize the incoming view vector to avoid artifacts:
//   vView = normalize( vView );
   vViewW = normalize( vViewW );
//   vLight = normalize( vLight );
   
   // Change the number of samples per ray depending on the viewing angle
   // for the surface. Oblique angles require smaller step sizes to achieve 
   // more accurate precision         
   int nNumSteps = (int) lerp( nMaxSamples, nMinSamples, dot( vViewW, vNormal ) );
      
   float4 cResultColor = float4( 0, 0, 0, 1 );
         
   //===============================================//
   // Parallax occlusion mapping offset computation //
   //===============================================//      
   float fCurrHeight = 0.0;
   float fStepSize   = 1.0 / (float) nNumSteps;
   float fPrevHeight = 1.0;
   float fNextHeight = 0.0;

   int nStepIndex = 0;
//   bool bCondition = true;
   
   float2 dx = ddx( inTexCoord );
   float2 dy = ddy( inTexCoord );
   
   float2 vTexOffsetPerStep = fStepSize * vParallax;
   
   float2 vTexCurrentOffset = inTexCoord;
   float fCurrentBound = 1.0;
   
   float x = 0;
   float y = 0;
   float xh = 0;
   float yh = 0;   
   
   float2 texOffset2 = 0;
   
   bool bCondition = true;
   while ( bCondition == true && nStepIndex < nNumSteps ) 
   {
      vTexCurrentOffset -= vTexOffsetPerStep;
      
      fCurrHeight = tex2Dgrad( HeightMapSampler, vTexCurrentOffset, dx, dy ).r;
            
      fCurrentBound -= fStepSize;
      
      if ( fCurrHeight > fCurrentBound ) 
      {                
         x  = fCurrentBound; 
         y  = fCurrentBound + fStepSize; 
         xh = fCurrHeight;
         yh = fPrevHeight;
         
         texOffset2 = vTexCurrentOffset - vTexOffsetPerStep;
                  
         bCondition = false;
      }
      else
      {
         nStepIndex++;
         fPrevHeight = fCurrHeight;
      }
     
   }   // End of while ( bCondition == true && nStepIndex > -1 )#else

   fCurrentBound -= fStepSize;
   
   float fParallaxAmount;
   float numerator = (x * (y - yh) - y * (x - xh));
   float denomenator = ((y - yh) - (x - xh));
	// avoid NaN generation
   if( ( numerator == 0.0f ) && ( denomenator == 0.0f ) )
   {
      fParallaxAmount = 0.0f;
   }
   else
   {
      fParallaxAmount = numerator / denomenator;
   }

   float2 vParallaxOffset = vParallax * (1 - fParallaxAmount );

   // Sample the height at the next possible step:
   fNextHeight = tex2Dgrad( HeightMapSampler, texOffset2, dx, dy ).r;
   
   // Original offset:
   float2 texSampleBase = inTexCoord - vParallaxOffset;

   return texSampleBase;

#if 0
   cResultColor.rgb = ComputeDiffuseColor( texSampleBase, vLight );
        
   float fBound = 1.0 - fStepSize * nStepIndex;
   if ( fNextHeight < fCurrentBound )
//    if( 0 )
   {
      //void DoIteration( in float2 vParallaxJittered, in float3 vLight, inout float4 cResultColor )
      //cResultColor.rgb = float3(1,0,0);
      DoIteration( vParallax + vPixelSize, vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax - vPixelSize, vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax + float2( -vPixelSize.x, vPixelSize.y ), vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );
      DoIteration( vParallax + float2( vPixelSize.x, -vPixelSize.y ), vLight, fStepSize, inTexCoord, nStepIndex, dx, dy, fBound, cResultColor );

      cResultColor.rgb /= 5;
//      cResultColor.rgb = float3( 1.0f, 0.0f, 0.0f );
   }   // End of if ( fNextHeight < fCurrentBound )
  
#if DOSHADOWS
   {
      //============================================//
      // Soft shadow and self-occlusion computation //
      //============================================//
      // Compute the blurry shadows (note that this computation takes into 
      // account self-occlusion for shadow computation):
      float sh0 =  tex2D( sNormalMap, texSampleBase).w;
      float shA = (tex2D( sNormalMap, texSampleBase + inXY * 0.88 ).w - sh0 - 0.88 ) *  1 * fShadowSoftening;
      float sh9 = (tex2D( sNormalMap, texSampleBase + inXY * 0.77 ).w - sh0 - 0.77 ) *  2 * fShadowSoftening;
      float sh8 = (tex2D( sNormalMap, texSampleBase + inXY * 0.66 ).w - sh0 - 0.66 ) *  4 * fShadowSoftening;
      float sh7 = (tex2D( sNormalMap, texSampleBase + inXY * 0.55 ).w - sh0 - 0.55 ) *  6 * fShadowSoftening;
      float sh6 = (tex2D( sNormalMap, texSampleBase + inXY * 0.44 ).w - sh0 - 0.44 ) *  8 * fShadowSoftening;
      float sh5 = (tex2D( sNormalMap, texSampleBase + inXY * 0.33 ).w - sh0 - 0.33 ) * 10 * fShadowSoftening;
      float sh4 = (tex2D( sNormalMap, texSampleBase + inXY * 0.22 ).w - sh0 - 0.22 ) * 12 * fShadowSoftening;
      
      // Compute the actual shadow strength:
      float fShadow = 1 - max( max( max( max( max( max( shA, sh9 ), sh8 ), sh7 ), sh6 ), sh5 ), sh4 );

      cResultColor.rgb *= fShadow * 0.6 + 0.4;
   }
#endif
   
   return cResultColor;
#endif
}


//======================================//
// HSL Color space conversion routines  //
//======================================//

#define HUE          0
#define SATURATION   1
#define LIGHTNESS    2

// Convert from RGB to HSL color space
float4 RGBtoHSL( float4 inColor )
{
   float h, s;
   float flMax = max( inColor.r, max( inColor.g, inColor.b ) );
   float flMin = min( inColor.r, min( inColor.g, inColor.b ) );
   
   float l = (flMax + flMin) / 2.0f;
   
   if (flMax == flMin)   // achromatic case
   {
      s = h = 0;
   }
   else                  // chromatic case
   {
      // Next, calculate the hue
      float delta = flMax - flMin;
      
      // First, calculate the saturation
      if (l < 0.5f)      // If we're in the lower hexcone
      {
         s = delta/(flMax + flMin);
      }
      else
      {
         s = delta/(2 - flMax - flMin);
      }
      
      if ( inColor.r == flMax )
      {
         h = (inColor.g - inColor.b)/delta;      // color between yellow and magenta
      }
      else if ( inColor.g == flMax )
      {
         h = 2 + (inColor.b - inColor.r)/delta;  // color between cyan and yellow
      }
      else // blue must be max
      {
         h = 4 + (inColor.r - inColor.g)/delta;  // color between magenta and cyan
      }
      
      h *= 60.0f;
      
      if (h < 0.0f)
      {
         h += 360.0f;
      }
      
      h /= 360.0f;  
   }

   return float4 (h, s, l, 1.0f);
}

float HueToRGB( float v1, float v2, float vH )
{
   float fResult = v1;
   
   vH = fmod (vH + 1.0f, 1.0f);

   if ( ( 6.0f * vH ) < 1.0f )
   {
      fResult = ( v1 + ( v2 - v1 ) * 6.0f * vH );
   }
   else if ( ( 2.0f * vH ) < 1.0f )
   {
      fResult = ( v2 );
   }
   else if ( ( 3.0f * vH ) < 2.0f )
   {
      fResult = ( v1 + ( v2 - v1 ) * ( ( 2.0f / 3.0f ) - vH ) * 6.0f );
   }

   return fResult;
}

// Convert from HSL to RGB color space
float4 HSLtoRGB( float4 hsl )
{
   float r, g, b;
   float h = hsl[HUE];
   float s = hsl[SATURATION];
   float l = hsl[LIGHTNESS];

   if ( s == 0 )
   {
      r = g = b = l;
   }
   else
   {
      float v1, v2;
      
      if ( l < 0.5f )
         v2 = l * ( 1.0f + s );
      else
         v2 = ( l + s ) - ( s * l );

      v1 = 2 * l - v2;

      r = HueToRGB( v1, v2, h + ( 1.0f / 3.0f ) );
      g = HueToRGB( v1, v2, h );
      b = HueToRGB( v1, v2, h - ( 1.0f / 3.0f ) );
   }
  
   return float4( r, g, b, 1.0f );
}


// texture combining modes for combining base and detail/basetexture2
#define TCOMBINE_RGB_EQUALS_BASE_x_DETAILx2 0				// original mode
#define TCOMBINE_RGB_ADDITIVE 1								// base.rgb+detail.rgb*fblend
#define TCOMBINE_DETAIL_OVER_BASE 2
#define TCOMBINE_FADE 3										// straight fade between base and detail.
#define TCOMBINE_BASE_OVER_DETAIL 4                         // use base alpha for blend over detail
#define TCOMBINE_RGB_ADDITIVE_SELFILLUM 5                   // add detail color post lighting
#define TCOMBINE_RGB_ADDITIVE_SELFILLUM_THRESHOLD_FADE 6
#define TCOMBINE_MOD2X_SELECT_TWO_PATTERNS 7				// use alpha channel of base to select between mod2x channels in r+a of detail
#define TCOMBINE_MULTIPLY 8
#define TCOMBINE_MASK_BASE_BY_DETAIL_ALPHA 9                // use alpha channel of detail to mask base
#define TCOMBINE_SSBUMP_BUMP 10								// use detail to modulate lighting as an ssbump
#define TCOMBINE_SSBUMP_NOBUMP 11					// detail is an ssbump but use it as an albedo. shader does the magic here - no user needs to specify mode 11

float4 TextureCombine( float4 baseColor, float4 detailColor, int combine_mode,
					   float fBlendFactor )
{
	if ( combine_mode == TCOMBINE_MOD2X_SELECT_TWO_PATTERNS)
	{
		float3 dc=lerp(detailColor.r,detailColor.a, baseColor.a);
		baseColor.rgb*=lerp(float3(1,1,1),2.0*dc,fBlendFactor);
	}
	if ( combine_mode == TCOMBINE_RGB_EQUALS_BASE_x_DETAILx2)
		baseColor.rgb*=lerp(float3(1,1,1),2.0*detailColor.rgb,fBlendFactor);
	if ( combine_mode == TCOMBINE_RGB_ADDITIVE )
 		baseColor.rgb += fBlendFactor * detailColor.rgb;
	if ( combine_mode == TCOMBINE_DETAIL_OVER_BASE )
	{
		float fblend=fBlendFactor * detailColor.a;
		baseColor.rgb = lerp( baseColor.rgb, detailColor.rgb, fblend);
	}
	if ( combine_mode == TCOMBINE_FADE )
	{
		baseColor = lerp( baseColor, detailColor, fBlendFactor);
	}
	if ( combine_mode == TCOMBINE_BASE_OVER_DETAIL )
	{
		float fblend=fBlendFactor * (1-baseColor.a);
		baseColor.rgb = lerp( baseColor.rgb, detailColor.rgb, fblend );
		baseColor.a = detailColor.a;
	}
	if ( combine_mode == TCOMBINE_MULTIPLY )
	{
		baseColor = lerp( baseColor, baseColor*detailColor, fBlendFactor);
	}

	if (combine_mode == TCOMBINE_MASK_BASE_BY_DETAIL_ALPHA )
	{
		baseColor.a = lerp( baseColor.a, baseColor.a*detailColor.a, fBlendFactor );
	}
	if ( combine_mode == TCOMBINE_SSBUMP_NOBUMP )
	{
		baseColor.rgb = baseColor.rgb * dot( detailColor.rgb, 2.0/3.0 );
	}
	return baseColor;
}

float3 lerp5(float3 f1, float3 f2, float i1, float i2, float x)
{
  return f1+(f2-f1)*(x-i1)/(i2-i1);
}

float3 TextureCombinePostLighting( float3 lit_baseColor, float4 detailColor, int combine_mode,
								   float fBlendFactor )
{
	if ( combine_mode == TCOMBINE_RGB_ADDITIVE_SELFILLUM )
 		lit_baseColor += fBlendFactor * detailColor.rgb;
	if ( combine_mode == TCOMBINE_RGB_ADDITIVE_SELFILLUM_THRESHOLD_FADE )
	{
 		// fade in an unusual way - instead of fading out color, remap an increasing band of it from
 		// 0..1
		//if (fBlendFactor > 0.5)
		//	lit_baseColor += min(1, (1.0/fBlendFactor)*max(0, detailColor.rgb-(1-fBlendFactor) ) );
		//else
		//	lit_baseColor += 2*fBlendFactor*2*max(0, detailColor.rgb-.5);

		float f = fBlendFactor - 0.5;
		float fMult = (f >= 0) ? 1.0/fBlendFactor : 4*fBlendFactor;
		float fAdd = (f >= 0) ? 1.0-fMult : -0.5*fMult;
		lit_baseColor += saturate(fMult * detailColor.rgb + fAdd);
	}
	return lit_baseColor;
}

//NOTE: On X360. fProjZ is expected to be pre-reversed for cheaper math here in the pixel shader
float DepthFeathering( sampler DepthSampler, const float2 vScreenPos, float fProjZ, float fProjW, float4 vDepthBlendConstants )
{
#	if ( !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0)) ) //minimum requirement of ps2b
	{
		float flFeatheredAlpha;
		float2 flDepths;
#define flSceneDepth flDepths.x
#define flSpriteDepth flDepths.y

#		if ( defined( _X360 ) )
		{
			//Get depth from the depth texture. Need to sample with the offset of (0.5, 0.5) to fix rounding errors
			asm {
				tfetch2D flDepths.x___, vScreenPos, DepthSampler, OffsetX=0.5, OffsetY=0.5, MinFilter=point, MagFilter=point, MipFilter=point
			};

#			if(	!defined( REVERSE_DEPTH_ON_X360 ) )
				flSceneDepth = 1.0f - flSceneDepth;
#			endif

			//get the sprite depth into the same range as the texture depth
			flSpriteDepth = fProjZ / fProjW;

			//unproject to get at the pre-projection z. This value is much more linear than depth
			flDepths = vDepthBlendConstants.z / flDepths;
			flDepths = vDepthBlendConstants.y - flDepths;

			flFeatheredAlpha = flSceneDepth - flSpriteDepth;
			flFeatheredAlpha *= vDepthBlendConstants.x;
			flFeatheredAlpha = saturate( flFeatheredAlpha );
		}
#		else
		{
			flSceneDepth = tex2D( DepthSampler, vScreenPos ).a;	// PC uses dest alpha of the frame buffer
			flSpriteDepth = SoftParticleDepth( fProjZ );

			flFeatheredAlpha = abs(flSceneDepth - flSpriteDepth) * vDepthBlendConstants.x;
			flFeatheredAlpha = max( smoothstep( 0.75f, 1.0f, flSceneDepth ), flFeatheredAlpha ); //as the sprite approaches the edge of our compressed depth space, the math stops working. So as the sprite approaches the far depth, smoothly remove feathering.
			flFeatheredAlpha = saturate( flFeatheredAlpha );
		}
#		endif

#undef flSceneDepth
#undef flSpriteDepth

		return flFeatheredAlpha;
	}
#	else
	{
		return 1.0f;
	}
#	endif
}

#endif //#ifndef COMMON_PS_FXC_H_
