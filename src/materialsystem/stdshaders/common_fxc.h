//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_FXC_H_
#define COMMON_FXC_H_

#include "common_pragmas.h"
#include "common_hlsl_cpp_consts.h"

#ifdef NV3X
#	define HALF half
#	define HALF2 half2
#	define HALF3 half3
#	define HALF4 half4
#	define HALF3x3 half3x3
#	define HALF3x4 half3x4
#	define HALF4x3 half4x3
#	define HALF_CONSTANT( _constant )	((HALF)_constant)
#else
#	define HALF float
#	define HALF2 float2
#	define HALF3 float3
#	define HALF4 float4
#	define HALF3x3 float3x3
#	define HALF3x4 float3x4
#	define HALF4x3 float4x3
#	define HALF_CONSTANT( _constant )	_constant
#endif

// This is where all common code for both vertex and pixel shaders.
#define OO_SQRT_3 0.57735025882720947f
static const HALF3 bumpBasis[3] = {
	HALF3( 0.81649661064147949f, 0.0f, OO_SQRT_3 ),
	HALF3(  -0.40824833512306213f, 0.70710676908493042f, OO_SQRT_3 ),
	HALF3(  -0.40824821591377258f, -0.7071068286895752f, OO_SQRT_3 )
};
static const HALF3 bumpBasisTranspose[3] = {
	HALF3( 0.81649661064147949f, -0.40824833512306213f, -0.40824833512306213f ),
	HALF3(  0.0f, 0.70710676908493042f, -0.7071068286895752f ),
	HALF3(  OO_SQRT_3, OO_SQRT_3, OO_SQRT_3 )
};

#if defined( _X360 )
#define REVERSE_DEPTH_ON_X360 //uncomment to use D3DFMT_D24FS8 with an inverted depth viewport for better performance. Keep this in sync with the same named #define in public/shaderapi/shareddefs.h
//Note that the reversal happens in the viewport. So ONLY reading back from a depth texture should be affected. Projected math is unaffected.
#endif

HALF3 CalcReflectionVectorNormalized( HALF3 normal, HALF3 eyeVector )
{
	// FIXME: might be better of normalizing with a normalizing cube map and
	// get rid of the dot( normal, normal )
	// compute reflection vector r = 2 * ((n dot v)/(n dot n)) n - v
	return 2.0 * ( dot( normal, eyeVector ) / dot( normal, normal ) ) * normal - eyeVector;
}

HALF3 CalcReflectionVectorUnnormalized( HALF3 normal, HALF3 eyeVector )
{
	// FIXME: might be better of normalizing with a normalizing cube map and
	// get rid of the dot( normal, normal )
	// compute reflection vector r = 2 * ((n dot v)/(n dot n)) n - v
	//  multiply all values through by N.N.  uniformly scaling reflection vector won't affect result
	//  since it is used in a cubemap lookup
	return (2.0*(dot( normal, eyeVector ))*normal) - (dot( normal, normal )*eyeVector);
}

float3 HuePreservingColorClamp( float3 c )
{
	// Get the max of all of the color components and a specified maximum amount
	float maximum = max( max( c.x, c.y ), max( c.z, 1.0f ) );

	return (c / maximum);
}

HALF3 HuePreservingColorClamp( HALF3 c, HALF maxVal )
{
	// Get the max of all of the color components and a specified maximum amount
	float maximum = max( max( c.x, c.y ), max( c.z, maxVal ) );
	return (c * ( maxVal / maximum ) );
}

#if (AA_CLAMP==1)
HALF2 ComputeLightmapCoordinates( HALF4 Lightmap1and2Coord, HALF2 Lightmap3Coord ) 
{
    HALF2 result = saturate(Lightmap1and2Coord.xy) * Lightmap1and2Coord.wz * 0.99;
    result += Lightmap3Coord;
    return result;
}

void ComputeBumpedLightmapCoordinates( HALF4 Lightmap1and2Coord, HALF2 Lightmap3Coord,
									  out HALF2 bumpCoord1,
									  out HALF2 bumpCoord2,
									  out HALF2 bumpCoord3 ) 
{
    HALF2 result = saturate(Lightmap1and2Coord.xy) * Lightmap1and2Coord.wz * 0.99;
    result += Lightmap3Coord;
    bumpCoord1 = result + HALF2(Lightmap1and2Coord.z, 0);
    bumpCoord2 = result + 2*HALF2(Lightmap1and2Coord.z, 0);
    bumpCoord3 = result + 3*HALF2(Lightmap1and2Coord.z, 0);
}
#else
HALF2 ComputeLightmapCoordinates( HALF4 Lightmap1and2Coord, HALF2 Lightmap3Coord ) 
{
    return Lightmap1and2Coord.xy;
}

void ComputeBumpedLightmapCoordinates( HALF4 Lightmap1and2Coord, HALF2 Lightmap3Coord,
									  out HALF2 bumpCoord1,
									  out HALF2 bumpCoord2,
									  out HALF2 bumpCoord3 ) 
{
    bumpCoord1 = Lightmap1and2Coord.xy;
    bumpCoord2 = Lightmap1and2Coord.wz; // reversed order!!!
    bumpCoord3 = Lightmap3Coord.xy;
}
#endif

// Versions of matrix multiply functions which force HLSL compiler to explictly use DOTs, 
// not giving it the option of using MAD expansion.  In a perfect world, the compiler would
// always pick the best strategy, and these shouldn't be needed.. but.. well.. umm..
//
// lorenmcq

float3 mul3x3(float3 v, float3x3 m)
{
#if !defined( _X360 )
    return float3(dot(v, transpose(m)[0]), dot(v, transpose(m)[1]), dot(v, transpose(m)[2]));
#else
	// xbox360 fxc.exe (new back end) borks with transposes, generates bad code
	return mul( v, m );
#endif
}

float3 mul4x3(float4 v, float4x3 m)
{
#if !defined( _X360 )
	return float3(dot(v, transpose(m)[0]), dot(v, transpose(m)[1]), dot(v, transpose(m)[2]));
#else
	// xbox360 fxc.exe (new back end) borks with transposes, generates bad code
	return mul( v, m );
#endif
}

float3 DecompressHDR( float4 input )
{
	return input.rgb * input.a * MAX_HDR_OVERBRIGHT;
}

float4 CompressHDR( float3 input )
{
	// FIXME: want to use min so that we clamp to white, but what happens if we 
	// have an albedo component that's less than 1/MAX_HDR_OVERBRIGHT?
	//	float fMax = max( max( color.r, color.g ), color.b );
	float4 output;
	float fMax = min( min( input.r, input.g ), input.b );
	if( fMax > 1.0f )
	{
		float oofMax = 1.0f / fMax;
		output.rgb = oofMax * input.rgb;
		output.a = min( fMax / MAX_HDR_OVERBRIGHT, 1.0f );
	}
	else
	{
		output.rgb = input.rgb;
		output.a = 0.0f;
	}
	return output;
}


float3 LinearToGamma( const float3 f3linear )
{
	return pow( f3linear, 1.0f / 2.2f );
}

float4 LinearToGamma( const float4 f4linear )
{
	return float4( pow( f4linear.xyz, 1.0f / 2.2f ), f4linear.w );
}

float LinearToGamma( const float f1linear )
{
	return pow( f1linear, 1.0f / 2.2f );
}

float3 GammaToLinear( const float3 gamma )
{
	return pow( gamma, 2.2f );
}

float4 GammaToLinear( const float4 gamma )
{
	return float4( pow( gamma.xyz, 2.2f ), gamma.w );
}

float GammaToLinear( const float gamma )
{
	return pow( gamma, 2.2f );
}

// These two functions use the actual sRGB math
float SrgbGammaToLinear( float flSrgbGammaValue )
{
	float x = saturate( flSrgbGammaValue );
	return ( x <= 0.04045f ) ? ( x / 12.92f ) : ( pow( ( x + 0.055f ) / 1.055f, 2.4f ) );
}

float SrgbLinearToGamma( float flLinearValue )
{
	float x = saturate( flLinearValue );
	return ( x <= 0.0031308f ) ? ( x * 12.92f ) : ( 1.055f * pow( x, ( 1.0f / 2.4f ) ) ) - 0.055f;
}

// These twofunctions use the XBox 360's exact piecewise linear algorithm
float X360GammaToLinear( float fl360GammaValue )
{
	float flLinearValue;

	fl360GammaValue = saturate( fl360GammaValue );
	if ( fl360GammaValue < ( 96.0f / 255.0f ) )
	{
		if ( fl360GammaValue < ( 64.0f / 255.0f ) )
		{
			flLinearValue = fl360GammaValue * 255.0f;
		}
		else
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 2.0f ) - 64.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 512.0f ) );
		}
	}
	else
	{
		if( fl360GammaValue < ( 192.0f / 255.0f ) )
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 4.0f ) - 256.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 256.0f ) );
		}
		else
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 8.0f ) - 1024.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 128.0f ) );
		}
	}

	flLinearValue *= 1.0f / 1023.0f;

	flLinearValue = saturate( flLinearValue );
	return flLinearValue;
}

float X360LinearToGamma( float flLinearValue )
{
	float fl360GammaValue;

	flLinearValue = saturate( flLinearValue );
	if ( flLinearValue < ( 128.0f / 1023.0f ) )
	{
		if ( flLinearValue < ( 64.0f / 1023.0f ) )
		{
			fl360GammaValue = flLinearValue * ( 1023.0f * ( 1.0f / 255.0f ) );
		}
		else
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f / 2.0f ) * ( 1.0f / 255.0f ) ) + ( 32.0f / 255.0f );
		}
	}
	else
	{
		if ( flLinearValue < ( 512.0f / 1023.0f ) )
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f / 4.0f ) * ( 1.0f / 255.0f ) ) + ( 64.0f / 255.0f );
		}
		else
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f /8.0f ) * ( 1.0f / 255.0f ) ) + ( 128.0f /255.0f ); // 1.0 -> 1.0034313725490196078431372549016
			if ( fl360GammaValue > 1.0f )
			{
				fl360GammaValue = 1.0f;
			}
		}
	}

	fl360GammaValue = saturate( fl360GammaValue );
	return fl360GammaValue;
}

float SrgbGammaTo360Gamma( float flSrgbGammaValue )
{
	float flLinearValue = SrgbGammaToLinear( flSrgbGammaValue );
	float fl360GammaValue = X360LinearToGamma( flLinearValue );
	return fl360GammaValue;
}

float3 Vec3WorldToTangent( float3 iWorldVector, float3 iWorldNormal, float3 iWorldTangent, float3 iWorldBinormal )
{
	float3 vTangentVector;
	vTangentVector.x = dot( iWorldVector.xyz, iWorldTangent.xyz );
	vTangentVector.y = dot( iWorldVector.xyz, iWorldBinormal.xyz );
	vTangentVector.z = dot( iWorldVector.xyz, iWorldNormal.xyz );
	return vTangentVector.xyz; // Return without normalizing
}

float3 Vec3WorldToTangentNormalized( float3 iWorldVector, float3 iWorldNormal, float3 iWorldTangent, float3 iWorldBinormal )
{
	return normalize( Vec3WorldToTangent( iWorldVector, iWorldNormal, iWorldTangent, iWorldBinormal ) );
}

float3 Vec3TangentToWorld( float3 iTangentVector, float3 iWorldNormal, float3 iWorldTangent, float3 iWorldBinormal )
{
	float3 vWorldVector;
	vWorldVector.xyz = iTangentVector.x * iWorldTangent.xyz;
	vWorldVector.xyz += iTangentVector.y * iWorldBinormal.xyz;
	vWorldVector.xyz += iTangentVector.z * iWorldNormal.xyz;
	return vWorldVector.xyz; // Return without normalizing
}

float3 Vec3TangentToWorldNormalized( float3 iTangentVector, float3 iWorldNormal, float3 iWorldTangent, float3 iWorldBinormal )
{
	return normalize( Vec3TangentToWorld( iTangentVector, iWorldNormal, iWorldTangent, iWorldBinormal ) );
}

// Returns 1.0f for no fog, 0.0f for fully fogged
float CalcRadialFog_FixedFunction( float3 vWorldPos, float3 vEyePos, float flFogMaxDensity, float flFogEndOverRange, float flFogOORange )
{
	float flDistance = distance( vEyePos.xyz, vWorldPos.xyz );
	return max( flFogMaxDensity, ( -flDistance * flFogOORange ) + flFogEndOverRange );
}

// Returns 0.0f for no fog, 1.0f for fully fogged which is opposite of what fixed function fog expects so that we don't have to do a "1-x" in the pixel shader.
float CalcRadialFog_NonFixedFunction( float3 vWorldPos, float3 vEyePos, float flFogMaxDensity, float flFogEndOverRange, float flFogOORange )
{
	float flDistance = distance( vEyePos.xyz, vWorldPos.xyz );
	return min( flFogMaxDensity, saturate( ( flDistance * flFogOORange ) - flFogEndOverRange ) );
}

#endif //#ifndef COMMON_FXC_H_
