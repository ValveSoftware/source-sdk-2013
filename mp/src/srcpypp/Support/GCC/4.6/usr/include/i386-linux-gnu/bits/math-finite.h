/* Entry points to finite-math-only compiler runs.
   Copyright (C) 2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _MATH_H
# error "Never use <bits/math-finite.h> directly; include <math.h> instead."
#endif

/* acos.  */
extern double __REDIRECT_NTH (acos, (double), __acos_finite);
extern float __REDIRECT_NTH (acosf, (float), __acosf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (acosl, (long double), __acosl_finite);
#endif

#if defined __USE_MISC || defined __USE_XOPEN_EXTENDED || defined __USE_ISOC99
/* acosh.  */
extern double __REDIRECT_NTH (acosh, (double), __acosh_finite);
extern float __REDIRECT_NTH (acoshf, (float), __acoshf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (acoshl, (long double), __acoshl_finite);
# endif
#endif

/* asin.  */
extern double __REDIRECT_NTH (asin, (double), __asin_finite);
extern float __REDIRECT_NTH (asinf, (float), __asinf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (asinl, (long double), __asinl_finite);
#endif

/* atan2.  */
extern double __REDIRECT_NTH (atan2, (double, double), __atan2_finite);
extern float __REDIRECT_NTH (atan2f, (float, float), __atan2f_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (atan2l, (long double, long double),
				   __atan2l_finite);
#endif

#if defined __USE_MISC || defined __USE_XOPEN_EXTENDED || defined __USE_ISOC99
/* atanh.  */
extern double __REDIRECT_NTH (atanh, (double), __atanh_finite);
extern float __REDIRECT_NTH (atanhf, (float), __atanhf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (atanhl, (long double), __atanhl_finite);
# endif
#endif

/* cosh.  */
extern double __REDIRECT_NTH (cosh, (double), __cosh_finite);
extern float __REDIRECT_NTH (coshf, (float), __coshf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (coshl, (long double), __coshl_finite);
#endif

/* exp.  */
extern double __REDIRECT_NTH (exp, (double), __exp_finite);
extern float __REDIRECT_NTH (expf, (float), __expf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (expl, (long double), __expl_finite);
#endif

#ifdef __USE_GNU
/* exp10.  */
extern double __REDIRECT_NTH (exp10, (double), __exp10_finite);
extern float __REDIRECT_NTH (exp10f, (float), __exp10f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (exp10l, (long double), __exp10l_finite);
# endif

/* pow10.  */
extern double __REDIRECT_NTH (pow10, (double), __exp10_finite);
extern float __REDIRECT_NTH (pow10f, (float), __exp10f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (pow10l, (long double), __exp10l_finite);
# endif
#endif

#ifdef __USE_ISOC99
/* exp2.  */
extern double __REDIRECT_NTH (exp2, (double), __exp2_finite);
extern float __REDIRECT_NTH (exp2f, (float), __exp2f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (exp2l, (long double), __exp2l_finite);
# endif
#endif

/* fmod.  */
extern double __REDIRECT_NTH (fmod, (double, double), __fmod_finite);
extern float __REDIRECT_NTH (fmodf, (float, float), __fmodf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (fmodl, (long double, long double),
				   __fmodl_finite);
#endif

#ifdef __USE_ISOC99
/* hypot.  */
extern double __REDIRECT_NTH (hypot, (double, double), __hypot_finite);
extern float __REDIRECT_NTH (hypotf, (float, float), __hypotf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (hypotl, (long double, long double),
				   __hypotl_finite);
# endif
#endif

#if defined __USE_MISC || defined __USE_XOPEN
/* j0.  */
extern double __REDIRECT_NTH (j0, (double), __j0_finite);
extern float __REDIRECT_NTH (j0f, (float), __j0f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (j0l, (long double), __j0l_finite);
# endif

/* y0.  */
extern double __REDIRECT_NTH (y0, (double), __y0_finite);
extern float __REDIRECT_NTH (y0f, (float), __y0f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (y0l, (long double), __y0l_finite);
# endif

/* j1.  */
extern double __REDIRECT_NTH (j1, (double), __j1_finite);
extern float __REDIRECT_NTH (j1f, (float), __j1f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (j1l, (long double), __j1l_finite);
# endif

/* y1.  */
extern double __REDIRECT_NTH (y1, (double), __y1_finite);
extern float __REDIRECT_NTH (y1f, (float), __y1f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (y1l, (long double), __y1l_finite);
# endif

/* jn.  */
extern double __REDIRECT_NTH (jn, (int, double), __jn_finite);
extern float __REDIRECT_NTH (jnf, (int, float), __jnf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (jnl, (int, long double), __jnl_finite);
# endif

/* yn.  */
extern double __REDIRECT_NTH (yn, (int, double), __yn_finite);
extern float __REDIRECT_NTH (ynf, (int, float), __ynf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (ynl, (int, long double), __ynl_finite);
# endif
#endif

#ifdef __USE_MISC
/* lgamma_r.  */
extern double __REDIRECT_NTH (lgamma_r, (double, int *), __lgamma_r_finite);
extern float __REDIRECT_NTH (lgammaf_r, (float, int *), __lgammaf_r_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (lgammal_r, (long double, int *),
				   __lgammal_r_finite);
# endif
#endif

#if defined __USE_MISC || defined __USE_XOPEN || defined __USE_ISOC99
/* lgamma.  */
__extern_always_inline double __NTH (lgamma (double __d))
{
# ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgamma_r (__d, &__local_signgam);
# else
  return lgamma_r (__d, &signgam);
# endif
}
__extern_always_inline float __NTH (lgammaf (float __d))
{
# ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgammaf_r (__d, &__local_signgam);
# else
  return lgammaf_r (__d, &signgam);
# endif
}
# ifdef __MATH_DECLARE_LDOUBLE
__extern_always_inline long double __NTH (lgammal (long double __d))
{
# ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgammal_r (__d, &__local_signgam);
# else
  return lgammal_r (__d, &signgam);
# endif
}
# endif
#endif

#if defined __USE_MISC || defined __USE_XOPEN
/* gamma.  */
__extern_always_inline double __NTH (gamma (double __d))
{
# ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgamma_r (__d, &__local_signgam);
# else
  return lgamma_r (__d, &signgam);
# endif
}
__extern_always_inline float __NTH (gammaf (float __d))
{
# ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgammaf_r (__d, &__local_signgam);
# else
  return lgammaf_r (__d, &signgam);
# endif
}
# ifdef __MATH_DECLARE_LDOUBLE
__extern_always_inline long double __NTH (gammal (long double __d))
{
#  ifdef __USE_ISOC99
  int __local_signgam = 0;
  return lgammal_r (__d, &__local_signgam);
#  else
  return lgammal_r (__d, &signgam);
#  endif
}
# endif
#endif

/* log.  */
extern double __REDIRECT_NTH (log, (double), __log_finite);
extern float __REDIRECT_NTH (logf, (float), __logf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (logl, (long double), __logl_finite);
#endif

/* log10.  */
extern double __REDIRECT_NTH (log10, (double), __log10_finite);
extern float __REDIRECT_NTH (log10f, (float), __log10f_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (log10l, (long double), __log10l_finite);
#endif

#ifdef __USE_ISOC99
/* log2.  */
extern double __REDIRECT_NTH (log2, (double), __log2_finite);
extern float __REDIRECT_NTH (log2f, (float), __log2f_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (log2l, (long double), __log2l_finite);
# endif
#endif

/* pow.  */
extern double __REDIRECT_NTH (pow, (double, double), __pow_finite);
extern float __REDIRECT_NTH (powf, (float, float), __powf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (powl, (long double, long double),
				   __powl_finite);
#endif

/* remainder.  */
extern double __REDIRECT_NTH (remainder, (double, double), __remainder_finite);
extern float __REDIRECT_NTH (remainderf, (float, float), __remainderf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (remainderl, (long double, long double),
				   __remainderl_finite);
#endif

#if defined __USE_MISC || defined __USE_XOPEN_EXTENDED
/* scalb.  */
extern double __REDIRECT_NTH (scalb, (double, double), __scalb_finite);
extern float __REDIRECT_NTH (scalbf, (float, float), __scalbf_finite);
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (scalbl, (long double, long double),
				   __scalbl_finite);
# endif
#endif

/* sinh.  */
extern double __REDIRECT_NTH (sinh, (double), __sinh_finite);
extern float __REDIRECT_NTH (sinhf, (float), __sinhf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (sinhl, (long double), __sinhl_finite);
#endif

/* sqrt.  */
extern double __REDIRECT_NTH (sqrt, (double), __sqrt_finite);
extern float __REDIRECT_NTH (sqrtf, (float), __sqrtf_finite);
#ifdef __MATH_DECLARE_LDOUBLE
extern long double __REDIRECT_NTH (sqrtl, (long double), __sqrtl_finite);
#endif

#ifdef __USE_ISOC99
/* tgamma.  */
extern double __gamma_r_finite (double, int *);
__extern_always_inline double __NTH (tgamma (double __d))
{
  int __local_signgam = 0;
  double __res = __gamma_r_finite (__d, &__local_signgam);
  return __local_signgam < 0 ? -__res : __res;
}
extern float __gammaf_r_finite (float, int *);
__extern_always_inline float __NTH (tgammaf (float __d))
{
  int __local_signgam = 0;
  float __res = __gammaf_r_finite (__d, &__local_signgam);
  return __local_signgam < 0 ? -__res : __res;
}
# ifdef __MATH_DECLARE_LDOUBLE
extern long double __gammal_r_finite (long double, int *);
__extern_always_inline long double __NTH (tgammal (long double __d))
{
  int __local_signgam = 0;
  long double __res = __gammal_r_finite (__d, &__local_signgam);
  return __local_signgam < 0 ? -__res : __res;
}
# endif
#endif
