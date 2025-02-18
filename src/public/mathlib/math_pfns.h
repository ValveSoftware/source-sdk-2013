//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _MATH_PFNS_H_
#define _MATH_PFNS_H_

#if defined( _X360 )
#include <xboxmath.h>
#endif

#if !defined( _X360 )

// These globals are initialized by mathlib and redirected based on available fpu features
extern float (*pfSqrt)(float x);
extern float (*pfRSqrt)(float x);
extern float (*pfRSqrtFast)(float x);
extern void  (*pfFastSinCos)(float x, float *s, float *c);
extern float (*pfFastCos)(float x);

// The following are not declared as macros because they are often used in limiting situations,
// and sometimes the compiler simply refuses to inline them for some reason
#define FastSqrt(x)			(*pfSqrt)(x)
#define	FastRSqrt(x)		(*pfRSqrt)(x)
#define FastRSqrtFast(x)    (*pfRSqrtFast)(x)
#define FastSinCos(x,s,c)   (*pfFastSinCos)(x,s,c)
#define FastCos(x)			(*pfFastCos)(x)

#if defined(__i386__) || defined(_M_IX86)
// On x86, the inline FPU or SSE sqrt instruction is faster than
// the overhead of setting up a function call and saving/restoring
// the FPU or SSE register state and can be scheduled better, too.
#undef FastSqrt
#define FastSqrt(x)			::sqrtf(x)
#endif

#endif // !_X360

#if defined( _X360 )

FORCEINLINE float _VMX_Sqrt( float x )
{
	return __fsqrts( x );
}

FORCEINLINE float _VMX_RSqrt( float x )
{
	float rroot = __frsqrte( x );

	// Single iteration NewtonRaphson on reciprocal square root estimate
	return (0.5f * rroot) * (3.0f - (x * rroot) * rroot);
}

FORCEINLINE float _VMX_RSqrtFast( float x )
{
	return __frsqrte( x );
}

FORCEINLINE void _VMX_SinCos( float a, float *pS, float *pC )
{
	XMScalarSinCos( pS, pC, a );
}

FORCEINLINE float _VMX_Cos( float a )
{
	return XMScalarCos( a );
}

// the 360 has fixed hw and calls directly
#define FastSqrt(x)			_VMX_Sqrt(x)
#define	FastRSqrt(x)		_VMX_RSqrt(x)
#define FastRSqrtFast(x)	_VMX_RSqrtFast(x)
#define FastSinCos(x,s,c)	_VMX_SinCos(x,s,c)
#define FastCos(x)			_VMX_Cos(x)

#endif // _X360

#endif // _MATH_PFNS_H_
