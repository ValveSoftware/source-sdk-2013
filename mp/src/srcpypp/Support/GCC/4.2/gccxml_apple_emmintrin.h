/* APPLE LOCAL file mainline 2005-06-30 Radar 4131077 */
/* Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

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

#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED

#ifdef __SSE2__
#include <xmmintrin.h>

/* SSE2 */
typedef double __v2df __attribute__ ((__vector_size__ (16)));
typedef long long __v2di __attribute__ ((__vector_size__ (16)));
typedef int __v4si __attribute__ ((__vector_size__ (16)));
typedef short __v8hi __attribute__ ((__vector_size__ (16)));
typedef char __v16qi __attribute__ ((__vector_size__ (16)));

/* The Intel API is flexible enough that we must allow aliasing with other
   vector types, and their scalar components.  */
typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
typedef double __m128d __attribute__ ((__vector_size__ (16), __may_alias__));

/* Create a selector for use with the SHUFPD instruction.  */
#define _MM_SHUFFLE2(fp1,fp0) \
 (((fp1) << 1) | (fp0))

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

/* APPLE LOCAL begin radar 4152603 */
/* Create a vector with element 0 as F and the rest zero.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_sd (double __F)
;

/* Create a vector with both elements equal to F.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_pd (double __F)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_pd1 (double __F)
;

/* Create a vector with the lower value X and upper value W.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_pd (double __W, double __X)
;

/* Create a vector with the lower value W and upper value X.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_pd (double __W, double __X)
;

/* Create a vector of zeros.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setzero_pd (void)
;

/* Sets the low DPFP value of A from the low value of B.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_move_sd (__m128d __A, __m128d __B)
;

/* Load two DPFP values from P.  The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_pd (double const *__P)
;

/* Load two DPFP values from P.  The address need not be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadu_pd (double const *__P)
;

/* Create a vector with all two elements equal to *P.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load1_pd (double const *__P)
;

/* Create a vector with element 0 as *P and the rest zero.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_sd (double const *__P)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_pd1 (double const *__P)
;

/* Load two DPFP values in reverse order.  The address must be aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadr_pd (double const *__P)
;

/* Store two DPFP values.  The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_pd (double *__P, __m128d __A)
;

/* Store two DPFP values.  The address need not be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storeu_pd (double *__P, __m128d __A)
;

/* Stores the lower DPFP value.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_sd (double *__P, __m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE double __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsd_f64 (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storel_pd (double *__P, __m128d __A)
;

/* Stores the upper DPFP value.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storeh_pd (double *__P, __m128d __A)
;

/* Store the lower DPFP value across two words.
   The address must be 16-byte aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store1_pd (double *__P, __m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_pd1 (double *__P, __m128d __A)
;

/* Store two DPFP values in reverse order.  The address must be aligned.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storer_pd (double *__P, __m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi128_si32 (__m128i __A)
;

#ifdef __x86_64__
/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi128_si64 (__m128i __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi128_si64x (__m128i __A)
;
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_div_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_div_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sqrt_pd (__m128d __A)
;

/* Return pair {sqrt (A[0), B[1]}.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sqrt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_and_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_andnot_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_or_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_xor_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmple_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpge_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpneq_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnlt_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnle_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpngt_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnge_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpord_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpunord_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmple_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpge_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpneq_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnlt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnle_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpngt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpnge_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpord_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpunord_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comieq_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comilt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comile_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comigt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comige_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_comineq_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomieq_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomilt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomile_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomigt_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomige_sd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_ucomineq_sd (__m128d __A, __m128d __B)
;

/* Create a vector of Qi, where i is the element number.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_epi64x (long long __q1, long long __q0)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_epi64 (__m64 __q1,  __m64 __q0)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_epi32 (int __q3, int __q2, int __q1, int __q0)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_epi16 (short __q7, short __q6, short __q5, short __q4,
               short __q3, short __q2, short __q1, short __q0)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set_epi8 (char __q15, char __q14, char __q13, char __q12,
              char __q11, char __q10, char __q09, char __q08,
              char __q07, char __q06, char __q05, char __q04,
              char __q03, char __q02, char __q01, char __q00)
;

/* APPLE LOCAL begin 4220129 */
/* functions moved to end of file */
/* APPLE LOCAL end 4220129 */

/* Create a vector of Qi, where i is the element number.
   The parameter order is reversed from the _mm_set_epi* functions.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_epi64 (__m64 __q0, __m64 __q1)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_epi32 (int __q0, int __q1, int __q2, int __q3)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_epi16 (short __q0, short __q1, short __q2, short __q3,
                short __q4, short __q5, short __q6, short __q7)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setr_epi8 (char __q00, char __q01, char __q02, char __q03,
               char __q04, char __q05, char __q06, char __q07,
               char __q08, char __q09, char __q10, char __q11,
               char __q12, char __q13, char __q14, char __q15)
;

/* Create a vector with element 0 as *P and the rest zero.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_load_si128 (__m128i const *__P)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadu_si128 (__m128i const *__P)
;

/* APPLE LOCAL begin 4099020 */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadl_epi64 (__m128i const *__P)
;
/* APPLE LOCAL end 4099020 */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_store_si128 (__m128i *__P, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storeu_si128 (__m128i *__P, __m128i __B)
;

/* APPLE LOCAL begin 4099020 */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_storel_epi64 (__m128i *__P, __m128i __B)
;
/* APPLE LOCAL end 4099020 */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movepi64_pi64 (__m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movpi64_epi64 (__m64 __A)
;

/* APPLE LOCAL begin 4099020 */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_move_epi64 (__m128i __A)
;
/* APPLE LOCAL end 4099020 */

/* Create a vector of zeros.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_setzero_si128 (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtepi32_pd (__m128i __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtepi32_ps (__m128i __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpd_epi32 (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpd_pi32 (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpd_ps (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttpd_epi32 (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttpd_pi32 (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtpi32_pd (__m64 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtps_epi32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttps_epi32 (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtps_pd (__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsd_si32 (__m128d __A)
;

#ifdef __x86_64__
/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsd_si64 (__m128d __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsd_si64x (__m128d __A)
;
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttsd_si32 (__m128d __A)
;

#ifdef __x86_64__
/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttsd_si64 (__m128d __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE long long __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvttsd_si64x (__m128d __A)
;
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsd_ss (__m128 __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi32_sd (__m128d __A, int __B)
;

#ifdef __x86_64__
/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64_sd (__m128d __A, long long __B)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64x_sd (__m128d __A, long long __B)
;
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtss_sd (__m128d __A, __m128 __B)
;

/* APPLE LOCAL 5814283 */
#define _mm_shuffle_pd(__A, __B, __C) ((__m128d)__builtin_ia32_shufpd ((__v2df)(__A), (__v2df)(__B), (__C)))

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_pd (__m128d __A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadh_pd (__m128d __A, double const *__B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_loadl_pd (__m128d __A, double const *__B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movemask_pd (__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_packs_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_packs_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_packus_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpackhi_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_unpacklo_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_add_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_adds_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_adds_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_adds_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_adds_epu16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sub_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_subs_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_subs_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_subs_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_subs_epu16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_madd_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mulhi_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mullo_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m64 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_su32 (__m64 __A, __m64 __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mul_epu32 (__m128i __A, __m128i __B)
;

#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_slli_epi16 (__m128i __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_slli_epi32 (__m128i __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_slli_epi64 (__m128i __A, int __B)
;
#else
#define _mm_slli_epi16(__A, __B) \
  ((__m128i)__builtin_ia32_psllwi128 ((__v8hi)(__A), __B))
#define _mm_slli_epi32(__A, __B) \
  ((__m128i)__builtin_ia32_pslldi128 ((__v8hi)(__A), __B))
#define _mm_slli_epi64(__A, __B) \
  ((__m128i)__builtin_ia32_psllqi128 ((__v8hi)(__A), __B))
#endif

#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srai_epi16 (__m128i __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srai_epi32 (__m128i __A, int __B)
;
#else
#define _mm_srai_epi16(__A, __B) \
  ((__m128i)__builtin_ia32_psrawi128 ((__v8hi)(__A), __B))
#define _mm_srai_epi32(__A, __B) \
  ((__m128i)__builtin_ia32_psradi128 ((__v8hi)(__A), __B))
#endif

#if 0
static __m128i __attribute__((__always_inline__))
_mm_srli_si128 (__m128i __A, int __B)
;

static __m128i __attribute__((__always_inline__))
_mm_srli_si128 (__m128i __A, int __B)
;
#else
/* APPLE LOCAL begin 5919583 */
#define _mm_srli_si128  (__m128i)__builtin_ia32_psrldqi128_byteshift
#define _mm_slli_si128  (__m128i)__builtin_ia32_pslldqi128_byteshift
/* APPLE LOCAL end 5919583 */
#endif

#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srli_epi16 (__m128i __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srli_epi32 (__m128i __A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srli_epi64 (__m128i __A, int __B)
;
#else
#define _mm_srli_epi16(__A, __B) \
  ((__m128i)__builtin_ia32_psrlwi128 ((__v8hi)(__A), __B))
#define _mm_srli_epi32(__A, __B) \
  ((__m128i)__builtin_ia32_psrldi128 ((__v4si)(__A), __B))
#define _mm_srli_epi64(__A, __B) \
  ((__m128i)__builtin_ia32_psrlqi128 ((__v4si)(__A), __B))
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sll_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sll_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sll_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sra_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sra_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srl_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srl_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_srl_epi64 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_and_si128 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_andnot_si128 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_or_si128 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_xor_si128 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpeq_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmplt_epi32 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_epi8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cmpgt_epi32 (__m128i __A, __m128i __B)
;

#if 0
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_extract_epi16 (__m128i const __A, int const __N)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_insert_epi16 (__m128i const __A, int const __D, int const __N)
;
#else
#define _mm_extract_epi16(A, N) \
  ((int) __builtin_ia32_vec_ext_v8hi ((__v8hi)(A), (N)))
#define _mm_insert_epi16(A, D, N) \
  ((__m128i) __builtin_ia32_vec_set_v8hi ((__v8hi)(A), (D), (N)))
#endif

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_max_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_epi16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_min_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE int __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_movemask_epi8 (__m128i __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mulhi_epu16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin 5814283 */
#define _mm_shufflehi_epi16(__A, __B) ((__m128i)__builtin_ia32_pshufhw ((__v8hi)(__A), __B))
#define _mm_shufflelo_epi16(__A, __B) ((__m128i)__builtin_ia32_pshuflw ((__v8hi)(__A), __B))
#define _mm_shuffle_epi32(__A, __B) ((__m128i)__builtin_ia32_pshufd ((__v4si)(__A), __B))
/* APPLE LOCAL end 5814283 */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_maskmoveu_si128 (__m128i __A, __m128i __B, char *__C)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_avg_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_avg_epu16 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_sad_epu8 (__m128i __A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_stream_si32 (int *__A, int __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_stream_si128 (__m128i *__A, __m128i __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_stream_pd (double *__A, __m128d __B)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_clflush (void const *__A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_lfence (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE void __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_mfence (void)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi32_si128 (int __A)
;

#ifdef __x86_64__
/* Intel intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64_si128 (long long __A)
;

/* Microsoft intrinsic.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_cvtsi64x_si128 (long long __A)
;
#endif

/* Casts between various SP, DP, INT vector types.  Note that these do no
   conversion of values, they just change the type.  */
/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castpd_ps(__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castpd_si128(__m128d __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castps_pd(__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castps_si128(__m128 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128 __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castsi128_ps(__m128i __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128d __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_castsi128_pd(__m128i __A)
;
/* APPLE LOCAL end radar 4152603 */

/* APPLE LOCAL begin 4220129, 4286110 */
/* Set all of the elements of the vector to A.  */

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_epi64x (long long __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_epi64 (__m64 __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_epi32 (int __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_epi16 (short __A)
;

/* APPLE LOCAL begin radar 5618945 */
__STATIC_INLINE __m128i __attribute__((__always_inline__))
/* APPLE LOCAL end radar 5618945 */
_mm_set1_epi8 (char __A)
;
/* APPLE LOCAL end 4220129, 4286110 */

/* APPLE LOCAL begin nodebug inline 4152603 */
#undef __always_inline__
/* APPLE LOCAL end nodebug inline 4152603 */

#endif /* __SSE2__  */

#endif /* _EMMINTRIN_H_INCLUDED */
