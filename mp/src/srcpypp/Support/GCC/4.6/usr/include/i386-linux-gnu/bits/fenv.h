/* Copyright (C) 1997, 1998, 1999, 2000, 2011 Free Software Foundation, Inc.
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

#ifndef _FENV_H
# error "Never use <bits/fenv.h> directly; include <fenv.h> instead."
#endif


/* Define bits representing the exception.  We use the bit positions
   of the appropriate bits in the FPU control word.  */
enum
  {
    FE_INVALID = 0x01,
#define FE_INVALID	FE_INVALID
    __FE_DENORM = 0x02,
    FE_DIVBYZERO = 0x04,
#define FE_DIVBYZERO	FE_DIVBYZERO
    FE_OVERFLOW = 0x08,
#define FE_OVERFLOW	FE_OVERFLOW
    FE_UNDERFLOW = 0x10,
#define FE_UNDERFLOW	FE_UNDERFLOW
    FE_INEXACT = 0x20
#define FE_INEXACT	FE_INEXACT
  };

#define FE_ALL_EXCEPT \
	(FE_INEXACT | FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID)

/* The ix87 FPU supports all of the four defined rounding modes.  We
   use again the bit positions in the FPU control word as the values
   for the appropriate macros.  */
enum
  {
    FE_TONEAREST = 0,
#define FE_TONEAREST	FE_TONEAREST
    FE_DOWNWARD = 0x400,
#define FE_DOWNWARD	FE_DOWNWARD
    FE_UPWARD = 0x800,
#define FE_UPWARD	FE_UPWARD
    FE_TOWARDZERO = 0xc00
#define FE_TOWARDZERO	FE_TOWARDZERO
  };


/* Type representing exception flags.  */
typedef unsigned short int fexcept_t;


/* Type representing floating-point environment.  This function corresponds
   to the layout of the block written by the `fstenv'.  */
typedef struct
  {
    unsigned short int __control_word;
    unsigned short int __unused1;
    unsigned short int __status_word;
    unsigned short int __unused2;
    unsigned short int __tags;
    unsigned short int __unused3;
    unsigned int __eip;
    unsigned short int __cs_selector;
    unsigned int __opcode:11;
    unsigned int __unused4:5;
    unsigned int __data_offset;
    unsigned short int __data_selector;
    unsigned short int __unused5;
  }
fenv_t;

/* If the default argument is used we use this value.  */
#define FE_DFL_ENV	((__const fenv_t *) -1)

#ifdef __USE_GNU
/* Floating-point environment where none of the exception is masked.  */
# define FE_NOMASK_ENV	((__const fenv_t *) -2)
#endif


#ifdef __USE_EXTERN_INLINES
__BEGIN_DECLS

/* Optimized versions.  */
extern int __REDIRECT_NTH (__feraiseexcept_renamed, (int), feraiseexcept);
__extern_inline int
__NTH (feraiseexcept (int __excepts))
{
  if (__builtin_constant_p (__excepts)
      && (__excepts & ~(FE_INVALID | FE_DIVBYZERO)) == 0)
    {
      if ((FE_INVALID & __excepts) != 0)
	{
	  /* One example of a invalid operation is 0.0 / 0.0.  */
	  float __f = 0.0;

# ifdef __SSE_MATH__
	  __asm__ __volatile__ ("divss %0, %0 " : : "x" (__f));
# else
	  __asm__ __volatile__ ("fdiv %%st, %%st(0); fwait"
				: "=t" (__f) : "0" (__f));
# endif
	  (void) &__f;
	}
      if ((FE_DIVBYZERO & __excepts) != 0)
	{
	  float __f = 1.0;
	  float __g = 0.0;

# ifdef __SSE_MATH__
	  __asm__ __volatile__ ("divss %1, %0" : : "x" (__f), "x" (__g));
# else
	  __asm__ __volatile__ ("fdivp %%st(1), %%st; fwait"
				: "=t" (__f) : "0" (__f), "u" (__g) : "st(1)");
# endif
	  (void) &__f;
	}

      return 0;
    }

  return __feraiseexcept_renamed (__excepts);
}

__END_DECLS
#endif
