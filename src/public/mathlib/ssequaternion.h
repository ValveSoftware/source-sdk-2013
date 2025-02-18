//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: - defines SIMD "structure of arrays" classes and functions.
//
//===========================================================================//
#ifndef SSEQUATMATH_H
#define SSEQUATMATH_H

#ifdef _WIN32
#pragma once
#endif


#include "mathlib/ssemath.h"

// Use this #define to allow SSE versions of Quaternion math
// to exist on PC.
// On PC, certain horizontal vector operations are not supported.
// This causes the SSE implementation of quaternion math to mix the
// vector and scalar floating point units, which is extremely 
// performance negative if you don't compile to native SSE2 (which 
// we don't as of Sept 1, 2007). So, it's best not to allow these
// functions to exist at all. It's not good enough to simply replace
// the contents of the functions with scalar math, because each call
// to LoadAligned and StoreAligned will result in an unnecssary copy
// of the quaternion, and several moves to and from the XMM registers.
//
// Basically, the problem you run into is that for efficient SIMD code,
// you need to load the quaternions and vectors into SIMD registers and
// keep them there as long as possible while doing only SIMD math,
// whereas for efficient scalar code, each time you copy onto or ever
// use a fltx4, it hoses your pipeline. So the difference has to be
// in the management of temporary variables in the calling function,
// not inside the math functions.
//
// If you compile assuming the presence of SSE2, the MSVC will abandon
// the traditional x87 FPU operations altogether and make everything use
// the SSE2 registers, which lessens this problem a little.

// permitted only on 360, as we've done careful tuning on its Altivec math:
#ifdef _X360
#define ALLOW_SIMD_QUATERNION_MATH 1  // not on PC!
#endif



//---------------------------------------------------------------------
// Load/store quaternions
//---------------------------------------------------------------------
#ifndef _X360
#if ALLOW_SIMD_QUATERNION_MATH
// Using STDC or SSE
FORCEINLINE fltx4 LoadAlignedSIMD( const QuaternionAligned & pSIMD )
{
	fltx4 retval = LoadAlignedSIMD( pSIMD.Base() );
	return retval;
}

FORCEINLINE fltx4 LoadAlignedSIMD( const QuaternionAligned * RESTRICT pSIMD )
{
	fltx4 retval = LoadAlignedSIMD( pSIMD );
	return retval;
}

FORCEINLINE void StoreAlignedSIMD( QuaternionAligned * RESTRICT pSIMD, const fltx4 & a )
{
	StoreAlignedSIMD( pSIMD->Base(), a );
}
#endif
#else

// for the transitional class -- load a QuaternionAligned
FORCEINLINE fltx4 LoadAlignedSIMD( const QuaternionAligned & pSIMD )
{
	fltx4 retval = XMLoadVector4A( pSIMD.Base() );
	return retval;
}

FORCEINLINE fltx4 LoadAlignedSIMD( const QuaternionAligned * RESTRICT pSIMD )
{
	fltx4 retval = XMLoadVector4A( pSIMD );
	return retval;
}

FORCEINLINE void StoreAlignedSIMD( QuaternionAligned * RESTRICT pSIMD, const fltx4 & a )
{
	XMStoreVector4A( pSIMD->Base(), a );
}

#endif


#if ALLOW_SIMD_QUATERNION_MATH
//---------------------------------------------------------------------
// Make sure quaternions are within 180 degrees of one another, if not, reverse q
//---------------------------------------------------------------------
FORCEINLINE fltx4 QuaternionAlignSIMD( const fltx4 &p, const fltx4 &q )
{
	// decide if one of the quaternions is backwards
	fltx4 a = SubSIMD( p, q );
	fltx4 b = AddSIMD( p, q );
	a = Dot4SIMD( a, a );
	b = Dot4SIMD( b, b );
	fltx4 cmp = CmpGtSIMD( a, b );
	fltx4 result = MaskedAssign( cmp, NegSIMD(q), q );
	return result;
}

//---------------------------------------------------------------------
// Normalize Quaternion
//---------------------------------------------------------------------
#if USE_STDC_FOR_SIMD

FORCEINLINE fltx4 QuaternionNormalizeSIMD( const fltx4 &q )
{
	fltx4 radius, result;
	radius = Dot4SIMD( q, q );

	if ( SubFloat( radius, 0 ) ) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
	{
		float iradius = 1.0f / sqrt( SubFloat( radius, 0 ) );
		result = ReplicateX4( iradius );
		result = MulSIMD( result, q );
		return result;
	}
	return q;
}

#else

// SSE + X360 implementation
FORCEINLINE fltx4 QuaternionNormalizeSIMD( const fltx4 &q )
{
	fltx4 radius, result, mask;
	radius = Dot4SIMD( q, q );
	mask = CmpEqSIMD( radius, Four_Zeros ); // all ones iff radius = 0
	result = ReciprocalSqrtSIMD( radius );
	result = MulSIMD( result, q );
	return MaskedAssign( mask, q, result );	// if radius was 0, just return q
}

#endif


//---------------------------------------------------------------------
// 0.0 returns p, 1.0 return q.
//---------------------------------------------------------------------
FORCEINLINE fltx4 QuaternionBlendNoAlignSIMD( const fltx4 &p, const fltx4 &q, float t )
{
	fltx4 sclp, sclq, result;
	sclq = ReplicateX4( t );
	sclp = SubSIMD( Four_Ones, sclq );
	result = MulSIMD( sclp, p );
	result = MaddSIMD( sclq, q, result );
	return QuaternionNormalizeSIMD( result );
}


//---------------------------------------------------------------------
// Blend Quaternions
//---------------------------------------------------------------------
FORCEINLINE fltx4 QuaternionBlendSIMD( const fltx4 &p, const fltx4 &q, float t )
{
	// decide if one of the quaternions is backwards
	fltx4 q2, result;
	q2 = QuaternionAlignSIMD( p, q );
	result = QuaternionBlendNoAlignSIMD( p, q2, t );
	return result;
}


//---------------------------------------------------------------------
// Multiply Quaternions
//---------------------------------------------------------------------
#ifndef _X360

// SSE and STDC
FORCEINLINE fltx4 QuaternionMultSIMD( const fltx4 &p, const fltx4 &q )
{
	// decide if one of the quaternions is backwards
	fltx4 q2, result;
	q2 = QuaternionAlignSIMD( p, q );
	SubFloat( result, 0 ) =  SubFloat( p, 0 ) * SubFloat( q2, 3 ) + SubFloat( p, 1 ) * SubFloat( q2, 2 ) - SubFloat( p, 2 ) * SubFloat( q2, 1 ) + SubFloat( p, 3 ) * SubFloat( q2, 0 );
	SubFloat( result, 1 ) = -SubFloat( p, 0 ) * SubFloat( q2, 2 ) + SubFloat( p, 1 ) * SubFloat( q2, 3 ) + SubFloat( p, 2 ) * SubFloat( q2, 0 ) + SubFloat( p, 3 ) * SubFloat( q2, 1 );
	SubFloat( result, 2 ) =  SubFloat( p, 0 ) * SubFloat( q2, 1 ) - SubFloat( p, 1 ) * SubFloat( q2, 0 ) + SubFloat( p, 2 ) * SubFloat( q2, 3 ) + SubFloat( p, 3 ) * SubFloat( q2, 2 );
	SubFloat( result, 3 ) = -SubFloat( p, 0 ) * SubFloat( q2, 0 ) - SubFloat( p, 1 ) * SubFloat( q2, 1 ) - SubFloat( p, 2 ) * SubFloat( q2, 2 ) + SubFloat( p, 3 ) * SubFloat( q2, 3 );
	return result;
}

#else 

// X360
extern const fltx4 g_QuatMultRowSign[4];
FORCEINLINE fltx4 QuaternionMultSIMD( const fltx4 &p, const fltx4 &q )
{
	fltx4 q2, row, result;
	q2 = QuaternionAlignSIMD( p, q );

	row = XMVectorSwizzle( q2, 3, 2, 1, 0 );
	row = MulSIMD( row, g_QuatMultRowSign[0] );
	result = Dot4SIMD( row, p );

	row = XMVectorSwizzle( q2, 2, 3, 0, 1 );
	row = MulSIMD( row, g_QuatMultRowSign[1] );
	row = Dot4SIMD( row, p );
	result = __vrlimi( result, row, 4, 0 );
	
	row = XMVectorSwizzle( q2, 1, 0, 3, 2 );
	row = MulSIMD( row, g_QuatMultRowSign[2] );
	row = Dot4SIMD( row, p );
	result = __vrlimi( result, row, 2, 0 );
	
	row = MulSIMD( q2, g_QuatMultRowSign[3] );
	row = Dot4SIMD( row, p );
	result = __vrlimi( result, row, 1, 0 );
	return result;
}

#endif


//---------------------------------------------------------------------
// Quaternion scale
//---------------------------------------------------------------------
#ifndef _X360

// SSE and STDC
FORCEINLINE fltx4 QuaternionScaleSIMD( const fltx4 &p, float t )
{
	float r;
	fltx4 q;

	// FIXME: nick, this isn't overly sensitive to accuracy, and it may be faster to 
	// use the cos part (w) of the quaternion (sin(omega)*N,cos(omega)) to figure the new scale.
	float sinom = sqrt( SubFloat( p, 0 ) * SubFloat( p, 0 ) + SubFloat( p, 1 ) * SubFloat( p, 1 ) + SubFloat( p, 2 ) * SubFloat( p, 2 ) );
	sinom = min( sinom, 1.f );

	float sinsom = sin( asin( sinom ) * t );

	t = sinsom / (sinom + FLT_EPSILON);
	SubFloat( q, 0 ) = t * SubFloat( p, 0 );
	SubFloat( q, 1 ) = t * SubFloat( p, 1 );
	SubFloat( q, 2 ) = t * SubFloat( p, 2 );

	// rescale rotation
	r = 1.0f - sinsom * sinsom;

	// Assert( r >= 0 );
	if (r < 0.0f) 
		r = 0.0f;
	r = sqrt( r );

	// keep sign of rotation
	SubFloat( q, 3 ) = fsel( SubFloat( p, 3 ), r, -r );
	return q;
}

#else

// X360
FORCEINLINE fltx4 QuaternionScaleSIMD( const fltx4 &p, float t )
{
	fltx4 sinom = Dot3SIMD( p, p );
	sinom = SqrtSIMD( sinom );
	sinom = MinSIMD( sinom, Four_Ones );
	fltx4 sinsom = ArcSinSIMD( sinom );
	fltx4 t4 = ReplicateX4( t );
	sinsom = MulSIMD( sinsom, t4 );
	sinsom = SinSIMD( sinsom );
	sinom = AddSIMD( sinom, Four_Epsilons );
	sinom = ReciprocalSIMD( sinom );
	t4 = MulSIMD( sinsom, sinom );
	fltx4 result = MulSIMD( p, t4 );

	// rescale rotation
	sinsom = MulSIMD( sinsom, sinsom );
	fltx4 r = SubSIMD( Four_Ones, sinsom );
	r = MaxSIMD( r, Four_Zeros );
	r = SqrtSIMD( r );

	// keep sign of rotation
	fltx4 cmp = CmpGeSIMD( p, Four_Zeros );
	r = MaskedAssign( cmp, r, NegSIMD( r ) );

	result = __vrlimi(result, r, 1, 0);
	return result;
}

#endif


//-----------------------------------------------------------------------------
// Quaternion sphereical linear interpolation
//-----------------------------------------------------------------------------
#ifndef _X360

// SSE and STDC
FORCEINLINE fltx4 QuaternionSlerpNoAlignSIMD( const fltx4 &p, const fltx4 &q, float t )
{
	float omega, cosom, sinom, sclp, sclq;

	fltx4 result;

	// 0.0 returns p, 1.0 return q.
	cosom = SubFloat( p, 0 ) * SubFloat( q, 0 ) + SubFloat( p, 1 ) * SubFloat( q, 1 ) + 
		SubFloat( p, 2 ) * SubFloat( q, 2 ) + SubFloat( p, 3 ) * SubFloat( q, 3 );

	if ( (1.0f + cosom ) > 0.000001f ) 
	{
		if ( (1.0f - cosom ) > 0.000001f ) 
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( (1.0f - t)*omega) / sinom;
			sclq = sin( t*omega ) / sinom;
		}
		else 
		{
			// TODO: add short circuit for cosom == 1.0f?
			sclp = 1.0f - t;
			sclq = t;
		}
		SubFloat( result, 0 ) = sclp * SubFloat( p, 0 ) + sclq * SubFloat( q, 0 );
		SubFloat( result, 1 ) = sclp * SubFloat( p, 1 ) + sclq * SubFloat( q, 1 );
		SubFloat( result, 2 ) = sclp * SubFloat( p, 2 ) + sclq * SubFloat( q, 2 );
		SubFloat( result, 3 ) = sclp * SubFloat( p, 3 ) + sclq * SubFloat( q, 3 );
	}
	else 
	{
		SubFloat( result, 0 ) = -SubFloat( q, 1 );
		SubFloat( result, 1 ) =  SubFloat( q, 0 );
		SubFloat( result, 2 ) = -SubFloat( q, 3 );
		SubFloat( result, 3 ) =  SubFloat( q, 2 );
		sclp = sin( (1.0f - t) * (0.5f * M_PI));
		sclq = sin( t * (0.5f * M_PI));
		SubFloat( result, 0 ) = sclp * SubFloat( p, 0 ) + sclq * SubFloat( result, 0 );
		SubFloat( result, 1 ) = sclp * SubFloat( p, 1 ) + sclq * SubFloat( result, 1 );
		SubFloat( result, 2 ) = sclp * SubFloat( p, 2 ) + sclq * SubFloat( result, 2 );
	}

	return result;
}

#else

// X360
FORCEINLINE fltx4 QuaternionSlerpNoAlignSIMD( const fltx4 &p, const fltx4 &q, float t )
{
	return XMQuaternionSlerp( p, q, t );
}

#endif


FORCEINLINE fltx4 QuaternionSlerpSIMD( const fltx4 &p, const fltx4 &q, float t )
{
	fltx4 q2, result;
	q2 = QuaternionAlignSIMD( p, q );
	result = QuaternionSlerpNoAlignSIMD( p, q2, t );
	return result;
}


#endif // ALLOW_SIMD_QUATERNION_MATH

#endif // SSEQUATMATH_H

