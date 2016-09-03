#ifndef COMMON_PARALLAX_H
#define COMMON_PARALLAX_H

#include "common_ps_fxc.h"

void GetWorldSpaceOffset( const bool bDoCalc, float3 wpos, float2 uvdelta,
							float prlxamt, float scale, out float3 worldSpaceOffset )
{
	if ( bDoCalc )
	{
		worldSpaceOffset.xy = uvdelta * scale;
		worldSpaceOffset.z = prlxamt; // * 0.7f;
	}
	else
		worldSpaceOffset = float3( 0, 0, 0 );
}

float CalcDerivativeScale( const bool bDoCalc, float2 uvddx, float2 uvddy, float3 wpos )
{
	float scale = 1;
	if ( bDoCalc )
		scale = max( length( ddx( wpos ) ) / length(uvddx), length( ddy( wpos ) ) / length(uvddy) );
	return scale;
}

float2 CalcParallaxUV_Relief( float2 inTexCoord, float3 vViewTS, float flParallaxAmt, float3 vNormal, 
													float3 vViewW, sampler HeightMapSampler,
													const int samples_min, const int samples_max, const int binary_max,
													float3 WPos, out float3 worldSpaceOffset,
													const bool bDoGradient, const bool bGetOffset )
{
	float3 p 				= float3(inTexCoord,1);
	float3 v 				= normalize(vViewTS);
	v.z						= abs(v.z);
			
	float2 dx = ddx( inTexCoord );
	float2 dy = ddy( inTexCoord );
	float scale = CalcDerivativeScale( true, dx, dy, WPos );

	flParallaxAmt /= scale;

	float depthBias			= 1.0 - v.z;
	depthBias = 1.0f - pow( depthBias, 10 );
			
	v.xy *= depthBias * flParallaxAmt;

	int linearSearchSteps = samples_max;
	if ( bDoGradient )
	{
		vViewW = normalize( vViewW );
		linearSearchSteps = (int) lerp( samples_max, samples_min, dot( vViewW, vNormal ) );
	}
	int binarySearchSteps = binary_max;
			
	v /= v.z * linearSearchSteps;
	int i;
	float curh = 0;
	v.z *= -1.0f;

	if ( !bDoGradient )
	{
		for ( i = 0; i < linearSearchSteps; i++ )
		{
			curh = tex2D( HeightMapSampler, p.xy ).r;
			if ( curh < p.z )
				p += v;
		}
	}
	else
	{
		bool bCondition = true;
		int nStepIndex = 0;
		while ( bCondition == true && nStepIndex < linearSearchSteps )
		{
			curh = tex2Dgrad( HeightMapSampler, p.xy, dx, dy ).r;
			if ( curh > p.z )
				bCondition = false;
			else
			{
				p += v;
				nStepIndex++;
			}
		}
	}

	for( i = 0; i < binarySearchSteps; i++ )
	{
		v *= 0.5f;
		float tex = tex2D(HeightMapSampler, p.xy).r;
		if (p.z > tex)
			p += v;
		else
			p -= v;
	}

	GetWorldSpaceOffset( bGetOffset, WPos, (inTexCoord-p.xy),
						(1.0f-p.z), scale, worldSpaceOffset );
	return p.xy;
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

float2 CalcParallaxUV_POM( float2 inTexCoord, float3 vViewTS, float flParallaxAmt, float3 vNormal, 
													float3 vViewW, sampler HeightMapSampler,
													const int samples_min, const int samples_max, const int binary_max,
													float3 WPos, out float3 worldSpaceOffset,
													const bool bDoGradient, const bool bGetOffset )
{
   float2 dx = ddx( inTexCoord );
   float2 dy = ddy( inTexCoord );

	float scale = CalcDerivativeScale( true, dx, dy, WPos );
	flParallaxAmt /= scale;

	float2 vParallax = normalize( vViewTS.xy );
	float fLength = length( vViewTS );
	float fParallaxLength = sqrt( fLength * fLength - vViewTS.z * vViewTS.z ) / vViewTS.z;
	vParallax *= fParallaxLength * flParallaxAmt;

   vViewW = normalize( vViewW );

   //===============================================//
   // Parallax occlusion mapping offset computation //
   //===============================================//
   int nNumSteps = (int) lerp( samples_max, samples_min, dot( vViewW, vNormal ) );

   float fCurrHeight = 0.0;
   float fStepSize   = 1.0 / (float) nNumSteps;
   float fPrevHeight = 1.0;
   float fNextHeight = 0.0;

   int nStepIndex = 0;
   
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
   //fCurrentBound -= fStepSize;
   
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

	fParallaxAmount = 1.0f - fParallaxAmount;
	float2 vParallaxOffset = vParallax * fParallaxAmount;
	float2 texSampleBase = inTexCoord - vParallaxOffset;

	vParallaxOffset = clamp( vParallaxOffset, -1, 1 );

	GetWorldSpaceOffset( bGetOffset, WPos, vParallaxOffset,
					fParallaxAmount, scale, worldSpaceOffset );

	return texSampleBase;
}
float CalcParallaxedShadows_OneLight( float2 UV, float2 UV_prlx, float3 vecLightTS, float3 WPos, float flParallaxAmt, float flSoftening, sampler Heightmap )
{
	float scale = CalcDerivativeScale( true, ddx( UV ), ddy( UV ), WPos );
	flParallaxAmt /= scale;

	float2 inXY = vecLightTS.xy * flParallaxAmt;

	float sh0 =  tex2D( Heightmap, UV_prlx).r;
	float shA = (tex2D( Heightmap, UV_prlx + inXY * 0.88 ).r - sh0 - 0.88 ) *  1 * flSoftening;
	float sh9 = (tex2D( Heightmap, UV_prlx + inXY * 0.77 ).r - sh0 - 0.77 ) *  2 * flSoftening;
	float sh8 = (tex2D( Heightmap, UV_prlx + inXY * 0.66 ).r - sh0 - 0.66 ) *  4 * flSoftening;
	float sh7 = (tex2D( Heightmap, UV_prlx + inXY * 0.55 ).r - sh0 - 0.55 ) *  6 * flSoftening;
	float sh6 = (tex2D( Heightmap, UV_prlx + inXY * 0.44 ).r - sh0 - 0.44 ) *  8 * flSoftening;
	float sh5 = (tex2D( Heightmap, UV_prlx + inXY * 0.33 ).r - sh0 - 0.33 ) * 10 * flSoftening;
	float sh4 = (tex2D( Heightmap, UV_prlx + inXY * 0.22 ).r - sh0 - 0.22 ) * 12 * flSoftening;

	float fShadow = 1.0f - max( max( max( max( max( max( shA, sh9 ), sh8 ), sh7 ), sh6 ), sh5 ), sh4 );
	return saturate( fShadow );
}
float CalcParallaxedShadows( float2 UV, float3 WPos, float flParallaxAmt, float flSoftening, sampler Heightmap )
{
	return 1;
	//float3 vecLightTS = float3( 0, 0.7f, 0 );
	//return CalcParallaxedShadows_OneLight( UV, vecLightTS, WPos, flParallaxAmt, flSoftening, Heightmap );
}

#endif