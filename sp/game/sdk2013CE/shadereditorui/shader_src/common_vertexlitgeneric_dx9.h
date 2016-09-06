#ifndef COMMON_VERTEXLITGENERIC_DX9_H_
#define COMMON_VERTEXLITGENERIC_DX9_H_

#include "common_ps_fxc.h"

//  We store four light colors and positions in an
//  array of three of these structures like so:
//
//       x		y		z      w
//    +------+------+------+------+
//    |       L0.rgb       |      |
//    +------+------+------+      |
//    |       L0.pos       |  L3  |
//    +------+------+------+  rgb |
//    |       L1.rgb       |      |
//    +------+------+------+------+
//    |       L1.pos       |      |
//    +------+------+------+      |
//    |       L2.rgb       |  L3  |
//    +------+------+------+  pos |
//    |       L2.pos       |      |
//    +------+------+------+------+
//
struct PixelShaderLightInfo
{
	float4 color;
	float4 pos;
};

#define cOverbright 2.0f
#define cOOOverbright 0.5f

#define LIGHTTYPE_NONE				0
#define LIGHTTYPE_SPOT				1
#define LIGHTTYPE_POINT				2
#define LIGHTTYPE_DIRECTIONAL		3

// Better suited to Pixel shader models, 11 instructions in pixel shader
float3 PixelShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 linearColor, nSquared = worldNormal * worldNormal;
	float3 isNegative = ( worldNormal < 0.0 );
	float3 isPositive = 1-isNegative;

	isNegative *= nSquared;
	isPositive *= nSquared;

	linearColor = isPositive.x * cAmbientCube[0] + isNegative.x * cAmbientCube[1] +
				  isPositive.y * cAmbientCube[2] + isNegative.y * cAmbientCube[3] +
				  isPositive.z * cAmbientCube[4] + isNegative.z * cAmbientCube[5];

	return linearColor;
}

// Better suited to Vertex shader models
// Six VS instructions due to use of constant indexing (slt, mova, mul, mul, mad, mad)
float3 VertexShaderAmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCube[isNegative.x] +
	              nSquared.y * cAmbientCube[isNegative.y+2] +
	              nSquared.z * cAmbientCube[isNegative.z+4];
	return linearColor;
}

float3 AmbientLight( const float3 worldNormal, const float3 cAmbientCube[6] )
{
	// Vertex shader cases
#ifdef SHADER_MODEL_VS_1_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_1_1
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_2_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#elif SHADER_MODEL_VS_3_0
	return VertexShaderAmbientLight( worldNormal, cAmbientCube );
#else
	// Pixel shader case
	return PixelShaderAmbientLight( worldNormal, cAmbientCube );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Compute scalar diffuse term with various optional tweaks such as
//          Half Lambert and ambient occlusion
//-----------------------------------------------------------------------------
float3 DiffuseTerm(const bool bHalfLambert, const float3 worldNormal, const float3 lightDir,
				   const bool bDoAmbientOcclusion, const float fAmbientOcclusion/*,
				   const bool bDoLightingWarp, in sampler lightWarpSampler*/ )
{
	float fResult;

	float NDotL = dot( worldNormal, lightDir );				// Unsaturated dot (-1 to 1 range)

	if ( bHalfLambert )
	{
		fResult = saturate(NDotL * 0.5 + 0.5);				// Scale and bias to 0 to 1 range

		//if ( !bDoLightingWarp )
		{
			fResult *= fResult;								// Square
		}
	}
	else
	{
		fResult = saturate( NDotL );						// Saturate pure Lambertian term
	}

	if ( bDoAmbientOcclusion )
	{
		// Raise to higher powers for darker AO values
//		float fAOPower = lerp( 4.0f, 1.0f, fAmbientOcclusion );
//		result *= pow( NDotL * 0.5 + 0.5, fAOPower );
		fResult *= fAmbientOcclusion;
	}

	float3 fOut = float3( fResult, fResult, fResult );
	//if ( bDoLightingWarp )
	//{
	//	fOut = 2.0f * tex1D( lightWarpSampler, fResult );
	//}

	return fOut;
}

float3 PixelShaderDoGeneralDiffuseLight( const float fAtten, const float3 worldPos, const float3 worldNormal,
										/* in sampler NormalizeSampler,*/
										 const float3 vPosition, const float3 vColor, const bool bHalfLambert,
										 const bool bDoAmbientOcclusion, const float fAmbientOcclusion/*,
										 const bool bDoLightingWarp, in sampler lightWarpSampler*/ )
{
//#if (defined(SHADER_MODEL_PS_2_B) || defined(SHADER_MODEL_PS_3_0))
	float3 lightDir = normalize( vPosition - worldPos );
//#else
//	float3 lightDir = NormalizeWithCubemap( NormalizeSampler, vPosition - worldPos );
//#endif
	return vColor * fAtten * DiffuseTerm( bHalfLambert, worldNormal, lightDir, bDoAmbientOcclusion, fAmbientOcclusion/*, bDoLightingWarp, lightWarpSampler*/ );
}

float3 PixelShaderGetLightVector( const float3 worldPos, PixelShaderLightInfo cLightInfo[3], int nLightIndex )
{
	if ( nLightIndex == 3 )
	{
		// Unpack light 3 from w components...
		float3 vLight3Pos = float3( cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w );
		return normalize( vLight3Pos - worldPos );
	}
	else
	{
		return normalize( cLightInfo[nLightIndex].pos - worldPos );
	}
}

float3 PixelShaderGetLightColor( PixelShaderLightInfo cLightInfo[3], int nLightIndex )
{
	if ( nLightIndex == 3 )
	{
		// Unpack light 3 from w components...
		return float3( cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w );
	}
	else
	{
		return cLightInfo[nLightIndex].color.rgb;
	}
}

void GetLightData( PixelShaderLightInfo cLightInfo[3], int index, float3 worldpos,
	out float3 color, out float3 dir )
{
	color = PixelShaderGetLightColor( cLightInfo, index );
	dir = PixelShaderGetLightVector( worldpos, cLightInfo, index );
}


void SpecularAndRimTerms( const float3 vWorldNormal, const float3 vLightDir, const float fSpecularExponent,
						  const float3 vEyeDir, const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
						  /*const bool bDoSpecularWarp, in sampler specularWarpSampler,*/ const float fFresnel,
						  const float3 color, /*const bool bDoRimLighting, const float fRimExponent,*/

						  // Outputs
						  out float3 specularLighting/*, out float3 rimLighting*/ )
{
	//rimLighting = float3(0.0f, 0.0f, 0.0f);

	float3 vReflect = reflect( -vEyeDir, vWorldNormal );				// Reflect view through normal
	float LdotR = saturate(dot( vReflect, vLightDir ));					// L.R	(use half-angle instead?)
	specularLighting = pow( LdotR, fSpecularExponent );					// Raise to specular exponent

	// Optionally warp as function of scalar specular and fresnel
	//if ( bDoSpecularWarp )
	//	specularLighting *= tex2D( specularWarpSampler, float2(specularLighting.x, fFresnel) ); // Sample at { (L.R)^k, fresnel }
	specularLighting *= fFresnel; // Sample at { (L.R)^k, fresnel }

	specularLighting *= saturate(dot( vWorldNormal, vLightDir ));		// Mask with N.L
	specularLighting *= color;											// Modulate with light color

	if ( bDoAmbientOcclusion )											// Optionally modulate with ambient occlusion
		specularLighting *= fAmbientOcclusion;

	//if ( bDoRimLighting )												// Optionally do rim lighting
	//{
	//	rimLighting  = pow( LdotR, fRimExponent );						// Raise to rim exponent
	//	rimLighting *= saturate(dot( vWorldNormal, vLightDir ));		// Mask with N.L
	//	rimLighting *= color;											// Modulate with light color
	//}
}

// Traditional fresnel term approximation
float Fresnel( const float3 vNormal, const float3 vEyeDir )
{
	float fresnel = 1-saturate( dot( vNormal, vEyeDir ) );				// 1-(N.V) for Fresnel term
	return fresnel * fresnel;											// Square for a more subtle look
}

// Traditional fresnel term approximation which uses 4th power (square twice)
float Fresnel4( const float3 vNormal, const float3 vEyeDir )
{
	float fresnel = 1-saturate( dot( vNormal, vEyeDir ) );				// 1-(N.V) for Fresnel term
	fresnel = fresnel * fresnel;										// Square
	return fresnel * fresnel;											// Square again for a more subtle look
}


//
// Custom Fresnel with low, mid and high parameters defining a piecewise continuous function
// with traditional fresnel (0 to 1 range) as input.  The 0 to 0.5 range blends between
// low and mid while the 0.5 to 1 range blends between mid and high
//
//    |
//    |    .  M . . . H
//    | . 
//    L
//    |
//    +----------------
//    0               1
//
float Fresnel( const float3 vNormal, const float3 vEyeDir, float3 vRanges )
{
	float result, f = Fresnel( vNormal, vEyeDir );			// Traditional Fresnel

	if ( f > 0.5f )
		result = lerp( vRanges.y, vRanges.z, (2*f)-1 );		// Blend between mid and high values
	else
        result = lerp( vRanges.x, vRanges.y, 2*f );			// Blend between low and mid values

	return result;
}

void PixelShaderDoSpecularLight( const float3 vWorldPos, const float3 vWorldNormal, const float fSpecularExponent, const float3 vEyeDir,
								 const float fAtten, const float3 vLightColor, const float3 vLightDir,
								 const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
								 /*const bool bDoSpecularWarp, in sampler specularWarpSampler,*/ float fFresnel,
								 /*const bool bDoRimLighting, const float fRimExponent,*/

								 // Outputs
								 out float3 specularLighting/*, out float3 rimLighting*/ )
{
	// Compute Specular and rim terms
	SpecularAndRimTerms( vWorldNormal, vLightDir, fSpecularExponent,
						 vEyeDir, bDoAmbientOcclusion, fAmbientOcclusion,
						 /*bDoSpecularWarp, specularWarpSampler,*/ fFresnel, vLightColor * fAtten,
						 /*bDoRimLighting, fRimExponent,*/ specularLighting/*, rimLighting*/ );
}

float3 PixelShaderDoLightingLinear( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const bool bStaticLight,
				   const bool bAmbientLight, const float4 lightAtten, const float3 cAmbientCube[6],
				   /*in sampler NormalizeSampler,*/ const int nNumLights, PixelShaderLightInfo cLightInfo[3],
				   const bool bHalfLambert, const bool bDoAmbientOcclusion, const float fAmbientOcclusion/*,
				   const bool bDoLightingWarp, in sampler lightWarpSampler*/ )
{
	float3 linearColor = 0.0f;

	if ( bStaticLight )
	{
		// The static lighting comes in in gamma space and has also been premultiplied by $cOOOverbright
		// need to get it into
		// linear space so that we can do adds.
		linearColor += GammaToLinear( staticLightingColor * cOverbright );
	}

	if ( bAmbientLight )
	{
		float3 ambient = AmbientLight( worldNormal, cAmbientCube );

		if ( bDoAmbientOcclusion )
			ambient *= fAmbientOcclusion * fAmbientOcclusion;	// Note squaring...

		linearColor += ambient;
	}

	if ( nNumLights > 0 )
	{
		linearColor += PixelShaderDoGeneralDiffuseLight( lightAtten.x, worldPos, worldNormal, /*NormalizeSampler,*/
														 cLightInfo[0].pos, cLightInfo[0].color, bHalfLambert,
														 bDoAmbientOcclusion, fAmbientOcclusion/*,
														 bDoLightingWarp, lightWarpSampler*/ );
		if ( nNumLights > 1 )
		{
			linearColor += PixelShaderDoGeneralDiffuseLight( lightAtten.y, worldPos, worldNormal, /*NormalizeSampler,*/
															 cLightInfo[1].pos, cLightInfo[1].color, bHalfLambert,
															 bDoAmbientOcclusion, fAmbientOcclusion/*,
															 bDoLightingWarp, lightWarpSampler*/ );
			if ( nNumLights > 2 )
			{
				linearColor += PixelShaderDoGeneralDiffuseLight( lightAtten.z, worldPos, worldNormal, /*NormalizeSampler,*/
																 cLightInfo[2].pos, cLightInfo[2].color, bHalfLambert,
																 bDoAmbientOcclusion, fAmbientOcclusion/*,
																 bDoLightingWarp, lightWarpSampler*/ );
				if ( nNumLights > 3 )
				{
					// Unpack the 4th light's data from tight constant packing
					float3 vLight3Color = float3( cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w );
					float3 vLight3Pos = float3( cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w );
					linearColor += PixelShaderDoGeneralDiffuseLight( lightAtten.w, worldPos, worldNormal, /*NormalizeSampler,*/
																	 vLight3Pos, vLight3Color, bHalfLambert,
																	 bDoAmbientOcclusion, fAmbientOcclusion/*,
																	 bDoLightingWarp, lightWarpSampler*/ );
				}
			}
		}
	}

	return linearColor;
}

void PixelShaderDoSpecularLighting( const float3 worldPos, const float3 worldNormal, const float fSpecularExponent, const float3 vEyeDir,
									const float4 lightAtten, const int nNumLights, PixelShaderLightInfo cLightInfo[3],
									const bool bDoAmbientOcclusion, const float fAmbientOcclusion,
									/*const bool bDoSpecularWarp, in sampler specularWarpSampler,*/ float fFresnel,
									/*const bool bDoRimLighting, const float fRimExponent,*/

									// Outputs
									out float3 specularLighting/*, out float3 rimLighting*/ )
{
	specularLighting /*= rimLighting*/ = float3( 0.0f, 0.0f, 0.0f );
	float3 localSpecularTerm/*, localRimTerm*/;

	if( nNumLights > 0 )
	{
		PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir,
									lightAtten.x, PixelShaderGetLightColor( cLightInfo, 0 ),
									PixelShaderGetLightVector( worldPos, cLightInfo, 0 ),
									bDoAmbientOcclusion, fAmbientOcclusion,
									/*bDoSpecularWarp, specularWarpSampler,*/ fFresnel,
									/*bDoRimLighting, fRimExponent,*/
									localSpecularTerm/*, localRimTerm*/ );

		specularLighting += localSpecularTerm;		// Accumulate specular and rim terms
		/*rimLighting += localRimTerm;*/
	}

	if( nNumLights > 1 )
	{
		PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir,
									lightAtten.y, PixelShaderGetLightColor( cLightInfo, 1 ),
									PixelShaderGetLightVector( worldPos, cLightInfo, 1 ),
									bDoAmbientOcclusion, fAmbientOcclusion,
									/*bDoSpecularWarp, specularWarpSampler,*/ fFresnel,
									/*bDoRimLighting, fRimExponent,*/
									localSpecularTerm/*, localRimTerm*/ );

		specularLighting += localSpecularTerm;		// Accumulate specular and rim terms
		/*rimLighting += localRimTerm;*/
	}


	if( nNumLights > 2 )
	{
		PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir,
									lightAtten.z, PixelShaderGetLightColor( cLightInfo, 2 ),
									PixelShaderGetLightVector( worldPos, cLightInfo, 2 ),
									bDoAmbientOcclusion, fAmbientOcclusion,
									/*bDoSpecularWarp, specularWarpSampler,*/ fFresnel,
									/*bDoRimLighting, fRimExponent,*/
									localSpecularTerm/*, localRimTerm*/ );

		specularLighting += localSpecularTerm;		// Accumulate specular and rim terms
		/*rimLighting += localRimTerm;*/
	}

	if( nNumLights > 3 )
	{
		PixelShaderDoSpecularLight( worldPos, worldNormal, fSpecularExponent, vEyeDir,
									lightAtten.w, PixelShaderGetLightColor( cLightInfo, 3 ),
									PixelShaderGetLightVector( worldPos, cLightInfo, 3 ),
									bDoAmbientOcclusion, fAmbientOcclusion,
									/*bDoSpecularWarp, specularWarpSampler,*/ fFresnel,
									/*bDoRimLighting, fRimExponent,*/
									localSpecularTerm/*, localRimTerm*/ );

		specularLighting += localSpecularTerm;		// Accumulate specular and rim terms
		/*rimLighting += localRimTerm;*/
	}

}

float3 PixelShaderDoRimLighting( const float3 worldNormal, const float3 vEyeDir, const float3 cAmbientCube[6], float fFresnel )
{
	float3 vReflect = reflect( -vEyeDir, worldNormal );			// Reflect view through normal

	return fFresnel * PixelShaderAmbientLight( vEyeDir, cAmbientCube );
}

// Called directly by newer shaders or through the following wrapper for older shaders
float3 PixelShaderDoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const bool bStaticLight,
				   const bool bAmbientLight, const float4 lightAtten, const float3 cAmbientCube[6],
				   /*in sampler NormalizeSampler,*/ const int nNumLights, PixelShaderLightInfo cLightInfo[3],
				   const bool bHalfLambert,
				   
				   // New optional/experimental parameters
				   const bool bDoAmbientOcclusion, const float fAmbientOcclusion/*,
				   const bool bDoLightingWarp, in sampler lightWarpSampler*/ )
{
	float3 returnColor;

	// special case for no lighting
	if( !bStaticLight && !bAmbientLight )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( bStaticLight && !bAmbientLight  )
	{
		// special case for static lighting only
		returnColor = GammaToLinear( staticLightingColor );
	}
	else
	{
		float3 linearColor;

		linearColor = PixelShaderDoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			bStaticLight, bAmbientLight, lightAtten,
			cAmbientCube, /*NormalizeSampler,*/ nNumLights, cLightInfo, bHalfLambert,
			bDoAmbientOcclusion, fAmbientOcclusion/*,
			bDoLightingWarp, lightWarpSampler*/ );

		// go ahead and clamp to the linear space equivalent of overbright 2 so that we match
		// everything else.
//		returnColor = HuePreservingColorClamp( linearColor, pow( 2.0f, 2.2 ) );
		returnColor = linearColor;
	}

	return returnColor;
}

#endif //#ifndef COMMON_VERTEXLITGENERIC_DX9_H_
