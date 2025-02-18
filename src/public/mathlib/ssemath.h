//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: - defines SIMD "structure of arrays" classes and functions.
//
//===========================================================================//
#ifndef SSEMATH_H
#define SSEMATH_H

#if defined( _X360 )
#include <xboxmath.h>
#else
#include <xmmintrin.h>
#endif

#include <mathlib/vector.h>
#include <mathlib/mathlib.h>

#if defined(GNUC)
#define USE_STDC_FOR_SIMD 0
#else
#define USE_STDC_FOR_SIMD 0
#endif

#if (!defined(_X360) && (USE_STDC_FOR_SIMD == 0))
#define _SSE1 1
#endif

// I thought about defining a class/union for the SIMD packed floats instead of using fltx4,
// but decided against it because (a) the nature of SIMD code which includes comparisons is to blur
// the relationship between packed floats and packed integer types and (b) not sure that the
// compiler would handle generating good code for the intrinsics.

#if USE_STDC_FOR_SIMD

typedef union
{
	float  m128_f32[4];
	uint32 m128_u32[4];
} fltx4;

typedef fltx4 i32x4;
typedef fltx4 u32x4;

typedef fltx4 bi32x4;

#elif ( defined( _X360 ) )

typedef union
{
	// This union allows float/int access (which generally shouldn't be done in inner loops)
	__vector4	vmx;
	float		m128_f32[4];
	uint32		m128_u32[4];
} fltx4_union;

typedef __vector4 fltx4;
typedef __vector4 i32x4; // a VMX register; just a way of making it explicit that we're doing integer ops.
typedef __vector4 u32x4; // a VMX register; just a way of making it explicit that we're doing unsigned integer ops.

#else

typedef __m128 fltx4;
typedef __m128 i32x4;
typedef __m128 u32x4;
typedef __m128 bi32x4;
typedef __m128i shortx8;

#endif

// The FLTX4 type is a fltx4 used as a parameter to a function.
// On the 360, the best way to do this is pass-by-copy on the registers.
// On the PC, the best way is to pass by const reference. 
// The compiler will sometimes, but not always, replace a pass-by-const-ref
// with a pass-in-reg on the 360; to avoid this confusion, you can
// explicitly use a FLTX4 as the parameter type.
#ifdef _X360
typedef __vector4 FLTX4;
#else
typedef const fltx4 & FLTX4;
#endif

// A 16-byte aligned int32 datastructure
// (for use when writing out fltx4's as SIGNED
// ints).
struct ALIGN16 intx4
{
	int32 m_i32[4];

	inline int & operator[](int which) 
	{
		return m_i32[which];
	}

	inline const int & operator[](int which) const
	{
		return m_i32[which];
	}

	inline int32 *Base() {
		return m_i32;
	}

	inline const int32 *Base() const
	{
		return m_i32;
	}

	inline const bool operator==(const intx4 &other) const
	{
		return m_i32[0] == other.m_i32[0] &&
			m_i32[1] == other.m_i32[1] &&
			m_i32[2] == other.m_i32[2] &&
			m_i32[3] == other.m_i32[3] 	;
	}
} ALIGN16_POST;


#if defined( _DEBUG ) && defined( _X360 )
FORCEINLINE void TestVPUFlags()
{
	// Check that the VPU is in the appropriate (Java-compliant) mode (see 3.2.1 in altivec_pem.pdf on xds.xbox.com)
	__vector4 a;
	__asm
	{
		mfvscr	a;
	}
	unsigned int * flags		= (unsigned int *)&a;
	unsigned int   controlWord	= flags[3];
	Assert(controlWord == 0);
}
#else  // _DEBUG
FORCEINLINE void TestVPUFlags() {}
#endif // _DEBUG


// useful constants in SIMD packed float format:
// (note: some of these aren't stored on the 360, 
// but are manufactured directly in one or two 
// instructions, saving a load and possible L2
// miss.)
#ifndef _X360
extern const fltx4 Four_Zeros;									// 0 0 0 0
extern const fltx4 Four_Ones;									// 1 1 1 1
extern const fltx4 Four_Twos;									// 2 2 2 2
extern const fltx4 Four_Threes;									// 3 3 3 3
extern const fltx4 Four_Fours;									// guess.
extern const fltx4 Four_Point225s;								// .225 .225 .225 .225
extern const fltx4 Four_PointFives;								// .5 .5 .5 .5
extern const fltx4 Four_Epsilons;								// FLT_EPSILON FLT_EPSILON FLT_EPSILON FLT_EPSILON
extern const fltx4 Four_2ToThe21s;								// (1<<21)..
extern const fltx4 Four_2ToThe22s;								// (1<<22)..
extern const fltx4 Four_2ToThe23s;								// (1<<23)..
extern const fltx4 Four_2ToThe24s;								// (1<<24)..
extern const fltx4 Four_Origin;									// 0 0 0 1 (origin point, like vr0 on the PS2)
extern const fltx4 Four_NegativeOnes;							// -1 -1 -1 -1 
#else
#define			   Four_Zeros XMVectorZero()					// 0 0 0 0
#define			   Four_Ones XMVectorSplatOne()					// 1 1 1 1
extern const fltx4 Four_Twos;									// 2 2 2 2
extern const fltx4 Four_Threes;									// 3 3 3 3
extern const fltx4 Four_Fours;									// guess.
extern const fltx4 Four_Point225s;								// .225 .225 .225 .225
extern const fltx4 Four_PointFives;								// .5 .5 .5 .5
extern const fltx4 Four_Epsilons;								// FLT_EPSILON FLT_EPSILON FLT_EPSILON FLT_EPSILON
extern const fltx4 Four_2ToThe21s;								// (1<<21)..
extern const fltx4 Four_2ToThe22s;								// (1<<22)..
extern const fltx4 Four_2ToThe23s;								// (1<<23)..
extern const fltx4 Four_2ToThe24s;								// (1<<24)..
extern const fltx4 Four_Origin;									// 0 0 0 1 (origin point, like vr0 on the PS2)
extern const fltx4 Four_NegativeOnes;							// -1 -1 -1 -1 
#endif
extern const fltx4 Four_FLT_MAX;								// FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX
extern const fltx4 Four_Negative_FLT_MAX;						// -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX
extern const fltx4 g_SIMD_0123;									// 0 1 2 3 as float

// external aligned integer constants
extern const ALIGN16 uint32 g_SIMD_clear_signmask[] ALIGN16_POST;			// 0x7fffffff x 4
extern const ALIGN16 uint32 g_SIMD_signmask[] ALIGN16_POST;				// 0x80000000 x 4
extern const ALIGN16 uint32 g_SIMD_lsbmask[] ALIGN16_POST;				// 0xfffffffe x 4
extern const ALIGN16 uint32 g_SIMD_clear_wmask[] ALIGN16_POST;			// -1 -1 -1 0
extern const ALIGN16 uint32 g_SIMD_ComponentMask[4][4] ALIGN16_POST;		// [0xFFFFFFFF 0 0 0], [0 0xFFFFFFFF 0 0], [0 0 0xFFFFFFFF 0], [0 0 0 0xFFFFFFFF]
extern const ALIGN16 uint32 g_SIMD_AllOnesMask[] ALIGN16_POST;			// ~0,~0,~0,~0
extern const ALIGN16 uint32 g_SIMD_Low16BitsMask[] ALIGN16_POST;			// 0xffff x 4

// this mask is used for skipping the tail of things. If you have N elements in an array, and wish
// to mask out the tail, g_SIMD_SkipTailMask[N & 3] what you want to use for the last iteration.
extern const uint32 ALIGN16 g_SIMD_SkipTailMask[4][4] ALIGN16_POST;

// Define prefetch macros.
// The characteristics of cache and prefetch are completely 
// different between the different platforms, so you DO NOT
// want to just define one macro that maps to every platform
// intrinsic under the hood -- you need to prefetch at different
// intervals between x86 and PPC, for example, and that is
// a higher level code change. 
// On the other hand, I'm tired of typing #ifdef _X360
// all over the place, so this is just a nop on Intel, PS3.
#ifdef _X360
#define PREFETCH360(address, offset) __dcbt(offset,address)
#else
#define PREFETCH360(x,y) // nothing
#endif

#if USE_STDC_FOR_SIMD

//---------------------------------------------------------------------
// Standard C (fallback/Linux) implementation (only there for compat - slow)
//---------------------------------------------------------------------

FORCEINLINE float SubFloat( const fltx4 & a, int idx )
{
	return a.m128_f32[ idx ];
}

FORCEINLINE float & SubFloat( fltx4 & a, int idx )
{
	return a.m128_f32[idx];
}

FORCEINLINE uint32 SubInt( const fltx4 & a, int idx )
{
	return a.m128_u32[idx];
}

FORCEINLINE uint32 & SubInt( fltx4 & a, int idx )
{
	return a.m128_u32[idx];
}

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE fltx4 LoadZeroSIMD( void )
{
	return Four_Zeros;
}

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE fltx4 LoadOneSIMD( void )
{
	return Four_Ones;
}

FORCEINLINE fltx4 SplatXSIMD( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 0 );
	SubFloat( retVal, 1 ) = SubFloat( a, 0 );
	SubFloat( retVal, 2 ) = SubFloat( a, 0 );
	SubFloat( retVal, 3 ) = SubFloat( a, 0 );
	return retVal;
}

FORCEINLINE fltx4 SplatYSIMD( fltx4 a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 1 );
	SubFloat( retVal, 1 ) = SubFloat( a, 1 );
	SubFloat( retVal, 2 ) = SubFloat( a, 1 );
	SubFloat( retVal, 3 ) = SubFloat( a, 1 );
	return retVal;
}

FORCEINLINE fltx4 SplatZSIMD( fltx4 a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 2 );
	SubFloat( retVal, 1 ) = SubFloat( a, 2 );
	SubFloat( retVal, 2 ) = SubFloat( a, 2 );
	SubFloat( retVal, 3 ) = SubFloat( a, 2 );
	return retVal;
}

FORCEINLINE fltx4 SplatWSIMD( fltx4 a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 3 );
	SubFloat( retVal, 1 ) = SubFloat( a, 3 );
	SubFloat( retVal, 2 ) = SubFloat( a, 3 );
	SubFloat( retVal, 3 ) = SubFloat( a, 3 );
	return retVal;
}

FORCEINLINE fltx4 SetXSIMD( const fltx4& a, const fltx4& x )
{
	fltx4 result = a;
	SubFloat( result, 0 ) = SubFloat( x, 0 );
	return result;
}

FORCEINLINE fltx4 SetYSIMD( const fltx4& a, const fltx4& y )
{
	fltx4 result = a;
	SubFloat( result, 1 ) = SubFloat( y, 1 );
	return result;
}

FORCEINLINE fltx4 SetZSIMD( const fltx4& a, const fltx4& z )
{
	fltx4 result = a;
	SubFloat( result, 2 ) = SubFloat( z, 2 );
	return result;
}

FORCEINLINE fltx4 SetWSIMD( const fltx4& a, const fltx4& w )
{
	fltx4 result = a;
	SubFloat( result, 3 ) = SubFloat( w, 3 );
	return result;
}

FORCEINLINE fltx4 SetComponentSIMD( const fltx4& a, int nComponent, float flValue )
{
	fltx4 result = a;
	SubFloat( result, nComponent ) = flValue;
	return result;
}

// a b c d -> b c d a
FORCEINLINE fltx4 RotateLeft( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 1 );
	SubFloat( retVal, 1 ) = SubFloat( a, 2 );
	SubFloat( retVal, 2 ) = SubFloat( a, 3 );
	SubFloat( retVal, 3 ) = SubFloat( a, 0 );
	return retVal;
}

// a b c d -> c d a b
FORCEINLINE fltx4 RotateLeft2( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = SubFloat( a, 2 );
	SubFloat( retVal, 1 ) = SubFloat( a, 3 );
	SubFloat( retVal, 2 ) = SubFloat( a, 0 );
	SubFloat( retVal, 3 ) = SubFloat( a, 1 );
	return retVal;
}

#define BINOP(op) 														\
	fltx4 retVal;                                          				\
	SubFloat( retVal, 0 ) = ( SubFloat( a, 0 ) op SubFloat( b, 0 ) );	\
	SubFloat( retVal, 1 ) = ( SubFloat( a, 1 ) op SubFloat( b, 1 ) );	\
	SubFloat( retVal, 2 ) = ( SubFloat( a, 2 ) op SubFloat( b, 2 ) );	\
	SubFloat( retVal, 3 ) = ( SubFloat( a, 3 ) op SubFloat( b, 3 ) );	\
    return retVal;

#define IBINOP(op) 														\
	fltx4 retVal;														\
	SubInt( retVal, 0 ) = ( SubInt( a, 0 ) op SubInt ( b, 0 ) );		\
	SubInt( retVal, 1 ) = ( SubInt( a, 1 ) op SubInt ( b, 1 ) );		\
	SubInt( retVal, 2 ) = ( SubInt( a, 2 ) op SubInt ( b, 2 ) );		\
	SubInt( retVal, 3 ) = ( SubInt( a, 3 ) op SubInt ( b, 3 ) );		\
    return retVal;

FORCEINLINE fltx4 AddSIMD( const fltx4 & a, const fltx4 & b )
{
	BINOP(+);
}

FORCEINLINE fltx4 SubSIMD( const fltx4 & a, const fltx4 & b )				// a-b
{
	BINOP(-);
};

FORCEINLINE fltx4 MulSIMD( const fltx4 & a, const fltx4 & b )				// a*b
{
	BINOP(*);
}

FORCEINLINE fltx4 DivSIMD( const fltx4 & a, const fltx4 & b )				// a/b
{
	BINOP(/);
}


FORCEINLINE fltx4 MaddSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// a*b + c
{
	return AddSIMD( MulSIMD(a,b), c );
}

FORCEINLINE fltx4 MsubSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// c - a*b
{
	return SubSIMD( c, MulSIMD(a,b) );
};


FORCEINLINE fltx4 SinSIMD( const fltx4 &radians )
{
	fltx4 result;
	SubFloat( result, 0 ) = sin( SubFloat( radians, 0 ) );
	SubFloat( result, 1 ) = sin( SubFloat( radians, 1 ) );
	SubFloat( result, 2 ) = sin( SubFloat( radians, 2 ) );
	SubFloat( result, 3 ) = sin( SubFloat( radians, 3 ) );
	return result;
}

FORCEINLINE void SinCos3SIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )
{
	SinCos( SubFloat( radians, 0 ), &SubFloat( sine, 0 ), &SubFloat( cosine, 0 ) );
	SinCos( SubFloat( radians, 1 ), &SubFloat( sine, 1 ), &SubFloat( cosine, 1 ) );
	SinCos( SubFloat( radians, 2 ), &SubFloat( sine, 2 ), &SubFloat( cosine, 2 ) );
}

FORCEINLINE void SinCosSIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )
{
	SinCos( SubFloat( radians, 0 ), &SubFloat( sine, 0 ), &SubFloat( cosine, 0 ) );
	SinCos( SubFloat( radians, 1 ), &SubFloat( sine, 1 ), &SubFloat( cosine, 1 ) );
	SinCos( SubFloat( radians, 2 ), &SubFloat( sine, 2 ), &SubFloat( cosine, 2 ) );
	SinCos( SubFloat( radians, 3 ), &SubFloat( sine, 3 ), &SubFloat( cosine, 3 ) );
}

FORCEINLINE fltx4 ArcSinSIMD( const fltx4 &sine )
{
	fltx4 result;
	SubFloat( result, 0 ) = asin( SubFloat( sine, 0 ) );
	SubFloat( result, 1 ) = asin( SubFloat( sine, 1 ) );
	SubFloat( result, 2 ) = asin( SubFloat( sine, 2 ) );
	SubFloat( result, 3 ) = asin( SubFloat( sine, 3 ) );
	return result;
}

FORCEINLINE fltx4 ArcCosSIMD( const fltx4 &cs )
{
	fltx4 result;
	SubFloat( result, 0 ) = acos( SubFloat( cs, 0 ) );
	SubFloat( result, 1 ) = acos( SubFloat( cs, 1 ) );
	SubFloat( result, 2 ) = acos( SubFloat( cs, 2 ) );
	SubFloat( result, 3 ) = acos( SubFloat( cs, 3 ) );
	return result;
}

// tan^1(a/b) .. ie, pass sin in as a and cos in as b
FORCEINLINE fltx4 ArcTan2SIMD( const fltx4 &a, const fltx4 &b )
{
	fltx4 result;
	SubFloat( result, 0 ) = atan2( SubFloat( a, 0 ), SubFloat( b, 0 ) );
	SubFloat( result, 1 ) = atan2( SubFloat( a, 1 ), SubFloat( b, 1 ) );
	SubFloat( result, 2 ) = atan2( SubFloat( a, 2 ), SubFloat( b, 2 ) );
	SubFloat( result, 3 ) = atan2( SubFloat( a, 3 ), SubFloat( b, 3 ) );
	return result;
}

FORCEINLINE fltx4 MaxSIMD( const fltx4 & a, const fltx4 & b )				// max(a,b)
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = max( SubFloat( a, 0 ), SubFloat( b, 0 ) );
	SubFloat( retVal, 1 ) = max( SubFloat( a, 1 ), SubFloat( b, 1 ) );
	SubFloat( retVal, 2 ) = max( SubFloat( a, 2 ), SubFloat( b, 2 ) );
	SubFloat( retVal, 3 ) = max( SubFloat( a, 3 ), SubFloat( b, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 MinSIMD( const fltx4 & a, const fltx4 & b )				// min(a,b)
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = min( SubFloat( a, 0 ), SubFloat( b, 0 ) );
	SubFloat( retVal, 1 ) = min( SubFloat( a, 1 ), SubFloat( b, 1 ) );
	SubFloat( retVal, 2 ) = min( SubFloat( a, 2 ), SubFloat( b, 2 ) );
	SubFloat( retVal, 3 ) = min( SubFloat( a, 3 ), SubFloat( b, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 AndSIMD( const fltx4 & a, const fltx4 & b )				// a & b
{
	IBINOP(&);
}

FORCEINLINE fltx4 AndNotSIMD( const fltx4 & a, const fltx4 & b )			// ~a & b
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ~SubInt( a, 0 ) & SubInt( b, 0 );
	SubInt( retVal, 1 ) = ~SubInt( a, 1 ) & SubInt( b, 1 );
	SubInt( retVal, 2 ) = ~SubInt( a, 2 ) & SubInt( b, 2 );
	SubInt( retVal, 3 ) = ~SubInt( a, 3 ) & SubInt( b, 3 );
	return retVal;
}

FORCEINLINE fltx4 XorSIMD( const fltx4 & a, const fltx4 & b )				// a ^ b
{
	IBINOP(^);
}

FORCEINLINE fltx4 OrSIMD( const fltx4 & a, const fltx4 & b )				// a | b
{
	IBINOP(|);
}

FORCEINLINE fltx4 NegSIMD(const fltx4 &a) // negate: -a
{
	fltx4 retval;
	SubFloat( retval, 0 ) = -SubFloat( a, 0 );
	SubFloat( retval, 1 ) = -SubFloat( a, 1 );
	SubFloat( retval, 2 ) = -SubFloat( a, 2 );
	SubFloat( retval, 3 ) = -SubFloat( a, 3 );

	return retval;
}

FORCEINLINE bool IsAllZeros( const fltx4 & a )								// all floats of a zero?
{
	return	( SubFloat( a, 0 ) == 0.0 ) &&
		( SubFloat( a, 1 ) == 0.0 ) &&
		( SubFloat( a, 2 ) == 0.0 ) &&
		( SubFloat( a, 3 ) == 0.0 ) ;
}


// for branching when a.xyzw > b.xyzw
FORCEINLINE bool IsAllGreaterThan( const fltx4 &a, const fltx4 &b )
{
	return	SubFloat(a,0) > SubFloat(b,0) &&
		SubFloat(a,1) > SubFloat(b,1) &&
		SubFloat(a,2) > SubFloat(b,2) &&
		SubFloat(a,3) > SubFloat(b,3);
}

// for branching when a.xyzw >= b.xyzw
FORCEINLINE bool IsAllGreaterThanOrEq( const fltx4 &a, const fltx4 &b )
{
	return	SubFloat(a,0) >= SubFloat(b,0) &&
		SubFloat(a,1) >= SubFloat(b,1) &&
		SubFloat(a,2) >= SubFloat(b,2) &&
		SubFloat(a,3) >= SubFloat(b,3);
}

// For branching if all a.xyzw == b.xyzw
FORCEINLINE bool IsAllEqual( const fltx4 & a, const fltx4 & b )
{
	return	SubFloat(a,0) == SubFloat(b,0) &&
		SubFloat(a,1) == SubFloat(b,1) &&
		SubFloat(a,2) == SubFloat(b,2) &&
		SubFloat(a,3) == SubFloat(b,3);
}

FORCEINLINE int TestSignSIMD( const fltx4 & a )								// mask of which floats have the high bit set
{
	int nRet = 0;

	nRet |= ( SubInt( a, 0 ) & 0x80000000 ) >> 31; // sign(x) -> bit 0
	nRet |= ( SubInt( a, 1 ) & 0x80000000 ) >> 30; // sign(y) -> bit 1
	nRet |= ( SubInt( a, 2 ) & 0x80000000 ) >> 29; // sign(z) -> bit 2
	nRet |= ( SubInt( a, 3 ) & 0x80000000 ) >> 28; // sign(w) -> bit 3

	return nRet;
}

FORCEINLINE bool IsAnyNegative( const fltx4 & a )							// (a.x < 0) || (a.y < 0) || (a.z < 0) || (a.w < 0)
{
	return (0 != TestSignSIMD( a ));
}

FORCEINLINE fltx4 CmpEqSIMD( const fltx4 & a, const fltx4 & b )				// (a==b) ? ~0:0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) == SubFloat( b, 0 )) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) == SubFloat( b, 1 )) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) == SubFloat( b, 2 )) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) == SubFloat( b, 3 )) ? ~0 : 0;
	return retVal;
}

FORCEINLINE fltx4 CmpGtSIMD( const fltx4 & a, const fltx4 & b )				// (a>b) ? ~0:0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) > SubFloat( b, 0 )) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) > SubFloat( b, 1 )) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) > SubFloat( b, 2 )) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) > SubFloat( b, 3 )) ? ~0 : 0;
	return retVal;
}

FORCEINLINE fltx4 CmpGeSIMD( const fltx4 & a, const fltx4 & b )				// (a>=b) ? ~0:0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) >= SubFloat( b, 0 )) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) >= SubFloat( b, 1 )) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) >= SubFloat( b, 2 )) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) >= SubFloat( b, 3 )) ? ~0 : 0;
	return retVal;
}

FORCEINLINE fltx4 CmpLtSIMD( const fltx4 & a, const fltx4 & b )				// (a<b) ? ~0:0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) < SubFloat( b, 0 )) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) < SubFloat( b, 1 )) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) < SubFloat( b, 2 )) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) < SubFloat( b, 3 )) ? ~0 : 0;
	return retVal;
}

FORCEINLINE fltx4 CmpLeSIMD( const fltx4 & a, const fltx4 & b )				// (a<=b) ? ~0:0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) <= SubFloat( b, 0 )) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) <= SubFloat( b, 1 )) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) <= SubFloat( b, 2 )) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) <= SubFloat( b, 3 )) ? ~0 : 0;
	return retVal;
}

FORCEINLINE fltx4 CmpInBoundsSIMD( const fltx4 & a, const fltx4 & b )		// (a <= b && a >= -b) ? ~0 : 0
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = ( SubFloat( a, 0 ) <= SubFloat( b, 0 ) && SubFloat( a, 0 ) >= -SubFloat( b, 0 ) ) ? ~0 : 0;
	SubInt( retVal, 1 ) = ( SubFloat( a, 1 ) <= SubFloat( b, 1 ) && SubFloat( a, 1 ) >= -SubFloat( b, 1 ) ) ? ~0 : 0;
	SubInt( retVal, 2 ) = ( SubFloat( a, 2 ) <= SubFloat( b, 2 ) && SubFloat( a, 2 ) >= -SubFloat( b, 2 ) ) ? ~0 : 0;
	SubInt( retVal, 3 ) = ( SubFloat( a, 3 ) <= SubFloat( b, 3 ) && SubFloat( a, 3 ) >= -SubFloat( b, 3 ) ) ? ~0 : 0;
	return retVal;
}


FORCEINLINE fltx4 MaskedAssign( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
	return OrSIMD(
		AndSIMD( ReplacementMask, NewValue ),
		AndNotSIMD( ReplacementMask, OldValue ) );
}

// the same as MaskedAssign, only 5 instructions instead of 6
FORCEINLINE fltx4 SelectSIMD( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
	return XorSIMD( AndSIMD( XorSIMD( NewValue, OldValue ), ReplacementMask ), OldValue );
}

FORCEINLINE fltx4 ReplicateX4( float flValue )					//  a,a,a,a
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = flValue;
	SubFloat( retVal, 1 ) = flValue;
	SubFloat( retVal, 2 ) = flValue;
	SubFloat( retVal, 3 ) = flValue;
	return retVal;
}

/// replicate a single 32 bit integer value to all 4 components of an m128
FORCEINLINE fltx4 ReplicateIX4( int nValue )
{
	fltx4 retVal;
	SubInt( retVal, 0 ) = nValue;
	SubInt( retVal, 1 ) = nValue;
	SubInt( retVal, 2 ) = nValue;
	SubInt( retVal, 3 ) = nValue;
	return retVal;

}

// Round towards positive infinity
FORCEINLINE fltx4 CeilSIMD( const fltx4 &a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = ceil( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = ceil( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = ceil( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = ceil( SubFloat( a, 3 ) );
	return retVal;

}

// Round towards negative infinity
FORCEINLINE fltx4 FloorSIMD( const fltx4 &a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = floor( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = floor( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = floor( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = floor( SubFloat( a, 3 ) );
	return retVal;

}

FORCEINLINE fltx4 SqrtEstSIMD( const fltx4 & a )				// sqrt(a), more or less
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = sqrt( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = sqrt( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = sqrt( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = sqrt( SubFloat( a, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 SqrtSIMD( const fltx4 & a )					// sqrt(a)
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = sqrt( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = sqrt( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = sqrt( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = sqrt( SubFloat( a, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 ReciprocalSqrtEstSIMD( const fltx4 & a )		// 1/sqrt(a), more or less
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / sqrt( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = 1.0 / sqrt( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = 1.0 / sqrt( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = 1.0 / sqrt( SubFloat( a, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 ReciprocalSqrtEstSaturateSIMD( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / sqrt( SubFloat( a, 0 ) != 0.0f ? SubFloat( a, 0 ) : FLT_EPSILON );
	SubFloat( retVal, 1 ) = 1.0 / sqrt( SubFloat( a, 1 ) != 0.0f ? SubFloat( a, 1 ) : FLT_EPSILON );
	SubFloat( retVal, 2 ) = 1.0 / sqrt( SubFloat( a, 2 ) != 0.0f ? SubFloat( a, 2 ) : FLT_EPSILON );
	SubFloat( retVal, 3 ) = 1.0 / sqrt( SubFloat( a, 3 ) != 0.0f ? SubFloat( a, 3 ) : FLT_EPSILON );
	return retVal;
}

FORCEINLINE fltx4 ReciprocalSqrtSIMD( const fltx4 & a )			// 1/sqrt(a)
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / sqrt( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = 1.0 / sqrt( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = 1.0 / sqrt( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = 1.0 / sqrt( SubFloat( a, 3 ) );
	return retVal;
}

FORCEINLINE fltx4 ReciprocalEstSIMD( const fltx4 & a )			// 1/a, more or less
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / SubFloat( a, 0 );
	SubFloat( retVal, 1 ) = 1.0 / SubFloat( a, 1 );
	SubFloat( retVal, 2 ) = 1.0 / SubFloat( a, 2 );
	SubFloat( retVal, 3 ) = 1.0 / SubFloat( a, 3 );
	return retVal;
}

FORCEINLINE fltx4 ReciprocalSIMD( const fltx4 & a )				// 1/a
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / SubFloat( a, 0 );
	SubFloat( retVal, 1 ) = 1.0 / SubFloat( a, 1 );
	SubFloat( retVal, 2 ) = 1.0 / SubFloat( a, 2 );
	SubFloat( retVal, 3 ) = 1.0 / SubFloat( a, 3 );
	return retVal;
}

/// 1/x for all 4 values.
/// 1/0 will result in a big but NOT infinite result
FORCEINLINE fltx4 ReciprocalEstSaturateSIMD( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / (SubFloat( a, 0 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 0 ));
	SubFloat( retVal, 1 ) = 1.0 / (SubFloat( a, 1 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 1 ));
	SubFloat( retVal, 2 ) = 1.0 / (SubFloat( a, 2 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 2 ));
	SubFloat( retVal, 3 ) = 1.0 / (SubFloat( a, 3 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 3 ));
	return retVal;
}

FORCEINLINE fltx4 ReciprocalSaturateSIMD( const fltx4 & a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = 1.0 / (SubFloat( a, 0 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 0 ));
	SubFloat( retVal, 1 ) = 1.0 / (SubFloat( a, 1 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 1 ));
	SubFloat( retVal, 2 ) = 1.0 / (SubFloat( a, 2 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 2 ));
	SubFloat( retVal, 3 ) = 1.0 / (SubFloat( a, 3 ) == 0.0f ? FLT_EPSILON : SubFloat( a, 3 ));
	return retVal;
}

// 2^x for all values (the antilog)
FORCEINLINE fltx4 ExpSIMD( const fltx4 &toPower )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = powf( 2, SubFloat(toPower, 0) );
	SubFloat( retVal, 1 ) = powf( 2, SubFloat(toPower, 1) );
	SubFloat( retVal, 2 ) = powf( 2, SubFloat(toPower, 2) );
	SubFloat( retVal, 3 ) = powf( 2, SubFloat(toPower, 3) );

	return retVal;
}

FORCEINLINE fltx4 Dot3SIMD( const fltx4 &a, const fltx4 &b )
{
	float flDot = SubFloat( a, 0 ) * SubFloat( b, 0 ) +
		SubFloat( a, 1 ) * SubFloat( b, 1 ) + 
		SubFloat( a, 2 ) * SubFloat( b, 2 );
	return ReplicateX4( flDot );
}

FORCEINLINE fltx4 Dot4SIMD( const fltx4 &a, const fltx4 &b )
{
	float flDot = SubFloat( a, 0 ) * SubFloat( b, 0 ) +
		SubFloat( a, 1 ) * SubFloat( b, 1 ) + 
		SubFloat( a, 2 ) * SubFloat( b, 2 ) +
		SubFloat( a, 3 ) * SubFloat( b, 3 );
	return ReplicateX4( flDot );
}

// Clamps the components of a vector to a specified minimum and maximum range.
FORCEINLINE fltx4 ClampVectorSIMD( FLTX4 in, FLTX4 min, FLTX4 max)
{
	return MaxSIMD( min, MinSIMD( max, in ) );
}

// Squelch the w component of a vector to +0.0.
// Most efficient when you say a = SetWToZeroSIMD(a) (avoids a copy)
FORCEINLINE fltx4 SetWToZeroSIMD( const fltx4 & a )
{
	fltx4 retval;
	retval = a;
	SubFloat( retval, 0 ) = 0;
	return retval;
}

FORCEINLINE fltx4 LoadUnalignedSIMD( const void *pSIMD )
{
	return *( reinterpret_cast< const fltx4 *> ( pSIMD ) );
}

FORCEINLINE fltx4 LoadUnaligned3SIMD( const void *pSIMD )
{
	return *( reinterpret_cast< const fltx4 *> ( pSIMD ) );
}

FORCEINLINE fltx4 LoadAlignedSIMD( const void *pSIMD )
{
	return *( reinterpret_cast< const fltx4 *> ( pSIMD ) );
}

// for the transitional class -- load a 3-by VectorAligned and squash its w component
FORCEINLINE fltx4 LoadAlignedSIMD( const VectorAligned & pSIMD )
{
	fltx4 retval = LoadAlignedSIMD(pSIMD.Base());
	// squelch w
	SubInt( retval, 3 ) = 0;
	return retval;
}

FORCEINLINE void StoreAlignedSIMD( float *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< fltx4 *> ( pSIMD ) ) = a;
}

FORCEINLINE void StoreUnalignedSIMD( float *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< fltx4 *> ( pSIMD ) ) = a;
}

FORCEINLINE void StoreUnaligned3SIMD( float *pSIMD, const fltx4 & a )
{
	*pSIMD     = SubFloat(a, 0);
	*(pSIMD+1) = SubFloat(a, 1);
	*(pSIMD+2) = SubFloat(a, 2);
}

// strongly typed -- syntactic castor oil used for typechecking as we transition to SIMD
FORCEINLINE void StoreAligned3SIMD( VectorAligned * RESTRICT pSIMD, const fltx4 & a )
{
	StoreAlignedSIMD(pSIMD->Base(),a);
}

FORCEINLINE void TransposeSIMD( fltx4 & x, fltx4 & y, fltx4 & z, fltx4 & w )
{
#define SWAP_FLOATS( _a_, _ia_, _b_, _ib_ ) { float tmp = SubFloat( _a_, _ia_ ); SubFloat( _a_, _ia_ ) = SubFloat( _b_, _ib_ ); SubFloat( _b_, _ib_ ) = tmp; }
	SWAP_FLOATS( x, 1, y, 0 );
	SWAP_FLOATS( x, 2, z, 0 );
	SWAP_FLOATS( x, 3, w, 0 );
	SWAP_FLOATS( y, 2, z, 1 );
	SWAP_FLOATS( y, 3, w, 1 );
	SWAP_FLOATS( z, 3, w, 2 );
}

// find the lowest component of a.x, a.y, a.z,
// and replicate it to the whole return value.
FORCEINLINE fltx4 FindLowestSIMD3( const fltx4 & a )
{
	float lowest = min( min( SubFloat(a, 0), SubFloat(a, 1) ), SubFloat(a, 2));
	return ReplicateX4(lowest);
}

// find the highest component of a.x, a.y, a.z,
// and replicate it to the whole return value.
FORCEINLINE fltx4 FindHighestSIMD3( const fltx4 & a )
{
	float highest = max( max( SubFloat(a, 0), SubFloat(a, 1) ), SubFloat(a, 2));
	return ReplicateX4(highest);
}

// Fixed-point conversion and save as SIGNED INTS.
// pDest->x = Int (vSrc.x)
// note: some architectures have means of doing 
// fixed point conversion when the fix depth is
// specified as an immediate.. but there is no way 
// to guarantee an immediate as a parameter to function
// like this.
FORCEINLINE void ConvertStoreAsIntsSIMD(intx4 * RESTRICT pDest, const fltx4 &vSrc)
{
	(*pDest)[0] = SubFloat(vSrc, 0);
	(*pDest)[1] = SubFloat(vSrc, 1);
	(*pDest)[2] = SubFloat(vSrc, 2);
	(*pDest)[3] = SubFloat(vSrc, 3);
}

// ------------------------------------
// INTEGER SIMD OPERATIONS.
// ------------------------------------
// splat all components of a vector to a signed immediate int number.
FORCEINLINE fltx4 IntSetImmediateSIMD( int nValue )
{
	fltx4 retval;
	SubInt( retval, 0 ) = SubInt( retval, 1 ) = SubInt( retval, 2 ) = SubInt( retval, 3) = nValue;
	return retval;
}

// Load 4 aligned words into a SIMD register
FORCEINLINE i32x4 LoadAlignedIntSIMD(const void * RESTRICT pSIMD)
{
	return *( reinterpret_cast< const i32x4 *> ( pSIMD ) );
}

// Load 4 unaligned words into a SIMD register
FORCEINLINE i32x4 LoadUnalignedIntSIMD( const void * RESTRICT pSIMD)
{
	return *( reinterpret_cast< const i32x4 *> ( pSIMD ) );
}

// save into four words, 16-byte aligned
FORCEINLINE void StoreAlignedIntSIMD( int32 *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< i32x4 *> ( pSIMD ) ) = a;
}

FORCEINLINE void StoreAlignedIntSIMD( intx4 &pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< i32x4 *> ( pSIMD.Base() ) ) = a;
}

FORCEINLINE void StoreUnalignedIntSIMD( int32 *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< i32x4 *> ( pSIMD ) ) = a;
}

// Take a fltx4 containing fixed-point uints and 
// return them as single precision floats. No
// fixed point conversion is done.
FORCEINLINE fltx4 UnsignedIntConvertToFltSIMD( const u32x4 &vSrcA )
{
	Assert(0);			/* pc has no such operation */
	fltx4 retval;
	SubFloat( retval, 0 ) = ( (float) SubInt( vSrcA, 0 ) );
	SubFloat( retval, 1 ) = ( (float) SubInt( vSrcA, 1 ) );
	SubFloat( retval, 2 ) = ( (float) SubInt( vSrcA, 2 ) );
	SubFloat( retval, 3 ) = ( (float) SubInt( vSrcA, 3 ) );
	return retval;
}


#if 0				/* pc has no such op */
// Take a fltx4 containing fixed-point sints and 
// return them as single precision floats. No 
// fixed point conversion is done.
FORCEINLINE fltx4 SignedIntConvertToFltSIMD( const i32x4 &vSrcA )
{
	fltx4 retval;
	SubFloat( retval, 0 ) = ( (float) (reinterpret_cast<int32 *>(&vSrcA.m128_s32[0])) );
	SubFloat( retval, 1 ) = ( (float) (reinterpret_cast<int32 *>(&vSrcA.m128_s32[1])) );
	SubFloat( retval, 2 ) = ( (float) (reinterpret_cast<int32 *>(&vSrcA.m128_s32[2])) );
	SubFloat( retval, 3 ) = ( (float) (reinterpret_cast<int32 *>(&vSrcA.m128_s32[3])) );
	return retval;
}


/*
  works on fltx4's as if they are four uints.
  the first parameter contains the words to be shifted,
  the second contains the amount to shift by AS INTS

  for i = 0 to 3
  shift = vSrcB_i*32:(i*32)+4
  vReturned_i*32:(i*32)+31 = vSrcA_i*32:(i*32)+31 << shift
*/
FORCEINLINE i32x4 IntShiftLeftWordSIMD(const i32x4 &vSrcA, const i32x4 &vSrcB)
{
	i32x4 retval;
	SubInt(retval, 0) = SubInt(vSrcA, 0) << SubInt(vSrcB, 0);
	SubInt(retval, 1) = SubInt(vSrcA, 1) << SubInt(vSrcB, 1);
	SubInt(retval, 2) = SubInt(vSrcA, 2) << SubInt(vSrcB, 2);
	SubInt(retval, 3) = SubInt(vSrcA, 3) << SubInt(vSrcB, 3);


	return retval;
}
#endif

#elif ( defined( _X360 ) )

//---------------------------------------------------------------------
// X360 implementation
//---------------------------------------------------------------------

FORCEINLINE float & FloatSIMD( fltx4 & a, int idx )
{
	fltx4_union & a_union = (fltx4_union &)a;
	return a_union.m128_f32[idx];
}

FORCEINLINE unsigned int & UIntSIMD( fltx4 & a, int idx )
{
	fltx4_union & a_union = (fltx4_union &)a;
	return a_union.m128_u32[idx];
}

FORCEINLINE fltx4 AddSIMD( const fltx4 & a, const fltx4 & b )
{
	return __vaddfp( a, b );
}

FORCEINLINE fltx4 SubSIMD( const fltx4 & a, const fltx4 & b )				// a-b
{
	return __vsubfp( a, b );
}

FORCEINLINE fltx4 MulSIMD( const fltx4 & a, const fltx4 & b )				// a*b
{
	return __vmulfp( a, b );
}

FORCEINLINE fltx4 MaddSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// a*b + c
{
	return __vmaddfp( a, b, c );
}

FORCEINLINE fltx4 MsubSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// c - a*b
{
	return __vnmsubfp( a, b, c );
};

FORCEINLINE fltx4 Dot3SIMD( const fltx4 &a, const fltx4 &b )
{
	return __vmsum3fp( a, b );
}

FORCEINLINE fltx4 Dot4SIMD( const fltx4 &a, const fltx4 &b )
{
	return __vmsum4fp( a, b );
}

FORCEINLINE fltx4 SinSIMD( const fltx4 &radians )
{
	return XMVectorSin( radians );
}

FORCEINLINE void SinCos3SIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )
{
	XMVectorSinCos( &sine, &cosine, radians ); 	
}

FORCEINLINE void SinCosSIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )			
{
	XMVectorSinCos( &sine, &cosine, radians ); 	
}

FORCEINLINE void CosSIMD( fltx4 &cosine, const fltx4 &radians )				
{
	cosine = XMVectorCos( radians ); 	
}

FORCEINLINE fltx4 ArcSinSIMD( const fltx4 &sine )
{
	return XMVectorASin( sine );
}

FORCEINLINE fltx4 ArcCosSIMD( const fltx4 &cs )
{
	return XMVectorACos( cs );
}

// tan^1(a/b) .. ie, pass sin in as a and cos in as b
FORCEINLINE fltx4 ArcTan2SIMD( const fltx4 &a, const fltx4 &b )
{
	return XMVectorATan2( a, b );
}

// DivSIMD defined further down, since it uses ReciprocalSIMD

FORCEINLINE fltx4 MaxSIMD( const fltx4 & a, const fltx4 & b )				// max(a,b)
{
	return __vmaxfp( a, b );
}

FORCEINLINE fltx4 MinSIMD( const fltx4 & a, const fltx4 & b )				// min(a,b)
{
	return __vminfp( a, b );
}

FORCEINLINE fltx4 AndSIMD( const fltx4 & a, const fltx4 & b )				// a & b
{
    return __vand( a, b );
}

FORCEINLINE fltx4 AndNotSIMD( const fltx4 & a, const fltx4 & b )			// ~a & b
{
	// NOTE: a and b are swapped in the call: SSE complements the first argument, VMX the second
    return __vandc( b, a );
}

FORCEINLINE fltx4 XorSIMD( const fltx4 & a, const fltx4 & b )				// a ^ b
{
    return __vxor( a, b );
}

FORCEINLINE fltx4 OrSIMD( const fltx4 & a, const fltx4 & b )				// a | b
{
    return __vor( a, b );
}

FORCEINLINE fltx4 NegSIMD(const fltx4 &a) // negate: -a
{
	return XMVectorNegate(a);
}

FORCEINLINE bool IsAllZeros( const fltx4 & a )								// all floats of a zero?
{
	unsigned int equalFlags = 0;
    __vcmpeqfpR( a, Four_Zeros, &equalFlags );
    return XMComparisonAllTrue( equalFlags );
}

FORCEINLINE bool IsAnyZeros( const fltx4 & a )								// any floats are zero?
{
	unsigned int conditionregister;
	XMVectorEqualR(&conditionregister, a, XMVectorZero());
	return XMComparisonAnyTrue(conditionregister);
}

FORCEINLINE bool IsAnyXYZZero( const fltx4 &a )								// are any of x,y,z zero?
{
	// copy a's x component into w, in case w was zero.
	fltx4 temp = __vrlimi(a, a, 1, 1);
	unsigned int conditionregister;
	XMVectorEqualR(&conditionregister, temp, XMVectorZero());
	return XMComparisonAnyTrue(conditionregister);
}

// for branching when a.xyzw > b.xyzw
FORCEINLINE bool IsAllGreaterThan( const fltx4 &a, const fltx4 &b )
{
	unsigned int cr;
	XMVectorGreaterR(&cr,a,b);
	return XMComparisonAllTrue(cr);
}

// for branching when a.xyzw >= b.xyzw
FORCEINLINE bool IsAllGreaterThanOrEq( const fltx4 &a, const fltx4 &b )
{
	unsigned int cr;
	XMVectorGreaterOrEqualR(&cr,a,b);
	return XMComparisonAllTrue(cr);
}

// For branching if all a.xyzw == b.xyzw
FORCEINLINE bool IsAllEqual( const fltx4 & a, const fltx4 & b )
{
	unsigned int cr;
	XMVectorEqualR(&cr,a,b);
	return XMComparisonAllTrue(cr);
}


FORCEINLINE int TestSignSIMD( const fltx4 & a )								// mask of which floats have the high bit set
{
	// NOTE: this maps to SSE way better than it does to VMX (most code uses IsAnyNegative(), though)
	int nRet = 0;

	const fltx4_union & a_union = (const fltx4_union &)a;
	nRet |= ( a_union.m128_u32[0] & 0x80000000 ) >> 31; // sign(x) -> bit 0
	nRet |= ( a_union.m128_u32[1] & 0x80000000 ) >> 30; // sign(y) -> bit 1
	nRet |= ( a_union.m128_u32[2] & 0x80000000 ) >> 29; // sign(z) -> bit 2
	nRet |= ( a_union.m128_u32[3] & 0x80000000 ) >> 28; // sign(w) -> bit 3

	return nRet;
}

// Squelch the w component of a vector to +0.0.
// Most efficient when you say a = SetWToZeroSIMD(a) (avoids a copy)
FORCEINLINE fltx4 SetWToZeroSIMD( const fltx4 & a )
{
	return __vrlimi( a, __vzero(), 1, 0 );
}

FORCEINLINE bool IsAnyNegative( const fltx4 & a )							// (a.x < 0) || (a.y < 0) || (a.z < 0) || (a.w < 0)
{
	// NOTE: this tests the top bits of each vector element using integer math
	//       (so it ignores NaNs - it will return true for "-NaN")
	unsigned int equalFlags = 0;
    fltx4 signMask = __vspltisw( -1 );             // 0xFFFFFFFF 0xFFFFFFFF 0xFFFFFFFF 0xFFFFFFFF (low order 5 bits of each element = 31)
    signMask       = __vslw( signMask, signMask ); // 0x80000000 0x80000000 0x80000000 0x80000000 
	__vcmpequwR( Four_Zeros, __vand( signMask, a ), &equalFlags );
	return !XMComparisonAllTrue( equalFlags );
}

FORCEINLINE fltx4 CmpEqSIMD( const fltx4 & a, const fltx4 & b )				// (a==b) ? ~0:0
{
    return __vcmpeqfp( a, b );
}


FORCEINLINE fltx4 CmpGtSIMD( const fltx4 & a, const fltx4 & b )				// (a>b) ? ~0:0
{
    return __vcmpgtfp( a, b );
}

FORCEINLINE fltx4 CmpGeSIMD( const fltx4 & a, const fltx4 & b )				// (a>=b) ? ~0:0
{
    return __vcmpgefp( a, b );
}

FORCEINLINE fltx4 CmpLtSIMD( const fltx4 & a, const fltx4 & b )				// (a<b) ? ~0:0
{
    return __vcmpgtfp( b, a );
}

FORCEINLINE fltx4 CmpLeSIMD( const fltx4 & a, const fltx4 & b )				// (a<=b) ? ~0:0
{
    return __vcmpgefp( b, a );
}

FORCEINLINE fltx4 CmpInBoundsSIMD( const fltx4 & a, const fltx4 & b )		// (a <= b && a >= -b) ? ~0 : 0
{
	return XMVectorInBounds( a, b );
}

// returned[i] = ReplacementMask[i] == 0 ? OldValue : NewValue
FORCEINLINE fltx4 MaskedAssign( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
    return __vsel( OldValue, NewValue, ReplacementMask );
}

FORCEINLINE fltx4 SelectSIMD( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
	return __vsel( OldValue, NewValue, ReplacementMask );
}

// AKA "Broadcast", "Splat"
FORCEINLINE fltx4 ReplicateX4( float flValue )					//  a,a,a,a
{
	// NOTE: if flValue comes from a register, this causes a Load-Hit-Store stall (don't mix fpu/vpu math!)
	float * pValue = &flValue;
	Assert( pValue );
    Assert( ((unsigned int)pValue & 3) == 0);
	return __vspltw( __lvlx( pValue, 0 ), 0 );
}

FORCEINLINE fltx4 ReplicateX4( const float *pValue )					//  a,a,a,a
{
	Assert( pValue );
	return __vspltw( __lvlx( pValue, 0 ), 0 );
}

/// replicate a single 32 bit integer value to all 4 components of an m128
FORCEINLINE fltx4 ReplicateIX4( int nValue )
{
	// NOTE: if nValue comes from a register, this causes a Load-Hit-Store stall (should not mix ints with fltx4s!)
	int * pValue = &nValue;
	Assert( pValue );
    Assert( ((unsigned int)pValue & 3) == 0);
	return __vspltw( __lvlx( pValue, 0 ), 0 );
}

// Round towards positive infinity
FORCEINLINE fltx4 CeilSIMD( const fltx4 &a )
{
	return __vrfip(a);
}

// Round towards nearest integer
FORCEINLINE fltx4 RoundSIMD( const fltx4 &a )
{
	return __vrfin(a);
}

// Round towards negative infinity
FORCEINLINE fltx4 FloorSIMD( const fltx4 &a )
{
	return __vrfim(a);
}

FORCEINLINE fltx4 SqrtEstSIMD( const fltx4 & a )				// sqrt(a), more or less
{
	// This is emulated from rsqrt
	return XMVectorSqrtEst( a );
}

FORCEINLINE fltx4 SqrtSIMD( const fltx4 & a )					// sqrt(a)
{
	// This is emulated from rsqrt
	return XMVectorSqrt( a );
}

FORCEINLINE fltx4 ReciprocalSqrtEstSIMD( const fltx4 & a )		// 1/sqrt(a), more or less
{
    return __vrsqrtefp( a );
}

FORCEINLINE fltx4 ReciprocalSqrtEstSaturateSIMD( const fltx4 & a )
{
	// Convert zeros to epsilons
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 a_safe = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	return ReciprocalSqrtEstSIMD( a_safe );
}

FORCEINLINE fltx4 ReciprocalSqrtSIMD( const fltx4 & a )			// 1/sqrt(a)
{
	// This uses Newton-Raphson to improve the HW result
 	return XMVectorReciprocalSqrt( a );
}

FORCEINLINE fltx4 ReciprocalEstSIMD( const fltx4 & a )			// 1/a, more or less
{
    return __vrefp( a );
}

/// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
/// No error checking!
FORCEINLINE fltx4 ReciprocalSIMD( const fltx4 & a )				// 1/a
{
	// This uses Newton-Raphson to improve the HW result
	return XMVectorReciprocal( a );
}

// FIXME: on 360, this is very slow, since it uses ReciprocalSIMD (do we need DivEstSIMD?)
FORCEINLINE fltx4 DivSIMD( const fltx4 & a, const fltx4 & b )	// a/b
{
	return MulSIMD( ReciprocalSIMD( b ), a );
}

/// 1/x for all 4 values.
/// 1/0 will result in a big but NOT infinite result
FORCEINLINE fltx4 ReciprocalEstSaturateSIMD( const fltx4 & a )
{
	// Convert zeros to epsilons
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 a_safe = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	return ReciprocalEstSIMD( a_safe );
}

FORCEINLINE fltx4 ReciprocalSaturateSIMD( const fltx4 & a )
{
	// Convert zeros to epsilons
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 a_safe = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	return ReciprocalSIMD( a_safe );

	// FIXME: This could be faster (BUT: it doesn't preserve the sign of -0.0, whereas the above does)
	// fltx4 zeroMask = CmpEqSIMD( Four_Zeros, a );
	// fltx4 a_safe = XMVectorSelect( a, Four_Epsilons, zeroMask );
	// return ReciprocalSIMD( a_safe );
}

// CHRISG: is it worth doing integer bitfiddling for this?
// 2^x for all values (the antilog)
FORCEINLINE fltx4 ExpSIMD( const fltx4 &toPower )
{
	return XMVectorExp(toPower);
}

// Clamps the components of a vector to a specified minimum and maximum range.
FORCEINLINE fltx4 ClampVectorSIMD( FLTX4 in, FLTX4 min, FLTX4 max)
{
	return XMVectorClamp(in, min, max);
}

FORCEINLINE fltx4 LoadUnalignedSIMD( const void *pSIMD )
{
	return XMLoadVector4( pSIMD );
}

// load a 3-vector (as opposed to LoadUnalignedSIMD, which loads a 4-vec). 
FORCEINLINE fltx4 LoadUnaligned3SIMD( const void *pSIMD )
{
	return XMLoadVector3( pSIMD );
}

FORCEINLINE fltx4 LoadAlignedSIMD( const void *pSIMD )
{
	return *( reinterpret_cast< const fltx4 *> ( pSIMD ) );
}

// for the transitional class -- load a 3-by VectorAligned and squash its w component
FORCEINLINE fltx4 LoadAlignedSIMD( const VectorAligned & pSIMD )
{
	fltx4 out = XMLoadVector3A(pSIMD.Base());
	// squelch w
	return __vrlimi( out, __vzero(), 1, 0 );
}

// for the transitional class -- load a 3-by VectorAligned and squash its w component
FORCEINLINE fltx4 LoadAlignedSIMD( const VectorAligned * RESTRICT pSIMD )
{
	fltx4 out = XMLoadVector3A(pSIMD);
	// squelch w
	return __vrlimi( out, __vzero(), 1, 0 );
}

FORCEINLINE void StoreAlignedSIMD( float *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< fltx4 *> ( pSIMD ) ) = a;
}

FORCEINLINE void StoreUnalignedSIMD( float *pSIMD, const fltx4 & a )
{
	XMStoreVector4( pSIMD, a );
}

FORCEINLINE void StoreUnaligned3SIMD( float *pSIMD, const fltx4 & a )
{
	XMStoreVector3( pSIMD, a );
}


// strongly typed -- for typechecking as we transition to SIMD
FORCEINLINE void StoreAligned3SIMD( VectorAligned * RESTRICT pSIMD, const fltx4 & a )
{
	XMStoreVector3A(pSIMD->Base(),a);
}


// Fixed-point conversion and save as SIGNED INTS.
// pDest->x = Int (vSrc.x)
// note: some architectures have means of doing 
// fixed point conversion when the fix depth is
// specified as an immediate.. but there is no way 
// to guarantee an immediate as a parameter to function
// like this.
FORCEINLINE void ConvertStoreAsIntsSIMD(intx4 * RESTRICT pDest, const fltx4 &vSrc)
{
	fltx4 asInt = __vctsxs( vSrc, 0 );
	XMStoreVector4A(pDest->Base(), asInt);
}

FORCEINLINE void TransposeSIMD( fltx4 & x, fltx4 & y, fltx4 & z, fltx4 & w )
{
	XMMATRIX xyzwMatrix = _XMMATRIX( x, y, z, w );
	xyzwMatrix = XMMatrixTranspose( xyzwMatrix );
	x = xyzwMatrix.r[0];
	y = xyzwMatrix.r[1];
	z = xyzwMatrix.r[2];
	w = xyzwMatrix.r[3];
}

// Return one in the fastest way -- faster even than loading.
FORCEINLINE fltx4 LoadZeroSIMD( void )
{
	return XMVectorZero();
}

// Return one in the fastest way -- faster even than loading.
FORCEINLINE fltx4 LoadOneSIMD( void )
{
	return XMVectorSplatOne();
}

FORCEINLINE fltx4 SplatXSIMD( fltx4 a )
{
	return XMVectorSplatX( a );
}

FORCEINLINE fltx4 SplatYSIMD( fltx4 a )
{
	return XMVectorSplatY( a );
}

FORCEINLINE fltx4 SplatZSIMD( fltx4 a )
{
	return XMVectorSplatZ( a );
}

FORCEINLINE fltx4 SplatWSIMD( fltx4 a )
{
	return XMVectorSplatW( a );
}

FORCEINLINE fltx4 SetXSIMD( const fltx4& a, const fltx4& x )
{
	fltx4 result = __vrlimi(a, x, 8, 0);
	return result;
}

FORCEINLINE fltx4 SetYSIMD( const fltx4& a, const fltx4& y )
{
	fltx4 result = __vrlimi(a, y, 4, 0);
	return result;
}

FORCEINLINE fltx4 SetZSIMD( const fltx4& a, const fltx4& z )
{
	fltx4 result = __vrlimi(a, z, 2, 0);
	return result;
}

FORCEINLINE fltx4 SetWSIMD( const fltx4& a, const fltx4& w )
{
	fltx4 result = __vrlimi(a, w, 1, 0);
	return result;
}

FORCEINLINE fltx4 SetComponentSIMD( const fltx4& a, int nComponent, float flValue )
{
	static int s_nVrlimiMask[4] = { 8, 4, 2, 1 };
	fltx4 val = ReplicateX4( flValue );
	fltx4 result = __vrlimi(a, val, s_nVrlimiMask[nComponent], 0);
	return result;
}

FORCEINLINE fltx4 RotateLeft( const fltx4 & a )
{
	fltx4 compareOne = a;
	return __vrlimi( compareOne, a, 8 | 4 | 2 | 1, 1 );
}

FORCEINLINE fltx4 RotateLeft2( const fltx4 & a )
{
	fltx4 compareOne = a;
	return __vrlimi( compareOne, a, 8 | 4 | 2 | 1, 2 );
}



// find the lowest component of a.x, a.y, a.z,
// and replicate it to the whole return value.
// ignores a.w.
// Though this is only five instructions long,
// they are all dependent, making this stall city.
// Forcing this inline should hopefully help with scheduling.
FORCEINLINE fltx4 FindLowestSIMD3( const fltx4 & a )
{
	// a is [x,y,z,G] (where G is garbage)
	// rotate left by one 
	fltx4 compareOne = a ;
	compareOne = __vrlimi( compareOne, a, 8 | 4 , 1 );
	// compareOne is [y,z,G,G]
	fltx4 retval = MinSIMD( a, compareOne );
	// retVal is [min(x,y), min(y,z), G, G]
	compareOne = __vrlimi( compareOne, a, 8 , 2);
	// compareOne is [z, G, G, G]
	retval = MinSIMD( retval, compareOne );
	// retVal = [ min(min(x,y),z), G, G, G ]
	
	// splat the x component out to the whole vector and return
	return SplatXSIMD( retval );
}

// find the highest component of a.x, a.y, a.z,
// and replicate it to the whole return value.
// ignores a.w.
// Though this is only five instructions long,
// they are all dependent, making this stall city.
// Forcing this inline should hopefully help with scheduling.
FORCEINLINE fltx4 FindHighestSIMD3( const fltx4 & a )
{
	// a is [x,y,z,G] (where G is garbage)
	// rotate left by one 
	fltx4 compareOne = a ;
	compareOne = __vrlimi( compareOne, a, 8 | 4 , 1 );
	// compareOne is [y,z,G,G]
	fltx4 retval = MaxSIMD( a, compareOne );
	// retVal is [max(x,y), max(y,z), G, G]
	compareOne = __vrlimi( compareOne, a, 8 , 2);
	// compareOne is [z, G, G, G]
	retval = MaxSIMD( retval, compareOne );
	// retVal = [ max(max(x,y),z), G, G, G ]

	// splat the x component out to the whole vector and return
	return SplatXSIMD( retval );
}


// Transform many (horizontal) points in-place by a 3x4 matrix,
// here already loaded onto three fltx4 registers. 
// The points must be stored as 16-byte aligned. They are points
// and not vectors because we assume the w-component to be 1. 
// To spare yourself the annoyance of loading the matrix yourself,
// use one of the overloads below.
void TransformManyPointsBy(VectorAligned * RESTRICT pVectors, unsigned int numVectors, FLTX4 mRow1, FLTX4 mRow2, FLTX4 mRow3);

// Transform many (horizontal) points in-place by a 3x4 matrix.
// The points must be stored as 16-byte aligned. They are points
// and not vectors because we assume the w-component to be 1. 
// In this function, the matrix need not be aligned.
FORCEINLINE void TransformManyPointsBy(VectorAligned * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t &pMatrix)
{
	return TransformManyPointsBy(pVectors, numVectors, 
								 LoadUnalignedSIMD( pMatrix[0] ), LoadUnalignedSIMD( pMatrix[1] ), LoadUnalignedSIMD( pMatrix[2] ) );
}

// Transform many (horizontal) points in-place by a 3x4 matrix.
// The points must be stored as 16-byte aligned. They are points
// and not vectors because we assume the w-component to be 1. 
// In this function, the matrix must itself be aligned on a 16-byte
// boundary.
FORCEINLINE void TransformManyPointsByA(VectorAligned * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t &pMatrix)
{
	return TransformManyPointsBy(pVectors, numVectors, 
								 LoadAlignedSIMD( pMatrix[0] ), LoadAlignedSIMD( pMatrix[1] ), LoadAlignedSIMD( pMatrix[2] ) );
}

// ------------------------------------
// INTEGER SIMD OPERATIONS.
// ------------------------------------

// Load 4 aligned words into a SIMD register
FORCEINLINE i32x4 LoadAlignedIntSIMD( const void * RESTRICT pSIMD)
{
	return XMLoadVector4A(pSIMD);
}

// Load 4 unaligned words into a SIMD register
FORCEINLINE i32x4 LoadUnalignedIntSIMD(const void * RESTRICT pSIMD)
{
	return XMLoadVector4( pSIMD );
}

// save into four words, 16-byte aligned
FORCEINLINE void StoreAlignedIntSIMD( int32 *pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< i32x4 *> ( pSIMD ) ) = a;
}

FORCEINLINE void StoreAlignedIntSIMD( intx4 &pSIMD, const fltx4 & a )
{
	*( reinterpret_cast< i32x4 *> ( pSIMD.Base() ) ) = a;
}

FORCEINLINE void StoreUnalignedIntSIMD( int32 *pSIMD, const fltx4 & a )
{
	XMStoreVector4(pSIMD, a);
}


// Take a fltx4 containing fixed-point uints and 
// return them as single precision floats. No
// fixed point conversion is done.
FORCEINLINE fltx4 UnsignedIntConvertToFltSIMD( const i32x4 &vSrcA )
{
	return __vcfux( vSrcA, 0 );
}


// Take a fltx4 containing fixed-point sints and 
// return them as single precision floats. No 
// fixed point conversion is done.
FORCEINLINE fltx4 SignedIntConvertToFltSIMD( const i32x4 &vSrcA )
{
	return __vcfsx( vSrcA, 0 );
}

// Take a fltx4 containing fixed-point uints and 
// return them as single precision floats. Each uint
// will be divided by 2^immed after conversion
// (eg, this is fixed point math). 
/* as if:
   FORCEINLINE fltx4 UnsignedIntConvertToFltSIMD( const i32x4 &vSrcA, unsigned int uImmed )
   {
   return __vcfux( vSrcA, uImmed );
   }
*/
#define UnsignedFixedIntConvertToFltSIMD(vSrcA, uImmed) (__vcfux( (vSrcA), (uImmed) ))

// Take a fltx4 containing fixed-point sints and 
// return them as single precision floats. Each int
// will be divided by 2^immed (eg, this is fixed point
// math). 
/* as if:
   FORCEINLINE fltx4 SignedIntConvertToFltSIMD( const i32x4 &vSrcA, unsigned int uImmed )
   {
   return __vcfsx( vSrcA, uImmed );
   }
*/
#define SignedFixedIntConvertToFltSIMD(vSrcA, uImmed) (__vcfsx( (vSrcA), (uImmed) ))

// set all components of a vector to a signed immediate int number.
/* as if:
   FORCEINLINE fltx4 IntSetImmediateSIMD(int toImmediate)
   {
   return __vspltisw( toImmediate );
   }
*/
#define IntSetImmediateSIMD(x) (__vspltisw(x))

/*
  works on fltx4's as if they are four uints.
  the first parameter contains the words to be shifted,
  the second contains the amount to shift by AS INTS

  for i = 0 to 3
  shift = vSrcB_i*32:(i*32)+4
  vReturned_i*32:(i*32)+31 = vSrcA_i*32:(i*32)+31 << shift
*/
FORCEINLINE fltx4 IntShiftLeftWordSIMD(fltx4 vSrcA, fltx4 vSrcB)
{
	return __vslw(vSrcA, vSrcB);
}

FORCEINLINE float SubFloat( const fltx4 & a, int idx )
{
	// NOTE: if the output goes into a register, this causes a Load-Hit-Store stall (don't mix fpu/vpu math!)
	const fltx4_union & a_union = (const fltx4_union &)a;
	return a_union.m128_f32[ idx ];
}

FORCEINLINE float & SubFloat( fltx4 & a, int idx )
{
	fltx4_union & a_union = (fltx4_union &)a;
	return a_union.m128_f32[idx];
}

FORCEINLINE uint32 SubFloatConvertToInt( const fltx4 & a, int idx )
{
	fltx4 t = __vctuxs( a, 0 );
	const fltx4_union & a_union = (const fltx4_union &)t;
	return a_union.m128_u32[idx];
}


FORCEINLINE uint32 SubInt( const fltx4 & a, int idx )
{
	const fltx4_union & a_union = (const fltx4_union &)a;
	return a_union.m128_u32[idx];
}

FORCEINLINE uint32 & SubInt( fltx4 & a, int idx )
{
	fltx4_union & a_union = (fltx4_union &)a;
	return a_union.m128_u32[idx];
}

#else

//---------------------------------------------------------------------
// Intel/SSE implementation
//---------------------------------------------------------------------

FORCEINLINE void StoreAlignedSIMD( float * RESTRICT pSIMD, const fltx4 & a )
{
	_mm_store_ps( pSIMD, a );
}

FORCEINLINE void StoreAlignedSIMD( short * RESTRICT pSIMD, const shortx8 & a )
{
	_mm_store_si128( (shortx8 *)pSIMD, a );
}

FORCEINLINE void StoreUnalignedSIMD( float * RESTRICT pSIMD, const fltx4 & a )
{
	_mm_storeu_ps( pSIMD, a );
}

FORCEINLINE void StoreUnalignedSIMD( short* RESTRICT pSIMD, const shortx8& a )
{
	_mm_storeu_si128((shortx8*)pSIMD, a);
}

FORCEINLINE fltx4 RotateLeft( const fltx4 & a );
FORCEINLINE fltx4 RotateLeft2( const fltx4 & a );

FORCEINLINE void StoreUnaligned3SIMD( float *pSIMD, const fltx4 & a )
{
	_mm_store_ss(pSIMD, a);
	_mm_store_ss(pSIMD+1, RotateLeft(a));
	_mm_store_ss(pSIMD+2, RotateLeft2(a));
}

// strongly typed -- syntactic castor oil used for typechecking as we transition to SIMD
FORCEINLINE void StoreAligned3SIMD( VectorAligned * RESTRICT pSIMD, const fltx4 & a )
{
	StoreAlignedSIMD( pSIMD->Base(),a );
}

FORCEINLINE fltx4 LoadAlignedSIMD( const void *pSIMD )
{
	return _mm_load_ps( reinterpret_cast< const float *> ( pSIMD ) );
}

FORCEINLINE shortx8 LoadAlignedShortSIMD( const void *pSIMD )
{
	return _mm_load_si128( reinterpret_cast< const shortx8 *> ( pSIMD ) );
}

FORCEINLINE shortx8 LoadUnalignedShortSIMD( const void *pSIMD )
{
	return _mm_loadu_si128( reinterpret_cast< const shortx8 *> ( pSIMD ) );
}

FORCEINLINE fltx4 AndSIMD( const fltx4 & a, const fltx4 & b )				// a & b
{
	return _mm_and_ps( a, b );
}

FORCEINLINE fltx4 AndNotSIMD( const fltx4 & a, const fltx4 & b )			// ~a & b
{
	return _mm_andnot_ps( a, b );
}

FORCEINLINE fltx4 XorSIMD( const fltx4 & a, const fltx4 & b )				// a ^ b
{
	return _mm_xor_ps( a, b );
}

FORCEINLINE fltx4 OrSIMD( const fltx4 & a, const fltx4 & b )				// a | b
{
	return _mm_or_ps( a, b );
}

// Squelch the w component of a vector to +0.0.
// Most efficient when you say a = SetWToZeroSIMD(a) (avoids a copy)
FORCEINLINE fltx4 SetWToZeroSIMD( const fltx4 & a )
{
	return AndSIMD( a, LoadAlignedSIMD( g_SIMD_clear_wmask ) );
}

// for the transitional class -- load a 3-by VectorAligned and squash its w component
FORCEINLINE fltx4 LoadAlignedSIMD( const VectorAligned & pSIMD )
{
	return SetWToZeroSIMD( LoadAlignedSIMD(pSIMD.Base()) );
}

NO_ASAN_FORCEINLINE fltx4 LoadUnalignedSIMD( const void *pSIMD )
{
	return _mm_loadu_ps( reinterpret_cast<const float *>( pSIMD ) );
}

NO_ASAN_FORCEINLINE fltx4 LoadUnaligned3SIMD( const void *pSIMD )
{
	return _mm_loadu_ps( reinterpret_cast<const float *>( pSIMD ) );
}

/// replicate a single 32 bit integer value to all 4 components of an m128
FORCEINLINE fltx4 ReplicateIX4( int i )
{
	fltx4 value = _mm_set_ss( * ( ( float *) &i ) );;
	return _mm_shuffle_ps( value, value, 0);
}


FORCEINLINE fltx4 ReplicateX4( float flValue )
{
	__m128 value = _mm_set_ss( flValue );
	return _mm_shuffle_ps( value, value, 0 );
}


FORCEINLINE float SubFloat( const fltx4 & a, int idx )
{
	// NOTE: if the output goes into a register, this causes a Load-Hit-Store stall (don't mix fpu/vpu math!)
#ifndef POSIX
	return a.m128_f32[ idx ];
#else
	return (reinterpret_cast<float const *>(&a))[idx];
#endif
}

FORCEINLINE float & SubFloat( fltx4 & a, int idx )
{
#ifndef POSIX
	return a.m128_f32[ idx ];
#else
	return (reinterpret_cast<float *>(&a))[idx];
#endif
}

FORCEINLINE uint32 SubFloatConvertToInt( const fltx4 & a, int idx )
{
	return (uint32)SubFloat(a,idx);
}

FORCEINLINE uint32 SubInt( const fltx4 & a, int idx )
{
#ifndef POSIX
	return a.m128_u32[idx];
#else
	return (reinterpret_cast<uint32 const *>(&a))[idx];
#endif
}

FORCEINLINE uint32 & SubInt( fltx4 & a, int idx )
{
#ifndef POSIX
	return a.m128_u32[idx];
#else
	return (reinterpret_cast<uint32 *>(&a))[idx];
#endif
}

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE fltx4 LoadZeroSIMD( void )
{
	return Four_Zeros;
}

// Return one in the fastest way -- on the x360, faster even than loading.
FORCEINLINE fltx4 LoadOneSIMD( void )
{
	return Four_Ones;
}

FORCEINLINE fltx4 MaskedAssign( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
	return OrSIMD(
		AndSIMD( ReplacementMask, NewValue ),
		AndNotSIMD( ReplacementMask, OldValue ) );
}

// the same as MaskedAssign, only 5 instructions instead of 6
FORCEINLINE fltx4 SelectSIMD( const fltx4 & ReplacementMask, const fltx4 & NewValue, const fltx4 & OldValue )
{
	return XorSIMD( AndSIMD( XorSIMD( NewValue, OldValue ), ReplacementMask ), OldValue );
}

// remember, the SSE numbers its words 3 2 1 0
// The way we want to specify shuffles is backwards from the default
// MM_SHUFFLE_REV is in array index order (default is reversed)
#define MM_SHUFFLE_REV(a,b,c,d) _MM_SHUFFLE(d,c,b,a)

FORCEINLINE fltx4 SplatXSIMD( fltx4 const & a )
{
	return _mm_shuffle_ps( a, a, MM_SHUFFLE_REV( 0, 0, 0, 0 ) );
}

FORCEINLINE fltx4 SplatYSIMD( fltx4 const &a )
{
	return _mm_shuffle_ps( a, a, MM_SHUFFLE_REV( 1, 1, 1, 1 ) );
}

FORCEINLINE fltx4 SplatZSIMD( fltx4 const &a )
{
	return _mm_shuffle_ps( a, a, MM_SHUFFLE_REV( 2, 2, 2, 2 ) );
}

FORCEINLINE fltx4 SplatWSIMD( fltx4 const &a )
{
	return _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 3, 3, 3 ) );
}

FORCEINLINE fltx4 SetXSIMD( const fltx4& a, const fltx4& x )
{
	fltx4 result = MaskedAssign( LoadAlignedSIMD( g_SIMD_ComponentMask[0] ), x, a );
	return result;
}

FORCEINLINE fltx4 SetYSIMD( const fltx4& a, const fltx4& y )
{
	fltx4 result = MaskedAssign( LoadAlignedSIMD( g_SIMD_ComponentMask[1] ), y, a );
	return result;
}

FORCEINLINE fltx4 SetZSIMD( const fltx4& a, const fltx4& z )
{
	fltx4 result = MaskedAssign( LoadAlignedSIMD( g_SIMD_ComponentMask[2] ), z, a );
	return result;
}

FORCEINLINE fltx4 SetWSIMD( const fltx4& a, const fltx4& w )
{
	fltx4 result = MaskedAssign( LoadAlignedSIMD( g_SIMD_ComponentMask[3] ), w, a );
	return result;
}

FORCEINLINE fltx4 SetComponentSIMD( const fltx4& a, int nComponent, float flValue )
{
	fltx4 val = ReplicateX4( flValue );
	fltx4 result = MaskedAssign( LoadAlignedSIMD( g_SIMD_ComponentMask[nComponent] ), val, a );
	return result;
}

// a b c d -> b c d a
FORCEINLINE fltx4 RotateLeft( const fltx4 & a )
{
	return _mm_shuffle_ps( a, a, MM_SHUFFLE_REV( 1, 2, 3, 0 ) );
}

// a b c d -> c d a b
FORCEINLINE fltx4 RotateLeft2( const fltx4 & a )
{
	return _mm_shuffle_ps( a, a, MM_SHUFFLE_REV( 2, 3, 0, 1 ) );
}

// a b c d -> d a b c
FORCEINLINE fltx4 RotateRight( const fltx4 & a )
{
	return _mm_shuffle_ps( a, a, _MM_SHUFFLE( 0, 3, 2, 1) );
}

// a b c d -> c d a b
FORCEINLINE fltx4 RotateRight2( const fltx4 & a )
{
	return _mm_shuffle_ps( a, a, _MM_SHUFFLE( 1, 0, 3, 2 ) );
}


FORCEINLINE fltx4 AddSIMD( const fltx4 & a, const fltx4 & b )				// a+b
{
	return _mm_add_ps( a, b );
};

FORCEINLINE fltx4 SubSIMD( const fltx4 & a, const fltx4 & b )				// a-b
{
	return _mm_sub_ps( a, b );
};

FORCEINLINE fltx4 MulSIMD( const fltx4 & a, const fltx4 & b )				// a*b
{
	return _mm_mul_ps( a, b );
};

FORCEINLINE fltx4 DivSIMD( const fltx4 & a, const fltx4 & b )				// a/b
{
	return _mm_div_ps( a, b );
};

FORCEINLINE fltx4 MaddSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// a*b + c
{
	return AddSIMD( MulSIMD(a,b), c );
}

FORCEINLINE fltx4 MsubSIMD( const fltx4 & a, const fltx4 & b, const fltx4 & c )				// c - a*b
{
	return SubSIMD( c, MulSIMD(a,b) );
};

FORCEINLINE fltx4 Dot3SIMD( const fltx4 &a, const fltx4 &b )
{
	fltx4 m = MulSIMD( a, b );
	float flDot = SubFloat( m, 0 ) + SubFloat( m, 1 ) + SubFloat( m, 2 );
	return ReplicateX4( flDot );
}

FORCEINLINE fltx4 Dot4SIMD( const fltx4 &a, const fltx4 &b )
{
	fltx4 m = MulSIMD( a, b );
	float flDot = SubFloat( m, 0 ) + SubFloat( m, 1 ) + SubFloat( m, 2 ) + SubFloat( m, 3 );
	return ReplicateX4( flDot );
}

//TODO: implement as four-way Taylor series (see xbox implementation)
FORCEINLINE fltx4 SinSIMD( const fltx4 &radians )
{
	fltx4 result;
	SubFloat( result, 0 ) = sin( SubFloat( radians, 0 ) );
	SubFloat( result, 1 ) = sin( SubFloat( radians, 1 ) );
	SubFloat( result, 2 ) = sin( SubFloat( radians, 2 ) );
	SubFloat( result, 3 ) = sin( SubFloat( radians, 3 ) );
	return result;
}

FORCEINLINE void SinCos3SIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )
{
	// FIXME: Make a fast SSE version
	SinCos( SubFloat( radians, 0 ), &SubFloat( sine, 0 ), &SubFloat( cosine, 0 ) );
	SinCos( SubFloat( radians, 1 ), &SubFloat( sine, 1 ), &SubFloat( cosine, 1 ) );
	SinCos( SubFloat( radians, 2 ), &SubFloat( sine, 2 ), &SubFloat( cosine, 2 ) );
}

FORCEINLINE void SinCosSIMD( fltx4 &sine, fltx4 &cosine, const fltx4 &radians )				// a*b + c
{
	// FIXME: Make a fast SSE version
	SinCos( SubFloat( radians, 0 ), &SubFloat( sine, 0 ), &SubFloat( cosine, 0 ) );
	SinCos( SubFloat( radians, 1 ), &SubFloat( sine, 1 ), &SubFloat( cosine, 1 ) );
	SinCos( SubFloat( radians, 2 ), &SubFloat( sine, 2 ), &SubFloat( cosine, 2 ) );
	SinCos( SubFloat( radians, 3 ), &SubFloat( sine, 3 ), &SubFloat( cosine, 3 ) );
}

//TODO: implement as four-way Taylor series (see xbox implementation)
FORCEINLINE fltx4 ArcSinSIMD( const fltx4 &sine )
{
	// FIXME: Make a fast SSE version
	fltx4 result;
	SubFloat( result, 0 ) = asin( SubFloat( sine, 0 ) );
	SubFloat( result, 1 ) = asin( SubFloat( sine, 1 ) );
	SubFloat( result, 2 ) = asin( SubFloat( sine, 2 ) );
	SubFloat( result, 3 ) = asin( SubFloat( sine, 3 ) );
	return result;
}

FORCEINLINE fltx4 ArcCosSIMD( const fltx4 &cs )
{
	fltx4 result;
	SubFloat( result, 0 ) = acos( SubFloat( cs, 0 ) );
	SubFloat( result, 1 ) = acos( SubFloat( cs, 1 ) );
	SubFloat( result, 2 ) = acos( SubFloat( cs, 2 ) );
	SubFloat( result, 3 ) = acos( SubFloat( cs, 3 ) );
	return result;
}

// tan^1(a/b) .. ie, pass sin in as a and cos in as b
FORCEINLINE fltx4 ArcTan2SIMD( const fltx4 &a, const fltx4 &b )
{
	fltx4 result;
	SubFloat( result, 0 ) = atan2( SubFloat( a, 0 ), SubFloat( b, 0 ) );
	SubFloat( result, 1 ) = atan2( SubFloat( a, 1 ), SubFloat( b, 1 ) );
	SubFloat( result, 2 ) = atan2( SubFloat( a, 2 ), SubFloat( b, 2 ) );
	SubFloat( result, 3 ) = atan2( SubFloat( a, 3 ), SubFloat( b, 3 ) );
	return result;
}

FORCEINLINE fltx4 NegSIMD(const fltx4 &a) // negate: -a
{
	return SubSIMD(LoadZeroSIMD(),a);
}

FORCEINLINE int TestSignSIMD( const fltx4 & a )								// mask of which floats have the high bit set
{
	return _mm_movemask_ps( a );
}

FORCEINLINE bool IsAnyNegative( const fltx4 & a )							// (a.x < 0) || (a.y < 0) || (a.z < 0) || (a.w < 0)
{
	return (0 != TestSignSIMD( a ));
}

FORCEINLINE fltx4 CmpEqSIMD( const fltx4 & a, const fltx4 & b )				// (a==b) ? ~0:0
{
	return _mm_cmpeq_ps( a, b );
}

FORCEINLINE fltx4 CmpGtSIMD( const fltx4 & a, const fltx4 & b )				// (a>b) ? ~0:0
{
	return _mm_cmpgt_ps( a, b );
}

FORCEINLINE fltx4 CmpGeSIMD( const fltx4 & a, const fltx4 & b )				// (a>=b) ? ~0:0
{
	return _mm_cmpge_ps( a, b );
}

FORCEINLINE fltx4 CmpLtSIMD( const fltx4 & a, const fltx4 & b )				// (a<b) ? ~0:0
{
	return _mm_cmplt_ps( a, b );
}

FORCEINLINE fltx4 CmpLeSIMD( const fltx4 & a, const fltx4 & b )				// (a<=b) ? ~0:0
{
	return _mm_cmple_ps( a, b );
}

// for branching when a.xyzw > b.xyzw
FORCEINLINE bool IsAllGreaterThan( const fltx4 &a, const fltx4 &b )
{
	return	TestSignSIMD( CmpLeSIMD( a, b ) ) == 0;
}

// for branching when a.xyzw >= b.xyzw
FORCEINLINE bool IsAllGreaterThanOrEq( const fltx4 &a, const fltx4 &b )
{
	return	TestSignSIMD( CmpLtSIMD( a, b ) ) == 0;
}

// For branching if all a.xyzw == b.xyzw
FORCEINLINE bool IsAllEqual( const fltx4 & a, const fltx4 & b )
{
	return	TestSignSIMD( CmpEqSIMD( a, b ) ) == 0xf;
}

FORCEINLINE fltx4 CmpInBoundsSIMD( const fltx4 & a, const fltx4 & b )		// (a <= b && a >= -b) ? ~0 : 0
{
	return AndSIMD( CmpLeSIMD(a,b), CmpGeSIMD(a, NegSIMD(b)) );
}

FORCEINLINE fltx4 MinSIMD( const fltx4 & a, const fltx4 & b )				// min(a,b)
{
	return _mm_min_ps( a, b );
}

FORCEINLINE fltx4 MaxSIMD( const fltx4 & a, const fltx4 & b )				// max(a,b)
{
	return _mm_max_ps( a, b );
}



// SSE lacks rounding operations. 
// Really.
// You can emulate them by setting the rounding mode for the 
// whole processor and then converting to int, and then back again.
// But every time you set the rounding mode, you clear out the
// entire pipeline. So, I can't do them per operation. You
// have to do it once, before the loop that would call these.
// Round towards positive infinity
FORCEINLINE fltx4 CeilSIMD( const fltx4 &a )
{
	fltx4 retVal;
	SubFloat( retVal, 0 ) = ceil( SubFloat( a, 0 ) );
	SubFloat( retVal, 1 ) = ceil( SubFloat( a, 1 ) );
	SubFloat( retVal, 2 ) = ceil( SubFloat( a, 2 ) );
	SubFloat( retVal, 3 ) = ceil( SubFloat( a, 3 ) );
	return retVal;

}

fltx4 fabs( const fltx4 & x );
// Round towards negative infinity
// This is the implementation that was here before; it assumes
// you are in round-to-floor mode, which I guess is usually the
// case for us vis-a-vis SSE. It's totally unnecessary on 
// VMX, which has a native floor op.
FORCEINLINE fltx4 FloorSIMD( const fltx4 &val )
{
	fltx4 fl4Abs = fabs( val );
	fltx4 ival = SubSIMD( AddSIMD( fl4Abs, Four_2ToThe23s ), Four_2ToThe23s );
	ival = MaskedAssign( CmpGtSIMD( ival, fl4Abs ), SubSIMD( ival, Four_Ones ), ival );
	return XorSIMD( ival, XorSIMD( val, fl4Abs ) );			// restore sign bits
}



inline bool IsAllZeros( const fltx4 & var )
{
	return TestSignSIMD( CmpEqSIMD( var, Four_Zeros ) ) == 0xF;
}

FORCEINLINE fltx4 SqrtEstSIMD( const fltx4 & a )					// sqrt(a), more or less
{
	return _mm_sqrt_ps( a );
}

FORCEINLINE fltx4 SqrtSIMD( const fltx4 & a )						// sqrt(a)
{
	return _mm_sqrt_ps( a );
}

FORCEINLINE fltx4 ReciprocalSqrtEstSIMD( const fltx4 & a )			// 1/sqrt(a), more or less
{
	return _mm_rsqrt_ps( a );
}

FORCEINLINE fltx4 ReciprocalSqrtEstSaturateSIMD( const fltx4 & a )
{
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 ret = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	ret = ReciprocalSqrtEstSIMD( ret );
	return ret;
}

/// uses newton iteration for higher precision results than ReciprocalSqrtEstSIMD
FORCEINLINE fltx4 ReciprocalSqrtSIMD( const fltx4 & a )				// 1/sqrt(a)
{
	fltx4 guess = ReciprocalSqrtEstSIMD( a );
	// newton iteration for 1/sqrt(a) : y(n+1) = 1/2 (y(n)*(3-a*y(n)^2));
	guess = MulSIMD( guess, SubSIMD( Four_Threes, MulSIMD( a, MulSIMD( guess, guess ))));
	guess = MulSIMD( Four_PointFives, guess);
	return guess;
}

FORCEINLINE fltx4 ReciprocalEstSIMD( const fltx4 & a )				// 1/a, more or less
{
	return _mm_rcp_ps( a );
}

/// 1/x for all 4 values, more or less
/// 1/0 will result in a big but NOT infinite result
FORCEINLINE fltx4 ReciprocalEstSaturateSIMD( const fltx4 & a )
{
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 ret = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	ret = ReciprocalEstSIMD( ret );
	return ret;
}

/// 1/x for all 4 values. uses reciprocal approximation instruction plus newton iteration.
/// No error checking!
FORCEINLINE fltx4 ReciprocalSIMD( const fltx4 & a )					// 1/a
{
	fltx4 ret = ReciprocalEstSIMD( a );
	// newton iteration is: Y(n+1) = 2*Y(n)-a*Y(n)^2
	ret = SubSIMD( AddSIMD( ret, ret ), MulSIMD( a, MulSIMD( ret, ret ) ) );
	return ret;
}

/// 1/x for all 4 values.
/// 1/0 will result in a big but NOT infinite result
FORCEINLINE fltx4 ReciprocalSaturateSIMD( const fltx4 & a )
{
	fltx4 zero_mask = CmpEqSIMD( a, Four_Zeros );
	fltx4 ret = OrSIMD( a, AndSIMD( Four_Epsilons, zero_mask ) );
	ret = ReciprocalSIMD( ret );
	return ret;
}

// CHRISG: is it worth doing integer bitfiddling for this?
// 2^x for all values (the antilog)
FORCEINLINE fltx4 ExpSIMD( const fltx4 &toPower )
{
	fltx4 retval;
	SubFloat( retval, 0 ) = powf( 2, SubFloat(toPower, 0) );
	SubFloat( retval, 1 ) = powf( 2, SubFloat(toPower, 1) );
	SubFloat( retval, 2 ) = powf( 2, SubFloat(toPower, 2) );
	SubFloat( retval, 3 ) = powf( 2, SubFloat(toPower, 3) );

	return retval;
}

// Clamps the components of a vector to a specified minimum and maximum range.
FORCEINLINE fltx4 ClampVectorSIMD( FLTX4 in, FLTX4 min, FLTX4 max)
{
	return MaxSIMD( min, MinSIMD( max, in ) );
}

FORCEINLINE void TransposeSIMD( fltx4 & x, fltx4 & y, fltx4 & z, fltx4 & w)
{
	_MM_TRANSPOSE4_PS( x, y, z, w );
}

FORCEINLINE fltx4 FindLowestSIMD3( const fltx4 &a )
{
	// a is [x,y,z,G] (where G is garbage)
	// rotate left by one 
	fltx4 compareOne = RotateLeft( a );
	// compareOne is [y,z,G,x]
	fltx4 retval = MinSIMD( a, compareOne );
	// retVal is [min(x,y), ... ]
	compareOne = RotateLeft2( a );
	// compareOne is [z, G, x, y]
	retval = MinSIMD( retval, compareOne );
	// retVal = [ min(min(x,y),z)..]
	// splat the x component out to the whole vector and return
	return SplatXSIMD( retval );
	
}

FORCEINLINE fltx4 FindHighestSIMD3( const fltx4 &a )
{
	// a is [x,y,z,G] (where G is garbage)
	// rotate left by one 
	fltx4 compareOne = RotateLeft( a );
	// compareOne is [y,z,G,x]
	fltx4 retval = MaxSIMD( a, compareOne );
	// retVal is [max(x,y), ... ]
	compareOne = RotateLeft2( a );
	// compareOne is [z, G, x, y]
	retval = MaxSIMD( retval, compareOne );
	// retVal = [ max(max(x,y),z)..]
	// splat the x component out to the whole vector and return
	return SplatXSIMD( retval );
	
}

// ------------------------------------
// INTEGER SIMD OPERATIONS.
// ------------------------------------


#if 0				/* pc does not have these ops */
// splat all components of a vector to a signed immediate int number.
FORCEINLINE fltx4 IntSetImmediateSIMD(int to)
{
	//CHRISG: SSE2 has this, but not SSE1. What to do?
	fltx4 retval;
	SubInt( retval, 0 ) = to;
	SubInt( retval, 1 ) = to;
	SubInt( retval, 2 ) = to;
	SubInt( retval, 3 ) = to;
	return retval;
}
#endif

// Load 4 aligned words into a SIMD register
FORCEINLINE i32x4 LoadAlignedIntSIMD( const void * RESTRICT pSIMD)
{
	return _mm_load_ps( reinterpret_cast<const float *>(pSIMD) );
}

// Load 4 unaligned words into a SIMD register
FORCEINLINE i32x4 LoadUnalignedIntSIMD( const void * RESTRICT pSIMD)
{
	return _mm_loadu_ps( reinterpret_cast<const float *>(pSIMD) );
}

// save into four words, 16-byte aligned
FORCEINLINE void StoreAlignedIntSIMD( int32 * RESTRICT pSIMD, const fltx4 & a )
{
	_mm_store_ps( reinterpret_cast<float *>(pSIMD), a );
}

FORCEINLINE void StoreAlignedIntSIMD( intx4 &pSIMD, const fltx4 & a )
{
	_mm_store_ps( reinterpret_cast<float *>(pSIMD.Base()), a );
}

FORCEINLINE void StoreUnalignedIntSIMD( int32 * RESTRICT pSIMD, const fltx4 & a )
{
	_mm_storeu_ps( reinterpret_cast<float *>(pSIMD), a );
}

FORCEINLINE fltx4 SignedIntConvertToFltSIMD( const shortx8 &vSrcA )
{
	return _mm_cvtepi32_ps( vSrcA );
}

// CHRISG: the conversion functions all seem to operate on m64's only...
// how do we make them work here?

// Take a fltx4 containing fixed-point uints and 
// return them as single precision floats. No
// fixed point conversion is done.
FORCEINLINE fltx4 UnsignedIntConvertToFltSIMD( const u32x4 &vSrcA )
{
	fltx4 retval;
	SubFloat( retval, 0 ) = ( (float) SubInt( retval, 0 ) );
	SubFloat( retval, 1 ) = ( (float) SubInt( retval, 1 ) );
	SubFloat( retval, 2 ) = ( (float) SubInt( retval, 2 ) );
	SubFloat( retval, 3 ) = ( (float) SubInt( retval, 3 ) );
	return retval;
}


// Take a fltx4 containing fixed-point sints and 
// return them as single precision floats. No 
// fixed point conversion is done.
FORCEINLINE fltx4 SignedIntConvertToFltSIMD( const i32x4 &vSrcA )
{
	fltx4 retval;
	SubFloat( retval, 0 ) = ( (float) (reinterpret_cast<const int32 *>(&vSrcA)[0]));
	SubFloat( retval, 1 ) = ( (float) (reinterpret_cast<const int32 *>(&vSrcA)[1]));
	SubFloat( retval, 2 ) = ( (float) (reinterpret_cast<const int32 *>(&vSrcA)[2]));
	SubFloat( retval, 3 ) = ( (float) (reinterpret_cast<const int32 *>(&vSrcA)[3]));
	return retval;
}

/*
  works on fltx4's as if they are four uints.
  the first parameter contains the words to be shifted,
  the second contains the amount to shift by AS INTS

  for i = 0 to 3
  shift = vSrcB_i*32:(i*32)+4
  vReturned_i*32:(i*32)+31 = vSrcA_i*32:(i*32)+31 << shift
*/
FORCEINLINE i32x4 IntShiftLeftWordSIMD(const i32x4 &vSrcA, const i32x4 &vSrcB)
{
	i32x4 retval;
	SubInt(retval, 0) = SubInt(vSrcA, 0) << SubInt(vSrcB, 0);
	SubInt(retval, 1) = SubInt(vSrcA, 1) << SubInt(vSrcB, 1);
	SubInt(retval, 2) = SubInt(vSrcA, 2) << SubInt(vSrcB, 2);
	SubInt(retval, 3) = SubInt(vSrcA, 3) << SubInt(vSrcB, 3);


	return retval;
}


// Fixed-point conversion and save as SIGNED INTS.
// pDest->x = Int (vSrc.x)
// note: some architectures have means of doing 
// fixed point conversion when the fix depth is
// specified as an immediate.. but there is no way 
// to guarantee an immediate as a parameter to function
// like this.
FORCEINLINE void ConvertStoreAsIntsSIMD(intx4 * RESTRICT pDest, const fltx4 &vSrc)
{
#if defined( COMPILER_MSVC64 )

	(*pDest)[0] = SubFloat( vSrc, 0 );
	(*pDest)[1] = SubFloat( vSrc, 1 );
	(*pDest)[2] = SubFloat( vSrc, 2 );
	(*pDest)[3] = SubFloat( vSrc, 3 );

#else
	__m64 bottom = _mm_cvttps_pi32( vSrc );
	__m64 top    = _mm_cvttps_pi32( _mm_movehl_ps(vSrc,vSrc) );

	*reinterpret_cast<__m64 *>(&(*pDest)[0]) = bottom;
	*reinterpret_cast<__m64 *>(&(*pDest)[2]) = top;

	_mm_empty();
#endif
}



#endif



/// class FourVectors stores 4 independent vectors for use in SIMD processing. These vectors are
/// stored in the format x x x x y y y y z z z z so that they can be efficiently SIMD-accelerated.
class ALIGN16 FourVectors
{
public:
	fltx4 x, y, z;

	FORCEINLINE void DuplicateVector(Vector const &v)			//< set all 4 vectors to the same vector value
	{
		x=ReplicateX4(v.x);
		y=ReplicateX4(v.y);
		z=ReplicateX4(v.z);
	}

	FORCEINLINE fltx4 const & operator[](int idx) const
	{
		return *((&x)+idx);
	}

	FORCEINLINE fltx4 & operator[](int idx)
	{
		return *((&x)+idx);
	}

	FORCEINLINE void operator+=(FourVectors const &b)			//< add 4 vectors to another 4 vectors
	{
		x=AddSIMD(x,b.x);
		y=AddSIMD(y,b.y);
		z=AddSIMD(z,b.z);
	}

	FORCEINLINE void operator-=(FourVectors const &b)			//< subtract 4 vectors from another 4
	{
		x=SubSIMD(x,b.x);
		y=SubSIMD(y,b.y);
		z=SubSIMD(z,b.z);
	}

	FORCEINLINE void operator*=(FourVectors const &b)			//< scale all four vectors per component scale
	{
		x=MulSIMD(x,b.x);
		y=MulSIMD(y,b.y);
		z=MulSIMD(z,b.z);
	}

	FORCEINLINE void operator*=(const fltx4 & scale)			//< scale 
	{
		x=MulSIMD(x,scale);
		y=MulSIMD(y,scale);
		z=MulSIMD(z,scale);
	}

	FORCEINLINE void operator*=(float scale)					//< uniformly scale all 4 vectors
	{
		fltx4 scalepacked = ReplicateX4(scale);
		*this *= scalepacked;
	}

	FORCEINLINE fltx4 operator*(FourVectors const &b) const		//< 4 dot products
	{
		fltx4 dot=MulSIMD(x,b.x);
		dot=MaddSIMD(y,b.y,dot);
		dot=MaddSIMD(z,b.z,dot);
		return dot;
	}

	FORCEINLINE fltx4 operator*(Vector const &b) const			//< dot product all 4 vectors with 1 vector
	{
		fltx4 dot=MulSIMD(x,ReplicateX4(b.x));
		dot=MaddSIMD(y,ReplicateX4(b.y), dot);
		dot=MaddSIMD(z,ReplicateX4(b.z), dot);
		return dot;
	}

	FORCEINLINE void VProduct(FourVectors const &b)				//< component by component mul
	{
		x=MulSIMD(x,b.x);
		y=MulSIMD(y,b.y);
		z=MulSIMD(z,b.z);
	}
	FORCEINLINE void MakeReciprocal(void)						//< (x,y,z)=(1/x,1/y,1/z)
	{
		x=ReciprocalSIMD(x);
		y=ReciprocalSIMD(y);
		z=ReciprocalSIMD(z);
	}

	FORCEINLINE void MakeReciprocalSaturate(void)				//< (x,y,z)=(1/x,1/y,1/z), 1/0=1.0e23
	{
		x=ReciprocalSaturateSIMD(x);
		y=ReciprocalSaturateSIMD(y);
		z=ReciprocalSaturateSIMD(z);
	}

	// Assume the given matrix is a rotation, and rotate these vectors by it.
	// If you have a long list of FourVectors structures that you all want 
	// to rotate by the same matrix, use FourVectors::RotateManyBy() instead.
	inline void RotateBy(const matrix3x4_t& matrix);

	/// You can use this to rotate a long array of FourVectors all by the same
	/// matrix. The first parameter is the head of the array. The second is the
	/// number of vectors to rotate. The third is the matrix.
	static void RotateManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix );

	/// Assume the vectors are points, and transform them in place by the matrix.
	inline void TransformBy(const matrix3x4_t& matrix);

	/// You can use this to Transform a long array of FourVectors all by the same
	/// matrix. The first parameter is the head of the array. The second is the
	/// number of vectors to rotate. The third is the matrix. The fourth is the 
	/// output buffer, which must not overlap the pVectors buffer. This is not 
	/// an in-place transformation.
	static void TransformManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix, FourVectors * RESTRICT pOut );

	/// You can use this to Transform a long array of FourVectors all by the same
	/// matrix. The first parameter is the head of the array. The second is the
	/// number of vectors to rotate. The third is the matrix. The fourth is the 
	/// output buffer, which must not overlap the pVectors buffer. 
	/// This is an in-place transformation.
	static void TransformManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix );

	// X(),Y(),Z() - get at the desired component of the i'th (0..3) vector.
	FORCEINLINE const float & X(int idx) const
	{
		// NOTE: if the output goes into a register, this causes a Load-Hit-Store stall (don't mix fpu/vpu math!)
		return SubFloat( (fltx4 &)x, idx );
	}

	FORCEINLINE const float & Y(int idx) const
	{
		return SubFloat( (fltx4 &)y, idx );
	}

	FORCEINLINE const float & Z(int idx) const
	{
		return SubFloat( (fltx4 &)z, idx );
	}

	FORCEINLINE float & X(int idx)
	{
		return SubFloat( x, idx );
	}

	FORCEINLINE float & Y(int idx)
	{
		return SubFloat( y, idx );
	}

	FORCEINLINE float & Z(int idx)
	{
		return SubFloat( z, idx );
	}

	FORCEINLINE Vector Vec(int idx) const						//< unpack one of the vectors
	{
		return Vector( X(idx), Y(idx), Z(idx) );
	}
	
	FourVectors() = default;

	FourVectors( FourVectors const &src )
	{
		x=src.x;
		y=src.y;
		z=src.z;
	}

	FORCEINLINE void operator=( FourVectors const &src )
	{
		x=src.x;
		y=src.y;
		z=src.z;
	}

	/// LoadAndSwizzle - load 4 Vectors into a FourVectors, performing transpose op
	FORCEINLINE void LoadAndSwizzle(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		// TransposeSIMD has large sub-expressions that the compiler can't eliminate on x360
		// use an unfolded implementation here
#if _X360
		fltx4 tx = LoadUnalignedSIMD( &a.x );
		fltx4 ty = LoadUnalignedSIMD( &b.x );
		fltx4 tz = LoadUnalignedSIMD( &c.x );
		fltx4 tw = LoadUnalignedSIMD( &d.x );
		fltx4 r0 = __vmrghw(tx, tz);
		fltx4 r1 = __vmrghw(ty, tw);
		fltx4 r2 = __vmrglw(tx, tz);
		fltx4 r3 = __vmrglw(ty, tw);

		x = __vmrghw(r0, r1);
		y = __vmrglw(r0, r1);
		z = __vmrghw(r2, r3);
#else
		x		= LoadUnalignedSIMD( &( a.x ));
		y		= LoadUnalignedSIMD( &( b.x ));
		z		= LoadUnalignedSIMD( &( c.x ));
		fltx4 w = LoadUnalignedSIMD( &( d.x ));
		// now, matrix is:
		// x y z ?
		// x y z ?
		// x y z ?
		// x y z ?
		TransposeSIMD(x, y, z, w);
#endif
	}

	/// LoadAndSwizzleAligned - load 4 Vectors into a FourVectors, performing transpose op.
	/// all 4 vectors must be 128 bit boundary
	FORCEINLINE void LoadAndSwizzleAligned(const float *RESTRICT a, const float *RESTRICT b, const float *RESTRICT c, const float *RESTRICT d)
	{
#if _X360
		fltx4 tx = LoadAlignedSIMD(a);
		fltx4 ty = LoadAlignedSIMD(b);
		fltx4 tz = LoadAlignedSIMD(c);
		fltx4 tw = LoadAlignedSIMD(d);
		fltx4 r0 = __vmrghw(tx, tz);
		fltx4 r1 = __vmrghw(ty, tw);
		fltx4 r2 = __vmrglw(tx, tz);
		fltx4 r3 = __vmrglw(ty, tw);

		x = __vmrghw(r0, r1);
		y = __vmrglw(r0, r1);
		z = __vmrghw(r2, r3);
#else
		x		= LoadAlignedSIMD( a );
		y		= LoadAlignedSIMD( b );
		z		= LoadAlignedSIMD( c );
		fltx4 w = LoadAlignedSIMD( d );
		// now, matrix is:
		// x y z ?
		// x y z ?
		// x y z ?
		// x y z ?
		TransposeSIMD( x, y, z, w );
#endif
	}

	FORCEINLINE void LoadAndSwizzleAligned(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		LoadAndSwizzleAligned( &a.x, &b.x, &c.x, &d.x );
	}

	/// return the squared length of all 4 vectors
	FORCEINLINE fltx4 length2(void) const
	{
		return (*this)*(*this);
	}

	/// return the approximate length of all 4 vectors. uses the sqrt approximation instruction
	FORCEINLINE fltx4 length(void) const
	{
		return SqrtEstSIMD(length2());
	}

	/// normalize all 4 vectors in place. not mega-accurate (uses reciprocal approximation instruction)
	FORCEINLINE void VectorNormalizeFast(void)
	{
		fltx4 mag_sq=(*this)*(*this);						// length^2
		(*this) *= ReciprocalSqrtEstSIMD(mag_sq);			// *(1.0/sqrt(length^2))
	}

	/// normalize all 4 vectors in place.
	FORCEINLINE void VectorNormalize(void)
	{
		fltx4 mag_sq=(*this)*(*this);						// length^2
		(*this) *= ReciprocalSqrtSIMD(mag_sq);				// *(1.0/sqrt(length^2))
	}

	/// construct a FourVectors from 4 separate Vectors
	FORCEINLINE FourVectors(Vector const &a, Vector const &b, Vector const &c, Vector const &d)
	{
		LoadAndSwizzle(a,b,c,d);
	}

	/// construct a FourVectors from 4 separate Vectors
	FORCEINLINE FourVectors(VectorAligned const &a, VectorAligned const &b, VectorAligned const &c, VectorAligned const &d)
	{
		LoadAndSwizzleAligned(a,b,c,d);
	}

	FORCEINLINE fltx4 DistToSqr( FourVectors const &pnt )
	{
		fltx4 fl4dX = SubSIMD( pnt.x, x );
		fltx4 fl4dY = SubSIMD( pnt.y, y );
		fltx4 fl4dZ = SubSIMD( pnt.z, z );
		return AddSIMD( MulSIMD( fl4dX, fl4dX), AddSIMD( MulSIMD( fl4dY, fl4dY ), MulSIMD( fl4dZ, fl4dZ ) ) );

	}
	
	FORCEINLINE fltx4 TValueOfClosestPointOnLine( FourVectors const &p0, FourVectors const &p1 ) const
	{
		FourVectors lineDelta = p1;
		lineDelta -= p0;
		fltx4 OOlineDirDotlineDir = ReciprocalSIMD( p1 * p1 );
		FourVectors v4OurPnt = *this;
		v4OurPnt -= p0;
		return MulSIMD( OOlineDirDotlineDir, v4OurPnt * lineDelta );
	}

	FORCEINLINE fltx4 DistSqrToLineSegment( FourVectors const &p0, FourVectors const &p1 ) const
	{
		FourVectors lineDelta = p1;
		FourVectors v4OurPnt = *this;
		v4OurPnt -= p0;
		lineDelta -= p0;

		fltx4 OOlineDirDotlineDir = ReciprocalSIMD( lineDelta * lineDelta );

		fltx4 fl4T = MulSIMD( OOlineDirDotlineDir, v4OurPnt * lineDelta );

		fl4T = MinSIMD( fl4T, Four_Ones );
		fl4T = MaxSIMD( fl4T, Four_Zeros );
		lineDelta *= fl4T;
		return v4OurPnt.DistToSqr( lineDelta );
	}

	FORCEINLINE FourVectors NormalizedSafeX() const
	{
		fltx4 f4LenSqr = length2();
		fltx4 isBigEnough = CmpGeSIMD( f4LenSqr, Four_Epsilons );
		fltx4 fl4LengthInv = ReciprocalSqrtSIMD( f4LenSqr );
		FourVectors out;
		out.x = SelectSIMD( isBigEnough, MulSIMD( x, fl4LengthInv ), Four_Ones );
		out.y = AndSIMD( MulSIMD( y, fl4LengthInv ), isBigEnough );
		out.z = AndSIMD( MulSIMD( z, fl4LengthInv ), isBigEnough );
		return out;
	}

	FORCEINLINE FourVectors NormalizedSafeY() const
	{
		fltx4 f4LenSqr = length2();
		fltx4 isBigEnough = CmpGeSIMD( f4LenSqr, Four_Epsilons );
		fltx4 fl4LengthInv = ReciprocalSqrtSIMD( f4LenSqr );
		FourVectors out;
		out.x = AndSIMD( MulSIMD( x, fl4LengthInv ), isBigEnough );
		out.y = SelectSIMD( isBigEnough, MulSIMD( y, fl4LengthInv ), Four_Ones );
		out.z = AndSIMD( MulSIMD( z, fl4LengthInv ), isBigEnough );
		return out;
	}

	FORCEINLINE FourVectors NormalizedSafeZ() const
	{
		fltx4 f4LenSqr = length2();
		fltx4 isBigEnough = CmpGeSIMD( f4LenSqr, Four_Epsilons );
		fltx4 fl4LengthInv = ReciprocalSqrtSIMD( f4LenSqr );
		FourVectors out;
		out.x = AndSIMD( MulSIMD( x, fl4LengthInv ), isBigEnough );
		out.y = AndSIMD( MulSIMD( y, fl4LengthInv ), isBigEnough );
		out.z = SelectSIMD( isBigEnough, MulSIMD( z, fl4LengthInv ), Four_Ones );
		return out;
	}
};

/// form 4 cross products
inline FourVectors operator ^(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=SubSIMD(MulSIMD(a.y,b.z),MulSIMD(a.z,b.y));
	ret.y=SubSIMD(MulSIMD(a.z,b.x),MulSIMD(a.x,b.z));
	ret.z=SubSIMD(MulSIMD(a.x,b.y),MulSIMD(a.y,b.x));
	return ret;
}

/// component-by-componentwise MAX operator
inline FourVectors maximum(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=MaxSIMD(a.x,b.x);
	ret.y=MaxSIMD(a.y,b.y);
	ret.z=MaxSIMD(a.z,b.z);
	return ret;
}

/// component-by-componentwise MIN operator
inline FourVectors minimum(const FourVectors &a, const FourVectors &b)
{
	FourVectors ret;
	ret.x=MinSIMD(a.x,b.x);
	ret.y=MinSIMD(a.y,b.y);
	ret.z=MinSIMD(a.z,b.z);
	return ret;
}

/// calculate reflection vector. incident and normal dir assumed normalized
FORCEINLINE FourVectors VectorReflect( const FourVectors &incident, const FourVectors &normal )
{
	FourVectors ret = incident;
	fltx4 iDotNx2 = incident * normal;
	iDotNx2 = AddSIMD( iDotNx2, iDotNx2 );
	FourVectors nPart = normal;
	nPart *= iDotNx2;
	ret -= nPart;											// i-2(n*i)n
	return ret;
}

/// calculate slide vector. removes all components of a vector which are perpendicular to a normal vector.
FORCEINLINE FourVectors VectorSlide( const FourVectors &incident, const FourVectors &normal )
{
	FourVectors ret = incident;
	fltx4 iDotN = incident * normal;
	FourVectors nPart = normal;
	nPart *= iDotN;
	ret -= nPart;											// i-(n*i)n
	return ret;
}


// Assume the given matrix is a rotation, and rotate these vectors by it.
// If you have a long list of FourVectors structures that you all want 
// to rotate by the same matrix, use FourVectors::RotateManyBy() instead.
void FourVectors::RotateBy(const matrix3x4_t& matrix)
{
	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02,
		matSplat10, matSplat11, matSplat12,
		matSplat20, matSplat21, matSplat22;

 {
	// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
	// often unaligned. The w components will be the tranpose row of
	// the matrix, but we don't really care about that.
	fltx4 matCol0 = LoadUnalignedSIMD( matrix[0] );
	fltx4 matCol1 = LoadUnalignedSIMD( matrix[1] );
	fltx4 matCol2 = LoadUnalignedSIMD( matrix[2] );
	
	matSplat00 = SplatXSIMD( matCol0 );
	matSplat01 = SplatYSIMD( matCol0 );
	matSplat02 = SplatZSIMD( matCol0 );

	matSplat10 = SplatXSIMD( matCol1 );
	matSplat11 = SplatYSIMD( matCol1 );
	matSplat12 = SplatZSIMD( matCol1 );
	
	matSplat20 = SplatXSIMD( matCol2 );
	matSplat21 = SplatYSIMD( matCol2 );
	matSplat22 = SplatZSIMD( matCol2 );
 }

	// Trust in the compiler to schedule these operations correctly:
	fltx4 outX, outY, outZ;
	outX = AddSIMD( AddSIMD( MulSIMD( x, matSplat00 ), MulSIMD( y, matSplat01 ) ), MulSIMD( z, matSplat02 ) );
	outY = AddSIMD( AddSIMD( MulSIMD( x, matSplat10 ), MulSIMD( y, matSplat11 ) ), MulSIMD( z, matSplat12 ) );
	outZ = AddSIMD( AddSIMD( MulSIMD( x, matSplat20 ), MulSIMD( y, matSplat21 ) ), MulSIMD( z, matSplat22 ) );
	
	x = outX;
	y = outY;
	z = outZ;
}

// Assume the given matrix is a rotation, and rotate these vectors by it.
// If you have a long list of FourVectors structures that you all want 
// to rotate by the same matrix, use FourVectors::RotateManyBy() instead.
void FourVectors::TransformBy(const matrix3x4_t& matrix)
{
	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02,
		matSplat10, matSplat11, matSplat12,
		matSplat20, matSplat21, matSplat22;

 {
	// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
	// often unaligned. The w components will be the tranpose row of
	// the matrix, but we don't really care about that.
	fltx4 matCol0 = LoadUnalignedSIMD( matrix[0] );
	fltx4 matCol1 = LoadUnalignedSIMD( matrix[1] );
	fltx4 matCol2 = LoadUnalignedSIMD( matrix[2] );
	
	matSplat00 = SplatXSIMD( matCol0 );
	matSplat01 = SplatYSIMD( matCol0 );
	matSplat02 = SplatZSIMD( matCol0 );
	
	matSplat10 = SplatXSIMD( matCol1 );
	matSplat11 = SplatYSIMD( matCol1 );
	matSplat12 = SplatZSIMD( matCol1 );
	
	matSplat20 = SplatXSIMD( matCol2 );
	matSplat21 = SplatYSIMD( matCol2 );
	matSplat22 = SplatZSIMD( matCol2 );
 }
	
	// Trust in the compiler to schedule these operations correctly:
	fltx4 outX, outY, outZ;
	
	outX = MaddSIMD( z, matSplat02, AddSIMD( MulSIMD( x, matSplat00 ), MulSIMD( y, matSplat01 ) ) );
	outY = MaddSIMD( z, matSplat12, AddSIMD( MulSIMD( x, matSplat10 ), MulSIMD( y, matSplat11 ) ) );
	outZ = MaddSIMD( z, matSplat22, AddSIMD( MulSIMD( x, matSplat20 ), MulSIMD( y, matSplat21 ) ) );
	
	x = AddSIMD( outX, ReplicateX4( matrix[0][3] ));
	y = AddSIMD( outY, ReplicateX4( matrix[1][3] ));
	 z = AddSIMD( outZ, ReplicateX4( matrix[2][3] ));
}



/// quick, low quality perlin-style noise() function suitable for real time use.
/// return value is -1..1. Only reliable around +/- 1 million or so.
fltx4 NoiseSIMD( const fltx4 & x, const fltx4 & y, const fltx4 & z );
fltx4 NoiseSIMD( FourVectors const &v );

// vector valued noise direction
FourVectors DNoiseSIMD( FourVectors const &v );

// vector value "curl" noise function. see http://hyperphysics.phy-astr.gsu.edu/hbase/curl.html
FourVectors CurlNoiseSIMD( FourVectors const &v );


/// calculate the absolute value of a packed single
inline fltx4 fabs( const fltx4 & x )
{
	return AndSIMD( x, LoadAlignedSIMD( g_SIMD_clear_signmask ) );
}

/// negate all four components of a SIMD packed single
inline fltx4 fnegate( const fltx4 & x )
{
	return XorSIMD( x, LoadAlignedSIMD( g_SIMD_signmask ) );
}


fltx4 Pow_FixedPoint_Exponent_SIMD( const fltx4 & x, int exponent);

// PowSIMD - raise a SIMD register to a power.  This is analogous to the C pow() function, with some
// restictions: fractional exponents are only handled with 2 bits of precision. Basically,
// fractions of 0,.25,.5, and .75 are handled. PowSIMD(x,.30) will be the same as PowSIMD(x,.25).
// negative and fractional powers are handled by the SIMD reciprocal and square root approximation
// instructions and so are not especially accurate ----Note that this routine does not raise
// numeric exceptions because it uses SIMD--- This routine is O(log2(exponent)).
inline fltx4 PowSIMD( const fltx4 & x, float exponent )
{
	return Pow_FixedPoint_Exponent_SIMD(x,(int) (4.0*exponent));
}



// random number generation - generate 4 random numbers quickly.

void SeedRandSIMD(uint32 seed);								// seed the random # generator
fltx4 RandSIMD( int nContext = 0 );							// return 4 numbers in the 0..1 range

// for multithreaded, you need to use these and use the argument form of RandSIMD:
int GetSIMDRandContext( void );
void ReleaseSIMDRandContext( int nContext );

FORCEINLINE fltx4 RandSignedSIMD( void )					// -1..1
{
	return SubSIMD( MulSIMD( Four_Twos, RandSIMD() ), Four_Ones );
}


// SIMD versions of mathlib simplespline functions
// hermite basis function for smooth interpolation
// Similar to Gain() above, but very cheap to call
// value should be between 0 & 1 inclusive
inline fltx4 SimpleSpline( const fltx4 & value )
{
	// Arranged to avoid a data dependency between these two MULs:
	fltx4 valueDoubled = MulSIMD( value, Four_Twos );
	fltx4 valueSquared = MulSIMD( value, value );

	// Nice little ease-in, ease-out spline-like curve
	return SubSIMD(
		MulSIMD( Four_Threes,  valueSquared ),
		MulSIMD( valueDoubled, valueSquared ) );
}

// remaps a value in [startInterval, startInterval+rangeInterval] from linear to
// spline using SimpleSpline
inline fltx4 SimpleSplineRemapValWithDeltas( const fltx4 & val,
											 const fltx4 & A, const fltx4 & BMinusA,
											 const fltx4 & OneOverBMinusA, const fltx4 & C, 
											 const fltx4 & DMinusC )
{
// 	if ( A == B )
// 		return val >= B ? D : C;
	fltx4 cVal = MulSIMD( SubSIMD( val, A), OneOverBMinusA );
	return AddSIMD( C, MulSIMD( DMinusC, SimpleSpline( cVal ) ) );
}

inline fltx4 SimpleSplineRemapValWithDeltasClamped( const fltx4 & val,
													const fltx4 & A, const fltx4 & BMinusA,
													const fltx4 & OneOverBMinusA, const fltx4 & C, 
													const fltx4 & DMinusC )
{
// 	if ( A == B )
// 		return val >= B ? D : C;
	fltx4 cVal = MulSIMD( SubSIMD( val, A), OneOverBMinusA );
	cVal = MinSIMD( Four_Ones, MaxSIMD( Four_Zeros, cVal ) );
	return AddSIMD( C, MulSIMD( DMinusC, SimpleSpline( cVal ) ) );
}

FORCEINLINE fltx4 FracSIMD( const fltx4 &val )
{
	fltx4 fl4Abs = fabs( val );
	fltx4 ival = SubSIMD( AddSIMD( fl4Abs, Four_2ToThe23s ), Four_2ToThe23s );
	ival = MaskedAssign( CmpGtSIMD( ival, fl4Abs ), SubSIMD( ival, Four_Ones ), ival );
	return XorSIMD( SubSIMD( fl4Abs, ival ), XorSIMD( val, fl4Abs ) );			// restore sign bits
}

FORCEINLINE fltx4 Mod2SIMD( const fltx4 &val )
{
	fltx4 fl4Abs = fabs( val );
	fltx4 ival = SubSIMD( AndSIMD( LoadAlignedSIMD( (float *) g_SIMD_lsbmask ), AddSIMD( fl4Abs, Four_2ToThe23s ) ), Four_2ToThe23s );
	ival = MaskedAssign( CmpGtSIMD( ival, fl4Abs ), SubSIMD( ival, Four_Twos ), ival );
	return XorSIMD( SubSIMD( fl4Abs, ival ), XorSIMD( val, fl4Abs ) );			// restore sign bits
}

FORCEINLINE fltx4 Mod2SIMDPositiveInput( const fltx4 &val )
{
	fltx4 ival = SubSIMD( AndSIMD( LoadAlignedSIMD( g_SIMD_lsbmask ), AddSIMD( val, Four_2ToThe23s ) ), Four_2ToThe23s );
	ival = MaskedAssign( CmpGtSIMD( ival, val ), SubSIMD( ival, Four_Twos ), ival );
	return SubSIMD( val, ival );
}


// approximate sin of an angle, with -1..1 representing the whole sin wave period instead of -pi..pi.
// no range reduction is done - for values outside of 0..1 you won't like the results
FORCEINLINE fltx4 _SinEst01SIMD( const fltx4 &val )
{
	// really rough approximation - x*(4-x*4) - a parabola. s(0) = 0, s(.5) = 1, s(1)=0, smooth in-between.
	// sufficient for simple oscillation.
	return MulSIMD( val, SubSIMD( Four_Fours, MulSIMD( val, Four_Fours ) ) );
}

FORCEINLINE fltx4 _Sin01SIMD( const fltx4 &val )
{
	// not a bad approximation : parabola always over-estimates. Squared parabola always
	// underestimates. So lets blend between them:  goodsin = badsin + .225*( badsin^2-badsin)
	fltx4 fl4BadEst = MulSIMD( val, SubSIMD( Four_Fours, MulSIMD( val, Four_Fours ) ) );
	return AddSIMD( MulSIMD( Four_Point225s, SubSIMD( MulSIMD( fl4BadEst, fl4BadEst ), fl4BadEst ) ), fl4BadEst );
}

// full range useable implementations
FORCEINLINE fltx4 SinEst01SIMD( const fltx4 &val )
{
	fltx4 fl4Abs = fabs( val );
	fltx4 fl4Reduced2 = Mod2SIMDPositiveInput( fl4Abs );
	fltx4 fl4OddMask = CmpGeSIMD( fl4Reduced2, Four_Ones );
	fltx4 fl4val = SubSIMD( fl4Reduced2, AndSIMD( Four_Ones, fl4OddMask ) );
	fltx4 fl4Sin = _SinEst01SIMD( fl4val );
	fl4Sin = XorSIMD( fl4Sin, AndSIMD( LoadAlignedSIMD( g_SIMD_signmask ), XorSIMD( val, fl4OddMask ) ) );
	return fl4Sin;

}

FORCEINLINE fltx4 Sin01SIMD( const fltx4 &val )
{
	fltx4 fl4Abs = fabs( val );
	fltx4 fl4Reduced2 = Mod2SIMDPositiveInput( fl4Abs );
	fltx4 fl4OddMask = CmpGeSIMD( fl4Reduced2, Four_Ones );
	fltx4 fl4val = SubSIMD( fl4Reduced2, AndSIMD( Four_Ones, fl4OddMask ) );
	fltx4 fl4Sin = _Sin01SIMD( fl4val );
	fl4Sin = XorSIMD( fl4Sin, AndSIMD( LoadAlignedSIMD( g_SIMD_signmask ), XorSIMD( val, fl4OddMask ) ) );
	return fl4Sin;

}

// Schlick style Bias approximation see graphics gems 4 : bias(t,a)= t/( (1/a-2)*(1-t)+1)
 
FORCEINLINE fltx4 PreCalcBiasParameter( const fltx4 &bias_parameter )
{
	// convert perlin-style-bias parameter to the value right for the approximation
	return SubSIMD( ReciprocalSIMD( bias_parameter ), Four_Twos );
}

FORCEINLINE fltx4 BiasSIMD( const fltx4 &val, const fltx4 &precalc_param )
{
	// similar to bias function except pass precalced bias value from calling PreCalcBiasParameter.

	//!!speed!! use reciprocal est?
	//!!speed!! could save one op by precalcing _2_ values
	return DivSIMD( val, AddSIMD( MulSIMD( precalc_param, SubSIMD( Four_Ones, val ) ), Four_Ones ) );
}

FORCEINLINE fltx4 LoadUnalignedFloatSIMD( const float *pFlt )
{
	return _mm_load_ss(pFlt);
}

inline const fltx4 Length3SIMD( const fltx4 vec )
{
	fltx4 scLengthSqr = Dot3SIMD( vec, vec );
	bi32x4 isSignificant = CmpGtSIMD( scLengthSqr, Four_Epsilons );
	fltx4 scLengthInv = ReciprocalSqrtSIMD( scLengthSqr );
	return AndSIMD( isSignificant, MulSIMD( scLengthInv, scLengthSqr ) );
}

inline const fltx4 Normalized3SIMD (const fltx4 vec)
{
	fltx4 scLengthSqr = Dot3SIMD(vec,vec);
	bi32x4 isSignificant = CmpGtSIMD(scLengthSqr, Four_Epsilons);
	fltx4 scLengthInv = ReciprocalSqrtSIMD(scLengthSqr);
	return AndSIMD(isSignificant, MulSIMD(vec, scLengthInv));
}

//-----------------------------------------------------------------------------
// Box/plane test 
// NOTE: The w component of emins + emaxs must be 1 for this to work
//-----------------------------------------------------------------------------
FORCEINLINE int BoxOnPlaneSideSIMD( const fltx4& emins, const fltx4& emaxs, const cplane_t *p, float tolerance = 0.f )
{
	fltx4 corners[2];
	fltx4 normal = LoadUnalignedSIMD( p->normal.Base() );
	fltx4 dist = ReplicateX4( -p->dist );
	normal = SetWSIMD( normal, dist );
	fltx4 t4 = ReplicateX4( tolerance );
	fltx4 negt4 = ReplicateX4( -tolerance );
	fltx4 cmp = CmpGeSIMD( normal, Four_Zeros );
	corners[0] = MaskedAssign( cmp, emaxs, emins );
	corners[1] = MaskedAssign( cmp, emins, emaxs );
	fltx4 dot1 = Dot4SIMD( normal, corners[0] );
	fltx4 dot2 = Dot4SIMD( normal, corners[1] );
	cmp = CmpGeSIMD( dot1, t4 );
	fltx4 cmp2 = CmpGtSIMD( negt4, dot2 );
	fltx4 result = MaskedAssign( cmp, Four_Ones, Four_Zeros );
	fltx4 result2 = MaskedAssign( cmp2, Four_Twos, Four_Zeros );
	result = AddSIMD( result, result2 );
	intx4 sides;
	ConvertStoreAsIntsSIMD( &sides, result );
	return sides[0];
}

#endif // _ssemath_h
