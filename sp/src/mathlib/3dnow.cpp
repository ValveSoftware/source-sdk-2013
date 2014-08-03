//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 3DNow Math primitives.
//
//=====================================================================================//

#include <math.h>
#include <float.h>	// Needed for FLT_EPSILON
#include "basetypes.h"
#include <memory.h>
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "mathlib/amd3dx.h"
#include "mathlib/vector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(COMPILER_MSVC64) && !defined(LINUX)
// Implement for 64-bit Windows if needed.
// Clang hits "fatal error: error in backend:" and other errors when trying
// to compile the inline assembly below. 3DNow support is highly unlikely to
// be useful/used, so it's not worth spending time on fixing.

#pragma warning(disable:4244)   // "conversion from 'const int' to 'float', possible loss of data"
#pragma warning(disable:4730)	// "mixing _m64 and floating point expressions may result in incorrect code"

//-----------------------------------------------------------------------------
// 3D Now Implementations of optimized routines:
//-----------------------------------------------------------------------------
float _3DNow_Sqrt(float x)
{
	Assert( s_bMathlibInitialized );
	float	root = 0.f;
#ifdef _WIN32
	_asm
	{
		femms
		movd		mm0, x
		PFRSQRT		(mm1,mm0)
		punpckldq	mm0, mm0
		PFMUL		(mm0, mm1)
		movd		root, mm0
		femms
	}
#elif LINUX
 	__asm __volatile__( "femms" );
 	__asm __volatile__
	(
		"pfrsqrt    %y0, %y1 \n\t"
		"punpckldq   %y1, %y1 \n\t"
		"pfmul      %y1, %y0 \n\t"
		: "=y" (root), "=y" (x)
 		:"0" (x)
 	);
 	__asm __volatile__( "femms" );
#else
#error
#endif

	return root;
}

// NJS FIXME: Need to test Recripricol squareroot performance and accuraccy
// on AMD's before using the specialized instruction.
float _3DNow_RSqrt(float x)
{
	Assert( s_bMathlibInitialized );

	return 1.f / _3DNow_Sqrt(x);
}


float FASTCALL _3DNow_VectorNormalize (Vector& vec)
{
	Assert( s_bMathlibInitialized );
	float *v = &vec[0];
	float	radius = 0.f;

	if ( v[0] || v[1] || v[2] )
	{
#ifdef _WIN32
	_asm
		{
			mov			eax, v
			femms
			movq		mm0, QWORD PTR [eax]
			movd		mm1, DWORD PTR [eax+8]
			movq		mm2, mm0
			movq		mm3, mm1
			PFMUL		(mm0, mm0)
			PFMUL		(mm1, mm1)
			PFACC		(mm0, mm0)
			PFADD		(mm1, mm0)
			PFRSQRT		(mm0, mm1)
			punpckldq	mm1, mm1
			PFMUL		(mm1, mm0)
			PFMUL		(mm2, mm0)
			PFMUL		(mm3, mm0)
			movq		QWORD PTR [eax], mm2
			movd		DWORD PTR [eax+8], mm3
			movd		radius, mm1
			femms
		}
#elif LINUX	
		long long a,c;
    		int b,d;
    		memcpy(&a,&vec[0],sizeof(a));
    		memcpy(&b,&vec[2],sizeof(b));
    		memcpy(&c,&vec[0],sizeof(c));
    		memcpy(&d,&vec[2],sizeof(d));

      		__asm __volatile__( "femms" );
        	__asm __volatile__
        	(
        		"pfmul           %y3, %y3\n\t"
        		"pfmul           %y0, %y0 \n\t"
        		"pfacc           %y3, %y3 \n\t"
        		"pfadd           %y3, %y0 \n\t"
        		"pfrsqrt         %y0, %y3 \n\t"
        		"punpckldq       %y0, %y0 \n\t"
        		"pfmul           %y3, %y0 \n\t"
        		"pfmul           %y3, %y2 \n\t"
        		"pfmul           %y3, %y1 \n\t"
        		: "=y" (radius), "=y" (c), "=y" (d)
        		: "y" (a), "0" (b), "1" (c), "2" (d)
        	);
      		memcpy(&vec[0],&c,sizeof(c));
      		memcpy(&vec[2],&d,sizeof(d));		
        	__asm __volatile__( "femms" );

#else
#error
#endif
	}
    return radius;
}


void FASTCALL _3DNow_VectorNormalizeFast (Vector& vec)
{
	_3DNow_VectorNormalize( vec );
}


// JAY: This complains with the latest processor pack
#pragma warning(disable: 4730)

float _3DNow_InvRSquared(const float* v)
{
	Assert( s_bMathlibInitialized );
	float	r2 = 1.f;
#ifdef _WIN32
	_asm { // AMD 3DNow only routine
		mov			eax, v
		femms
		movq		mm0, QWORD PTR [eax]
		movd		mm1, DWORD PTR [eax+8]
		movd		mm2, [r2]
		PFMUL		(mm0, mm0)
		PFMUL		(mm1, mm1)
		PFACC		(mm0, mm0)
		PFADD		(mm1, mm0)
		PFMAX		(mm1, mm2)
		PFRCP		(mm0, mm1)
		movd		[r2], mm0
		femms
	}
#elif LINUX
		long long a,c;
    		int b;
    		memcpy(&a,&v[0],sizeof(a));
    		memcpy(&b,&v[2],sizeof(b));
    		memcpy(&c,&v[0],sizeof(c));

      		__asm __volatile__( "femms" );
        	__asm __volatile__
        	(
			"PFMUL          %y2, %y2 \n\t"
                        "PFMUL          %y3, %y3 \n\t"
                        "PFACC          %y2, %y2 \n\t"
                        "PFADD          %y2, %y3 \n\t"
                        "PFMAX          %y3, %y4 \n\t"
                        "PFRCP          %y3, %y2 \n\t"
                        "movq           %y2, %y0 \n\t"
        		: "=y" (r2)
        		: "0" (r2), "y" (a), "y" (b), "y" (c)
        	);
        	__asm __volatile__( "femms" );
#else
#error
#endif

	return r2;
}

#endif // COMPILER_MSVC64 
