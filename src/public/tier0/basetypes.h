//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASETYPES_H
#define BASETYPES_H

#include "commonmacros.h"
#include "wchartypes.h"

#include "tier0/valve_off.h"

#ifdef _WIN32
#pragma once
#endif


// This is a trick to get the DLL extension off the -D option on the command line.
#define DLLExtTokenPaste(x) #x
#define DLLExtTokenPaste2(x) DLLExtTokenPaste(x)
#define DLL_EXT_STRING DLLExtTokenPaste2( _DLL_EXT )


#include "protected_things.h"

// There's a different version of this file in the xbox codeline
// so the PC version built in the xbox branch includes things like 
// tickrate changes.
#include "xbox_codeline_defines.h"

#ifdef IN_XBOX_CODELINE
#define XBOX_CODELINE_ONLY()
#else
#define XBOX_CODELINE_ONLY() Error_Compiling_Code_Only_Valid_in_Xbox_Codeline
#endif

// stdio.h
#ifndef NULL
#define NULL 0
#endif

#include <stdint.h>

#define ExecuteNTimes( nTimes, x )	\
	{								\
		static int __executeCount=0;\
		if ( __executeCount < nTimes )\
		{							\
			x;						\
			++__executeCount;		\
		}							\
	}


#define ExecuteOnce( x )			ExecuteNTimes( 1, x )


template <typename T>
inline T AlignValue( T val, uintptr_t alignment )
{
	return (T)( ( (uintptr_t)val + alignment - 1 ) & ~( alignment - 1 ) );
}


// Pad a number so it lies on an N byte boundary.
// So PAD_NUMBER(0,4) is 0 and PAD_NUMBER(1,4) is 4
#define PAD_NUMBER(number, boundary) \
	( ((number) + ((boundary)-1)) / (boundary) ) * (boundary)

// In case this ever changes
#if !defined(M_PI) && !defined(HAVE_M_PI)
#define M_PI			3.14159265358979323846
#endif

#include "valve_minmax_on.h"

// #define COMPILETIME_MAX and COMPILETIME_MIN for max/min in constant expressions
#define COMPILETIME_MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define COMPILETIME_MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef MAX
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#ifdef __cplusplus

// This is the preferred clamp operator. Using the clamp macro can lead to
// unexpected side-effects or more expensive code. Even the clamp (all
// lower-case) function can generate more expensive code because of the
// mixed types involved.
template< class T >
T Clamp( T const &val, T const &minVal, T const &maxVal )
{
	if( val < minVal )
		return minVal;
	else if( val > maxVal )
		return maxVal;
	else
		return val;
}

// This is the preferred Min operator. Using the MIN macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Min( T const &val1, T const &val2 )
{
	return val1 < val2 ? val1 : val2;
}

// This is the preferred Max operator. Using the MAX macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Max( T const &val1, T const &val2 )
{
	return val1 > val2 ? val1 : val2;
}

#endif

#ifndef USE_DXVK_NATIVE
#ifndef DONT_DEFINE_BOOL // Needed for Cocoa stuff to compile.
typedef int BOOL;
#endif
typedef unsigned char BYTE;
#else
#include <windows.h>
#endif
typedef int qboolean;
typedef unsigned char byte;
typedef unsigned short word;

#ifdef _WIN32
typedef wchar_t ucs2; // under windows wchar_t is ucs2
#else
typedef unsigned short ucs2;
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#ifdef POSIX
#include <stdint.h>
#endif

enum ThreeState_t
{
	TRS_FALSE,
	TRS_TRUE,
	TRS_NONE,
};

typedef float vec_t;

#if defined(__GNUC__)
#define fpmin __builtin_fminf
#define fpmax __builtin_fmaxf
#elif !defined(_X360)
static inline float fpmin( float a, float b )
{
	return a > b  ? b : a;
}

static inline float fpmax( float a, float b )
{
	return a >= b ? a : b;
}
#endif


//-----------------------------------------------------------------------------
// look for NANs, infinities, and underflows. 
// This assumes the ANSI/IEEE 754-1985 standard
//-----------------------------------------------------------------------------

inline unsigned int& FloatBits( vec_t& f )
{
	return *reinterpret_cast<unsigned int*>(&f);
}

inline unsigned int const& FloatBits( vec_t const& f )
{
	return *reinterpret_cast<unsigned int const*>(&f);
}

inline vec_t BitsToFloat( unsigned int i )
{
	return *reinterpret_cast<vec_t*>(&i);
}

inline bool IsFinite( vec_t f )
{
	return ((FloatBits(f) & 0x7F800000) != 0x7F800000);
}

inline unsigned int FloatAbsBits( vec_t f )
{
	return FloatBits(f) & 0x7FFFFFFF;
}

// Given today's processors, I cannot think of any circumstance
// where bit tricks would be faster than fabs. henryg 8/16/2011
#ifdef _MSC_VER
#ifndef _In_
#define _In_
#endif
extern "C" _Check_return_ __inline float __CRTDECL fabsf(_In_ float _X);
#else
#include <math.h>
#endif

inline float FloatMakeNegative( vec_t f )
{
	return -fabsf(f);
}

inline float FloatMakePositive( vec_t f )
{
	return fabsf(f);
}

inline float FloatNegate( vec_t f )
{
	return -f;
}


#define FLOAT32_NAN_BITS     (unsigned long)0x7FC00000	// not a number!
#define FLOAT32_NAN          BitsToFloat( FLOAT32_NAN_BITS )

#define VEC_T_NAN FLOAT32_NAN



// FIXME: why are these here?  Hardly anyone actually needs them.
struct color24
{
	byte r, g, b;
};

typedef struct color32_s
{
	bool operator!=( const struct color32_s &other ) const;

	byte r, g, b, a;
} color32;

inline bool color32::operator!=( const color32 &other ) const
{
	return r != other.r || g != other.g || b != other.b || a != other.a;
}

struct colorVec
{
	unsigned r, g, b, a;
};


#ifndef NOTE_UNUSED
#define NOTE_UNUSED(x)	(void)(x)	// for pesky compiler / lint warnings
#endif

struct vrect_t
{
	int				x,y,width,height;
	vrect_t			*pnext;
};


//-----------------------------------------------------------------------------
// MaterialRect_t struct - used for DrawDebugText
//-----------------------------------------------------------------------------
struct Rect_t
{
    int x, y;
	int width, height;
};


//-----------------------------------------------------------------------------
// Interval, used by soundemittersystem + the game
//-----------------------------------------------------------------------------
struct interval_t
{
	float start;
	float range;
};


//-----------------------------------------------------------------------------
// Declares a type-safe handle type; you can't assign one handle to the next
//-----------------------------------------------------------------------------

// 32-bit pointer handles.

// Typesafe 8-bit and 16-bit handles.
template< class HandleType >
class CBaseIntHandle
{
public:
	
	inline bool			operator==( const CBaseIntHandle &other )	{ return m_Handle == other.m_Handle; }
	inline bool			operator!=( const CBaseIntHandle &other )	{ return m_Handle != other.m_Handle; }

	// Only the code that doles out these handles should use these functions.
	// Everyone else should treat them as a transparent type.
	inline HandleType	GetHandleValue()					{ return m_Handle; }
	inline void			SetHandleValue( HandleType val )	{ m_Handle = val; }

	typedef HandleType	HANDLE_TYPE;

protected:

	HandleType	m_Handle;
};

template< class DummyType >
class CIntHandle16 : public CBaseIntHandle< unsigned short >
{
public:
	inline			CIntHandle16() {}

	static inline	CIntHandle16<DummyType> MakeHandle( HANDLE_TYPE val )
	{
		return CIntHandle16<DummyType>( val );
	}

protected:
	inline			CIntHandle16( HANDLE_TYPE val )
	{
		m_Handle = val;
	}
};


template< class DummyType >
class CIntHandle32 : public CBaseIntHandle< unsigned long >
{
public:
	inline			CIntHandle32() {}

	static inline	CIntHandle32<DummyType> MakeHandle( HANDLE_TYPE val )
	{
		return CIntHandle32<DummyType>( val );
	}

protected:
	inline			CIntHandle32( HANDLE_TYPE val )
	{
		m_Handle = val;
	}
};


// NOTE: This macro is the same as windows uses; so don't change the guts of it
#define DECLARE_HANDLE_16BIT(name)	typedef CIntHandle16< struct name##__handle * > name;
#define DECLARE_HANDLE_32BIT(name)	typedef CIntHandle32< struct name##__handle * > name;

#define DECLARE_POINTER_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name

// @TODO: Find a better home for this
#if !defined(_STATIC_LINKED) && !defined(PUBLISH_DLL_SUBSYSTEM)
// for platforms built with dynamic linking, the dll interface does not need spoofing
#define PUBLISH_DLL_SUBSYSTEM()
#endif

#define UID_PREFIX generated_id_
#define UID_CAT1(a,c) a ## c
#define UID_CAT2(a,c) UID_CAT1(a,c)
#define EXPAND_CONCAT(a,c) UID_CAT1(a,c)
#ifdef _MSC_VER
#define UNIQUE_ID UID_CAT2(UID_PREFIX,__COUNTER__)
#else
#define UNIQUE_ID UID_CAT2(UID_PREFIX,__LINE__)
#endif

// this allows enumerations to be used as flags, and still remain type-safe!
#define DEFINE_ENUM_BITWISE_OPERATORS( Type ) \
	inline Type  operator|  ( Type  a, Type b ) { return Type( int( a ) | int( b ) ); } \
	inline Type  operator&  ( Type  a, Type b ) { return Type( int( a ) & int( b ) ); } \
	inline Type  operator^  ( Type  a, Type b ) { return Type( int( a ) ^ int( b ) ); } \
	inline Type  operator<< ( Type  a, int  b ) { return Type( int( a ) << b ); } \
	inline Type  operator>> ( Type  a, int  b ) { return Type( int( a ) >> b ); } \
	inline Type &operator|= ( Type &a, Type b ) { return a = a |  b; } \
	inline Type &operator&= ( Type &a, Type b ) { return a = a &  b; } \
	inline Type &operator^= ( Type &a, Type b ) { return a = a ^  b; } \
	inline Type &operator<<=( Type &a, int  b ) { return a = a << b; } \
	inline Type &operator>>=( Type &a, int  b ) { return a = a >> b; } \
	inline Type  operator~( Type a ) { return Type( ~int( a ) ); }

// defines increment/decrement operators for enums for easy iteration
#define DEFINE_ENUM_INCREMENT_OPERATORS( Type ) \
	inline Type &operator++( Type &a      ) { return a = Type( int( a ) + 1 ); } \
	inline Type &operator--( Type &a      ) { return a = Type( int( a ) - 1 ); } \
	inline Type  operator++( Type &a, int ) { Type t = a; ++a; return t; } \
	inline Type  operator--( Type &a, int ) { Type t = a; --a; return t; }

#include "tier0/valve_on.h"

#endif // BASETYPES_H
