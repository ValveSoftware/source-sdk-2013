/* APPLE LOCAL file mainline 2005-06-30 Radar 4131077 */
/* Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007
   Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, if you include this header file into source
   files compiled by GCC, this header file does not by itself cause
   the resulting executable to be covered by the GNU General Public
   License.  This exception does not however invalidate any other
   reasons why the executable file might be covered by the GNU General
   Public License.  */

/* Implemented from the specification included in the Intel C++ Compiler
   User Guide and Reference, version 9.0.  */

#ifndef _XMMINTRIN_H_INCLUDED
#define _XMMINTRIN_H_INCLUDED

#ifndef __SSE__
# error "SSE instruction set not enabled"
#else

/* We need type definitions from the MMX header file.  */
#include <mmintrin.h>

/* Get _mm_malloc () and _mm_free ().  */
/* APPLE LOCAL begin xmmintrin.h for kernel 4123064 */
#if __STDC_HOSTED__
#include <mm_malloc.h>
#endif
/* APPLE LOCAL end xmmintrin.h for kernel 4123064 */

/* The Intel API is flexible enough that we must allow aliasing with other
   vector types, and their scalar components.  */
typedef float __m128 __attribute__ ((__vector_size__ (16), __may_alias__));

/* Internal data types for implementing the intrinsics.  */
typedef float __v4sf __attribute__ ((__vector_size__ (16)));

/* Create a selector for use with the SHUFPS instruction.  */
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
 (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))

/* Constants for use with _mm_prefetch.  */
enum _mm_hint
{
  _MM_HINT_T0 = 3,
  _MM_HINT_T1 = 2,
  _MM_HINT_T2 = 1,
  _MM_HINT_NTA = 0
};

/* Bits in the MXCSR.  */
#define _MM_EXCEPT_MASK       0x003f
#define _MM_EXCEPT_INVALID    0x0001
#define _MM_EXCEPT_DENORM     0x0002
#define _MM_EXCEPT_DIV_ZERO   0x0004
#define _MM_EXCEPT_OVERFLOW   0x0008
#define _MM_EXCEPT_UNDERFLOW  0x0010
#define _MM_EXCEPT_INEXACT    0x0020

#define _MM_MASK_MASK         0x1f80
#define _MM_MASK_INVALID      0x0080
#define _MM_MASK_DENORM       0x0100
#define _MM_MASK_DIV_ZERO     0x0200
#define _MM_MASK_OVERFLOW     0x0400
#define _MM_MASK_UNDERFLOW    0x0800
#define _MM_MASK_INEXACT      0x1000

#define _MM_ROUND_MASK        0x6000
#define _MM_ROUND_NEAREST     0x0000
#define _MM_ROUND_DOWN        0x2000
#define _MM_ROUND_UP          0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_FLUSH_ZERO_MASK   0x8000
#define _MM_FLUSH_ZERO_ON     0x8000
#define _MM_FLUSH_ZERO_OFF    0x0000

/* APPLE LOCAL begin nodebug inline 4152603 */
#define __always_inline__ __always_inline__, __nodebug__
/* APPLE LOCAL end nodebug inline 4152603 */

/* APPLE LOCAL begin radar 5618945 */
#undef __STATIC_INLINE
#ifdef __GNUC_STDC_INLINE__
#define __STATIC_INLINE __inline
#else
#define __STATIC_INLINE static __inline
#endif
/* APPLE LOCAL end radar 5618945 */

/* Create a vector of zeros.  */
/* APPLE LOCAL begin radar 4152603 */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setzero_ps (void)
;

/* Perform the respective operation on the lower SPFP (single-precision
   floating-point) values of A and B; the upper three SPFP values are
   passed through from A.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_div_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sqrt_ss (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_rcp_ss (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_rsqrt_ss (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_ss (__m128 __A, __m128 __B)
;

/* Perform the respective operation on the four SPFP values in A and B.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_div_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sqrt_ps (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_rcp_ps (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_rsqrt_ps (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_ps (__m128 __A, __m128 __B)
;

/* Perform logical bit-wise operations on 128-bit values.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_and_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_andnot_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_or_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_xor_ps (__m128 __A, __m128 __B)
;

/* Perform a comparison on the lower SPFP values of A and B.  If the
   comparison is true, place a mask of all ones in the result, otherwise a
   mask of zeros.  The upper three SPFP values are passed through from A.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmple_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpge_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpneq_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnlt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnle_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpngt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnge_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpord_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpunord_ss (__m128 __A, __m128 __B)
;

/* Perform a comparison on the four SPFP values of A and B.  For each
   element, if the comparison is true, place a mask of all ones in the
   result, otherwise a mask of zeros.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmple_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpge_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpneq_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnlt_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnle_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpngt_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnge_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpord_ps (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpunord_ps (__m128 __A, __m128 __B)
;

/* Compare the lower SPFP values of A and B and return 1 if true
   and 0 if false.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comieq_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comilt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comile_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comigt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comige_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comineq_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomieq_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomilt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomile_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomigt_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomige_ss (__m128 __A, __m128 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomineq_ss (__m128 __A, __m128 __B)
;

/* Convert the lower SPFP value to a 32-bit integer according to the current
   rounding mode.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtss_si32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvt_ss2si (__m128 __A)
;

#ifdef __x86_64__
/* Convert the lower SPFP value to a 32-bit integer according to the
   current rounding mode.  */

/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtss_si64 (__m128 __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtss_si64x (__m128 __A)
;
#endif

/* Convert the two lower SPFP values to 32-bit integers according to the
   current rounding mode.  Return the integers in packed form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtps_pi32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvt_ps2pi (__m128 __A)
;

/* Truncate the lower SPFP value to a 32-bit integer.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttss_si32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtt_ss2si (__m128 __A)
;

#ifdef __x86_64__
/* Truncate the lower SPFP value to a 32-bit integer.  */

/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttss_si64 (__m128 __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttss_si64x (__m128 __A)
;
#endif

/* Truncate the two lower SPFP values to 32-bit integers.  Return the
   integers in packed form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttps_pi32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtt_ps2pi (__m128 __A)
;

/* Convert B to a SPFP value and insert it as element zero in A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi32_ss (__m128 __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvt_si2ss (__m128 __A, int __B)
;

#ifdef __x86_64__
/* Convert B to a SPFP value and insert it as element zero in A.  */

/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64_ss (__m128 __A, long long __B)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64x_ss (__m128 __A, long long __B)
;
#endif

/* Convert the two 32-bit values in B to SPFP form and insert them
   as the two lower elements in A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpi32_ps (__m128 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvt_pi2ps (__m128 __A, __m64 __B)
;

/* Convert the four signed 16-bit values in A to SPFP form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpi16_ps (__m64 __A)
;

/* Convert the four unsigned 16-bit values in A to SPFP form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpu16_ps (__m64 __A)
;

/* Convert the low four signed 8-bit values in A to SPFP form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpi8_ps (__m64 __A)
;

/* Convert the low four unsigned 8-bit values in A to SPFP form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpu8_ps(__m64 __A)
;

/* Convert the four signed 32-bit values in A and B to SPFP form.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpi32x2_ps(__m64 __A, __m64 __B)
;

/* Convert the four SPFP values in A to four signed 16-bit integers.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtps_pi16(__m128 __A)
;

/* Convert the four SPFP values in A to four signed 8-bit integers.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtps_pi8(__m128 __A)
;

/* Selects four specific SPFP values from A and B based on MASK.  */
#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_shuffle_ps (__m128 __A, __m128 __B, int __mask)
;
#else
#define _mm_shuffle_ps(A, B, MASK) \
 ((__m128) __builtin_ia32_shufps ((__v4sf)(A), (__v4sf)(B), (MASK)))
#endif


/* Selects and interleaves the upper two SPFP values from A and B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_ps (__m128 __A, __m128 __B)
;

/* Selects and interleaves the lower two SPFP values from A and B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_ps (__m128 __A, __m128 __B)
;

/* Sets the upper two SPFP values with 64-bits of data loaded from P;
   the lower two values are passed through from A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadh_pi (__m128 __A, __m64 const *__P)
;

/* Stores the upper two SPFP values of A into P.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storeh_pi (__m64 *__P, __m128 __A)
;

/* Moves the upper two values of B into the lower two values of A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movehl_ps (__m128 __A, __m128 __B)
;

/* Moves the lower two values of B into the upper two values of A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movelh_ps (__m128 __A, __m128 __B)
;

/* Sets the lower two SPFP values with 64-bits of data loaded from P;
   the upper two values are passed through from A.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadl_pi (__m128 __A, __m64 const *__P)
;

/* Stores the lower two SPFP values of A into P.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storel_pi (__m64 *__P, __m128 __A)
;

/* Creates a 4-bit mask from the most significant bits of the SPFP values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movemask_ps (__m128 __A)
;

/* Return the contents of the control register.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE unsigned int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_getcsr (void)
;

/* Read exception bits from the control register.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE unsigned int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_GET_EXCEPTION_STATE (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE unsigned int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_GET_EXCEPTION_MASK (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE unsigned int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_GET_ROUNDING_MODE (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE unsigned int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_GET_FLUSH_ZERO_MODE (void)
;

/* Set the control register to I.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setcsr (unsigned int __I)
;

/* Set exception bits in the control register.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_SET_EXCEPTION_STATE(unsigned int __mask)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_SET_EXCEPTION_MASK (unsigned int __mask)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_SET_ROUNDING_MODE (unsigned int __mode)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_MM_SET_FLUSH_ZERO_MODE (unsigned int __mode)
;

/* Create a vector with element 0 as F and the rest zero.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_ss (float __F)
;

/* Create a vector with all four elements equal to F.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_ps (float __F)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_ps1 (float __F)
;

/* Create a vector with element 0 as *P and the rest zero.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_ss (float const *__P)
;

/* Create a vector with all four elements equal to *P.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load1_ps (float const *__P)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_ps1 (float const *__P)
;

/* Load four SPFP values from P.  The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_ps (float const *__P)
;

/* Load four SPFP values from P.  The address need not be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadu_ps (float const *__P)
;

/* Load four SPFP values in reverse order.  The address must be aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadr_ps (float const *__P)
;

/* Create the vector [Z Y X W].  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_ps (const float __Z, const float __Y, const float __X, const float __W)
;

/* Create the vector [W X Y Z].  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_ps (float __Z, float __Y, float __X, float __W)
;

/* Stores the lower SPFP value.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_ss (float *__P, __m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE float __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtss_f32 (__m128 __A)
;

/* Store four SPFP values.  The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_ps (float *__P, __m128 __A)
;

/* Store four SPFP values.  The address need not be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storeu_ps (float *__P, __m128 __A)
;

/* Store the lower SPFP value across four words.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store1_ps (float *__P, __m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_ps1 (float *__P, __m128 __A)
;

/* Store four SPFP values in reverse order.  The address must be aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storer_ps (float *__P, __m128 __A)
;

/* Sets the low SPFP value of A from the low value of B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_move_ss (__m128 __A, __m128 __B)
;

/* Extracts one of the four words of A.  The selector N must be immediate.  */
#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_extract_pi16 (__m64 const __A, int const __N)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pextrw (__m64 const __A, int const __N)
;
#else
#define _mm_extract_pi16(A, N)  __builtin_ia32_vec_ext_v4hi ((__v4hi)(A), (N))
#define _m_pextrw(A, N)         _mm_extract_pi16((A), (N))
#endif

/* Inserts word D into one of four words of A.  The selector N must be
   immediate.  */
#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_insert_pi16 (__m64 const __A, int const __D, int const __N)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pinsrw (__m64 const __A, int const __D, int const __N)
;
#else
#define _mm_insert_pi16(A, D, N) \
  ((__m64) __builtin_ia32_vec_set_v4hi ((__v4hi)(A), (D), (N)))
#define _m_pinsrw(A, D, N)       _mm_insert_pi16((A), (D), (N))
#endif

/* Compute the element-wise maximum of signed 16-bit values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_pi16 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pmaxsw (__m64 __A, __m64 __B)
;

/* Compute the element-wise maximum of unsigned 8-bit values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_pu8 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pmaxub (__m64 __A, __m64 __B)
;

/* Compute the element-wise minimum of signed 16-bit values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_pi16 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pminsw (__m64 __A, __m64 __B)
;

/* Compute the element-wise minimum of unsigned 8-bit values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_pu8 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pminub (__m64 __A, __m64 __B)
;

/* Create an 8-bit mask of the signs of 8-bit values.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movemask_pi8 (__m64 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pmovmskb (__m64 __A)
;

/* Multiply four unsigned 16-bit values in A by four unsigned 16-bit values
   in B and produce the high 16 bits of the 32-bit results.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mulhi_pu16 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pmulhuw (__m64 __A, __m64 __B)
;

/* Return a combination of the four 16-bit values in A.  The selector
   must be an immediate.  */
#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_shuffle_pi16 (__m64 __A, int __N)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pshufw (__m64 __A, int __N)
;
#else
#define _mm_shuffle_pi16(A, N) \
  ((__m64) __builtin_ia32_pshufw ((__v4hi)(A), (N)))
#define _m_pshufw(A, N)         _mm_shuffle_pi16 ((A), (N))
#endif

/* Conditionally store byte elements of A into P.  The high bit of each
   byte in the selector N determines whether the corresponding byte from
   A is stored.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_maskmove_si64 (__m64 __A, __m64 __N, char *__P)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_maskmovq (__m64 __A, __m64 __N, char *__P)
;

/* Compute the rounded averages of the unsigned 8-bit values in A and B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_avg_pu8 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pavgb (__m64 __A, __m64 __B)
;

/* Compute the rounded averages of the unsigned 16-bit values in A and B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_avg_pu16 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_pavgw (__m64 __A, __m64 __B)
;

/* Compute the sum of the absolute differences of the unsigned 8-bit
   values in A and B.  Return the value in the lower 16-bit word; the
   upper words are cleared.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sad_pu8 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_m_psadbw (__m64 __A, __m64 __B)
;

/* Loads one cache line from address P to a location "closer" to the
   processor.  The selector I specifies the type of prefetch operation.  */
#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_prefetch (void *__P, enum _mm_hint __I)
;
#else
#define _mm_prefetch(P, I) \
  __builtin_prefetch ((P), 0, (I))
#endif

/* Stores the data in A to the address P without polluting the caches.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_stream_pi (__m64 *__P, __m64 __A)
;

/* Likewise.  The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_stream_ps (float *__P, __m128 __A)
;

/* Guarantees that every preceding store is globally visible before
   any subsequent store.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sfence (void)
;

/* The execution of the next instruction is delayed by an implementation
   specific amount of time.  The instruction does not modify the
   architectural state.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_pause (void)
;
/* APPLE LOCAL end radar 4152603 */

/* Transpose the 4x4 matrix composed of row[0-3].  */
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3)                       \
do {                                                                    \
} while (0)

/* APPLE LOCAL begin nodebug inline 4152603 */
#undef __always_inline__
/* APPLE LOCAL end nodebug inline 4152603 */

/* For backward source compatibility.  */
#include <emmintrin.h>

#endif /* __SSE__ */
#endif /* _XMMINTRIN_H_INCLUDED */
