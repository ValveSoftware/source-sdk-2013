/* Optimized, inlined string functions.  i486 version.
   Copyright (C) 1997,1998,1999,2000,2001,2002,2003,2004,2007,2011
   Free Software Foundation, Inc.
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

#ifndef _STRING_H
# error "Never use <bits/string.h> directly; include <string.h> instead."
#endif

/* The ix86 processors can access unaligned multi-byte variables.  */
#define _STRING_ARCH_unaligned	1


/* We only provide optimizations if the user selects them and if
   GNU CC is used.  */
#if !defined __NO_STRING_INLINES && defined __USE_STRING_INLINES \
    && defined __GNUC__ && __GNUC__ >= 2 && !__BOUNDED_POINTERS__

#ifndef __STRING_INLINE
# ifndef __extern_inline
#  define __STRING_INLINE inline
# else
#  define __STRING_INLINE __extern_inline
# endif
#endif

/* The macros are used in some of the optimized implementations below.  */
#define __STRING_SMALL_GET16(src, idx) \
  ((((__const unsigned char *) (src))[idx + 1] << 8)			      \
   | ((__const unsigned char *) (src))[idx])
#define __STRING_SMALL_GET32(src, idx) \
  (((((__const unsigned char *) (src))[idx + 3] << 8			      \
     | ((__const unsigned char *) (src))[idx + 2]) << 8			      \
    | ((__const unsigned char *) (src))[idx + 1]) << 8			      \
   | ((__const unsigned char *) (src))[idx])


/* Copy N bytes of SRC to DEST.  */
#define _HAVE_STRING_ARCH_memcpy 1
#define memcpy(dest, src, n) \
  (__extension__ (__builtin_constant_p (n)				      \
		  ? __memcpy_c ((dest), (src), (n))			      \
		  : __memcpy_g ((dest), (src), (n))))
#define __memcpy_c(dest, src, n) \
  ((n) == 0								      \
   ? (dest)								      \
   : (((n) % 4 == 0)							      \
      ? __memcpy_by4 (dest, src, n)					      \
      : (((n) % 2 == 0)							      \
	 ? __memcpy_by2 (dest, src, n)					      \
	 : __memcpy_g (dest, src, n))))

__STRING_INLINE void *__memcpy_by4 (void *__dest, __const void *__src,
				    size_t __n);

__STRING_INLINE void *
__memcpy_by4 (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1;
  register void *__tmp = __dest;
  __asm__ __volatile__
    ("1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b"
     : "=&r" (__d0), "=&r" (__tmp), "=&r" (__src), "=&r" (__d1)
     : "1" (__tmp), "2" (__src), "3" (__n / 4)
     : "memory", "cc");
  return __dest;
}

__STRING_INLINE void *__memcpy_by2 (void *__dest, __const void *__src,
				    size_t __n);

__STRING_INLINE void *
__memcpy_by2 (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1;
  register void *__tmp = __dest;
  __asm__ __volatile__
    ("shrl	$1,%3\n\t"
     "jz	2f\n"                 /* only a word */
     "1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b\n"
     "2:\n\t"
     "movw	(%2),%w0\n\t"
     "movw	%w0,(%1)"
     : "=&q" (__d0), "=&r" (__tmp), "=&r" (__src), "=&r" (__d1)
     : "1" (__tmp), "2" (__src), "3" (__n / 2)
     : "memory", "cc");
  return __dest;
}

__STRING_INLINE void *__memcpy_g (void *__dest, __const void *__src,
				  size_t __n);

__STRING_INLINE void *
__memcpy_g (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  register void *__tmp = __dest;
  __asm__ __volatile__
    ("cld\n\t"
     "shrl	$1,%%ecx\n\t"
     "jnc	1f\n\t"
     "movsb\n"
     "1:\n\t"
     "shrl	$1,%%ecx\n\t"
     "jnc	2f\n\t"
     "movsw\n"
     "2:\n\t"
     "rep; movsl"
     : "=&c" (__d0), "=&D" (__d1), "=&S" (__d2),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__dest)
     : "0" (__n), "1" (__tmp), "2" (__src),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__src)
     : "cc");
  return __dest;
}

#define _HAVE_STRING_ARCH_memmove 1
#ifndef _FORCE_INLINES
/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
#define memmove(dest, src, n) __memmove_g (dest, src, n)

__STRING_INLINE void *__memmove_g (void *, __const void *, size_t)
     __asm__ ("memmove");

__STRING_INLINE void *
__memmove_g (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  register void *__tmp = __dest;
  if (__dest < __src)
    __asm__ __volatile__
      ("cld\n\t"
       "rep; movsb"
       : "=&c" (__d0), "=&S" (__d1), "=&D" (__d2),
	 "=m" ( *(struct { __extension__ char __x[__n]; } *)__dest)
       : "0" (__n), "1" (__src), "2" (__tmp),
	 "m" ( *(struct { __extension__ char __x[__n]; } *)__src));
  else
    __asm__ __volatile__
      ("std\n\t"
       "rep; movsb\n\t"
       "cld"
       : "=&c" (__d0), "=&S" (__d1), "=&D" (__d2),
	 "=m" ( *(struct { __extension__ char __x[__n]; } *)__dest)
       : "0" (__n), "1" (__n - 1 + (__const char *) __src),
	 "2" (__n - 1 + (char *) __tmp),
	 "m" ( *(struct { __extension__ char __x[__n]; } *)__src));
  return __dest;
}
#endif

/* Compare N bytes of S1 and S2.  */
#define _HAVE_STRING_ARCH_memcmp 1
#ifndef _FORCE_INLINES
# ifndef __PIC__
/* gcc has problems to spill registers when using PIC.  */
__STRING_INLINE int
memcmp (__const void *__s1, __const void *__s2, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  register int __res;
  __asm__ __volatile__
    ("cld\n\t"
     "testl %3,%3\n\t"
     "repe; cmpsb\n\t"
     "je	1f\n\t"
     "sbbl	%0,%0\n\t"
     "orl	$1,%0\n"
     "1:"
     : "=&a" (__res), "=&S" (__d0), "=&D" (__d1), "=&c" (__d2)
     : "0" (0), "1" (__s1), "2" (__s2), "3" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s2)
     : "cc");
  return __res;
}
# endif
#endif

/* Set N bytes of S to C.  */
#define _HAVE_STRING_ARCH_memset 1
#define _USE_STRING_ARCH_memset 1
#define memset(s, c, n) \
  (__extension__ (__builtin_constant_p (n) && (n) <= 16			      \
		  ? ((n) == 1						      \
		     ? __memset_c1 ((s), (c))				      \
		     : __memset_gc ((s), (c), (n)))			      \
		  : (__builtin_constant_p (c)				      \
		     ? (__builtin_constant_p (n)			      \
			? __memset_ccn ((s), (c), (n))			      \
			: memset ((s), (c), (n)))			      \
		     : (__builtin_constant_p (n)			      \
			? __memset_gcn ((s), (c), (n))			      \
			: memset ((s), (c), (n))))))

#define __memset_c1(s, c) ({ void *__s = (s);				      \
			     *((unsigned char *) __s) = (unsigned char) (c);  \
			     __s; })

#define __memset_gc(s, c, n) \
  ({ void *__s = (s);							      \
     union {								      \
       unsigned int __ui;						      \
       unsigned short int __usi;					      \
       unsigned char __uc;						      \
     } *__u = __s;							      \
     unsigned int __c = ((unsigned int) ((unsigned char) (c))) * 0x01010101;  \
									      \
     /* We apply a trick here.  `gcc' would implement the following	      \
	assignments using immediate operands.  But this uses to much	      \
	memory (7, instead of 4 bytes).  So we force the value in a	      \
	registers.  */							      \
     if ((n) == 3 || (n) >= 5)						      \
       __asm__ __volatile__ ("" : "=r" (__c) : "0" (__c));		      \
									      \
     /* This `switch' statement will be removed at compile-time.  */	      \
     switch (n)								      \
       {								      \
       case 15:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 11:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 7:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 3:								      \
	 __u->__usi = (unsigned short int) __c;				      \
	 __u = __extension__ ((void *) __u + 2);			      \
	 __u->__uc = (unsigned char) __c;				      \
	 break;								      \
									      \
       case 14:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 10:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 6:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 2:								      \
	 __u->__usi = (unsigned short int) __c;				      \
	 break;								      \
									      \
       case 13:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 9:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 5:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 1:								      \
	 __u->__uc = (unsigned char) __c;				      \
	 break;								      \
									      \
       case 16:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 12:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 8:								      \
	 __u->__ui = __c;						      \
	 __u = __extension__ ((void *) __u + 4);			      \
       case 4:								      \
	 __u->__ui = __c;						      \
       case 0:								      \
	 break;								      \
       }								      \
									      \
     __s; })

#define __memset_ccn(s, c, n) \
  (((n) % 4 == 0)							      \
   ? __memset_ccn_by4 (s, ((unsigned int) ((unsigned char) (c))) * 0x01010101,\
		       n)						      \
   : (((n) % 2 == 0)							      \
      ? __memset_ccn_by2 (s,						      \
			  ((unsigned int) ((unsigned char) (c))) * 0x01010101,\
			   n)						      \
      : memset (s, c, n)))

__STRING_INLINE void *__memset_ccn_by4 (void *__s, unsigned int __c,
					size_t __n);

__STRING_INLINE void *
__memset_ccn_by4 (void *__s, unsigned int __c, size_t __n)
{
  register void *__tmp = __s;
  register unsigned long int __d0;
#ifdef __i686__
  __asm__ __volatile__
    ("cld\n\t"
     "rep; stosl"
     : "=&a" (__c), "=&D" (__tmp), "=&c" (__d0),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
#else
  __asm__ __volatile__
    ("1:\n\t"
     "movl	%0,(%1)\n\t"
     "addl	$4,%1\n\t"
     "decl	%2\n\t"
     "jnz	1b\n"
     : "=&r" (__c), "=&r" (__tmp), "=&r" (__d0),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
#endif
  return __s;
}

__STRING_INLINE void *__memset_ccn_by2 (void *__s, unsigned int __c,
					size_t __n);

__STRING_INLINE void *
__memset_ccn_by2 (void *__s, unsigned int __c, size_t __n)
{
  register unsigned long int __d0, __d1;
  register void *__tmp = __s;
#ifdef __i686__
  __asm__ __volatile__
    ("cld\n\t"
     "rep; stosl\n"
     "stosw"
     : "=&a" (__d0), "=&D" (__tmp), "=&c" (__d1),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
#else
  __asm__ __volatile__
    ("1:\tmovl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%2\n\t"
     "jnz	1b\n"
     "movw	%w0,(%1)"
     : "=&q" (__d0), "=&r" (__tmp), "=&r" (__d1),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
#endif
  return __s;
}

#define __memset_gcn(s, c, n) \
  (((n) % 4 == 0)							      \
   ? __memset_gcn_by4 (s, c, n)						      \
   : (((n) % 2 == 0)							      \
      ? __memset_gcn_by2 (s, c, n)					      \
      : memset (s, c, n)))

__STRING_INLINE void *__memset_gcn_by4 (void *__s, int __c, size_t __n);

__STRING_INLINE void *
__memset_gcn_by4 (void *__s, int __c, size_t __n)
{
  register void *__tmp = __s;
  register unsigned long int __d0;
  __asm__ __volatile__
    ("movb	%b0,%h0\n"
     "pushw	%w0\n\t"
     "shll	$16,%0\n\t"
     "popw	%w0\n"
     "1:\n\t"
     "movl	%0,(%1)\n\t"
     "addl	$4,%1\n\t"
     "decl	%2\n\t"
     "jnz	1b\n"
     : "=&q" (__c), "=&r" (__tmp), "=&r" (__d0),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
  return __s;
}

__STRING_INLINE void *__memset_gcn_by2 (void *__s, int __c, size_t __n);

__STRING_INLINE void *
__memset_gcn_by2 (void *__s, int __c, size_t __n)
{
  register unsigned long int __d0, __d1;
  register void *__tmp = __s;
  __asm__ __volatile__
    ("movb	%b0,%h0\n\t"
     "pushw	%w0\n\t"
     "shll	$16,%0\n\t"
     "popw	%w0\n"
     "1:\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%2\n\t"
     "jnz	1b\n"
     "movw	%w0,(%1)"
     : "=&q" (__d0), "=&r" (__tmp), "=&r" (__d1),
       "=m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "0" ((unsigned int) __c), "1" (__tmp), "2" (__n / 4)
     : "cc");
  return __s;
}


/* Search N bytes of S for C.  */
#define _HAVE_STRING_ARCH_memchr 1
#ifndef _FORCE_INLINES
__STRING_INLINE void *
memchr (__const void *__s, int __c, size_t __n)
{
  register unsigned long int __d0;
#ifdef __i686__
  register unsigned long int __d1;
#endif
  register unsigned char *__res;
  if (__n == 0)
    return NULL;
#ifdef __i686__
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "cmovne %2,%0"
     : "=D" (__res), "=&c" (__d0), "=&r" (__d1)
     : "a" (__c), "0" (__s), "1" (__n), "2" (1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
#else
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "je	1f\n\t"
     "movl	$1,%0\n"
     "1:"
     : "=D" (__res), "=&c" (__d0)
     : "a" (__c), "0" (__s), "1" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
#endif
  return __res - 1;
}
#endif

#define _HAVE_STRING_ARCH_memrchr 1
#ifndef _FORCE_INLINES
__STRING_INLINE void *__memrchr (__const void *__s, int __c, size_t __n);

__STRING_INLINE void *
__memrchr (__const void *__s, int __c, size_t __n)
{
  register unsigned long int __d0;
# ifdef __i686__
  register unsigned long int __d1;
# endif
  register void *__res;
  if (__n == 0)
    return NULL;
# ifdef __i686__
  __asm__ __volatile__
    ("std\n\t"
     "repne; scasb\n\t"
     "cmovne %2,%0\n\t"
     "cld\n\t"
     "incl %0"
     : "=D" (__res), "=&c" (__d0), "=&r" (__d1)
     : "a" (__c), "0" (__s + __n - 1), "1" (__n), "2" (-1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
# else
  __asm__ __volatile__
    ("std\n\t"
     "repne; scasb\n\t"
     "je 1f\n\t"
     "orl $-1,%0\n"
     "1:\tcld\n\t"
     "incl %0"
     : "=D" (__res), "=&c" (__d0)
     : "a" (__c), "0" (__s + __n - 1), "1" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
# endif
  return __res;
}
# ifdef __USE_GNU
#  define memrchr(s, c, n) __memrchr ((s), (c), (n))
# endif
#endif

/* Return pointer to C in S.  */
#define _HAVE_STRING_ARCH_rawmemchr 1
__STRING_INLINE void *__rawmemchr (const void *__s, int __c);

#ifndef _FORCE_INLINES
__STRING_INLINE void *
__rawmemchr (const void *__s, int __c)
{
  register unsigned long int __d0;
  register unsigned char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     : "=D" (__res), "=&c" (__d0)
     : "a" (__c), "0" (__s), "1" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}
# ifdef __USE_GNU
__STRING_INLINE void *
rawmemchr (const void *__s, int __c)
{
  return __rawmemchr (__s, __c);
}
# endif	/* use GNU */
#endif


/* Return the length of S.  */
#define _HAVE_STRING_ARCH_strlen 1
#define strlen(str) \
  (__extension__ (__builtin_constant_p (str)				      \
		  ? __builtin_strlen (str)				      \
		  : __strlen_g (str)))
__STRING_INLINE size_t __strlen_g (__const char *__str);

__STRING_INLINE size_t
__strlen_g (__const char *__str)
{
  register char __dummy;
  register __const char *__tmp = __str;
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%0),%b1\n\t"
     "leal	1(%0),%0\n\t"
     "testb	%b1,%b1\n\t"
     "jne	1b"
     : "=r" (__tmp), "=&q" (__dummy)
     : "0" (__str),
       "m" ( *(struct { char __x[0xfffffff]; } *)__str)
     : "cc" );
  return __tmp - __str - 1;
}


/* Copy SRC to DEST.  */
#define _HAVE_STRING_ARCH_strcpy 1
#define strcpy(dest, src) \
  (__extension__ (__builtin_constant_p (src)				      \
		  ? (sizeof ((src)[0]) == 1 && strlen (src) + 1 <= 8	      \
		     ? __strcpy_a_small ((dest), (src), strlen (src) + 1)     \
		     : (char *) memcpy ((char *) (dest),		      \
					(__const char *) (src),		      \
					strlen (src) + 1))		      \
		  : __strcpy_g ((dest), (src))))

#define __strcpy_a_small(dest, src, srclen) \
  (__extension__ ({ char *__dest = (dest);				      \
		    union {						      \
		      unsigned int __ui;				      \
		      unsigned short int __usi;				      \
		      unsigned char __uc;				      \
		      char __c;						      \
		    } *__u = (void *) __dest;				      \
		    switch (srclen)					      \
		      {							      \
		      case 1:						      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 2:						      \
			__u->__usi = __STRING_SMALL_GET16 (src, 0);	      \
			break;						      \
		      case 3:						      \
			__u->__usi = __STRING_SMALL_GET16 (src, 0);	      \
			__u = __extension__ ((void *) __u + 2);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 4:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			break;						      \
		      case 5:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 6:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__usi = __STRING_SMALL_GET16 (src, 4);	      \
			break;						      \
		      case 7:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__usi = __STRING_SMALL_GET16 (src, 4);	      \
			__u = __extension__ ((void *) __u + 2);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 8:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__ui = __STRING_SMALL_GET32 (src, 4);	      \
			break;						      \
		      }							      \
		    (char *) __dest; }))

__STRING_INLINE char *__strcpy_g (char *__dest, __const char *__src);

__STRING_INLINE char *
__strcpy_g (char *__dest, __const char *__src)
{
  register char *__tmp = __dest;
  register char __dummy;
  __asm__ __volatile__
    (
     "1:\n\t"
     "movb	(%0),%b2\n\t"
     "leal	1(%0),%0\n\t"
     "movb	%b2,(%1)\n\t"
     "leal	1(%1),%1\n\t"
     "testb	%b2,%b2\n\t"
     "jne	1b"
     : "=&r" (__src), "=&r" (__tmp), "=&q" (__dummy),
       "=m" ( *(struct { char __x[0xfffffff]; } *)__dest)
     : "0" (__src), "1" (__tmp),
       "m" ( *(struct { char __x[0xfffffff]; } *)__src)
     : "cc");
  return __dest;
}


#ifdef __USE_GNU
# define _HAVE_STRING_ARCH_stpcpy 1
/* Copy SRC to DEST.  */
# define __stpcpy(dest, src) \
  (__extension__ (__builtin_constant_p (src)				      \
		  ? (strlen (src) + 1 <= 8				      \
		     ? __stpcpy_a_small ((dest), (src), strlen (src) + 1)     \
		     : __stpcpy_c ((dest), (src), strlen (src) + 1))	      \
		  : __stpcpy_g ((dest), (src))))
# define __stpcpy_c(dest, src, srclen) \
  ((srclen) % 4 == 0							      \
   ? __mempcpy_by4 (dest, src, srclen) - 1				      \
   : ((srclen) % 2 == 0							      \
      ? __mempcpy_by2 (dest, src, srclen) - 1				      \
      : __mempcpy_byn (dest, src, srclen) - 1))

/* In glibc itself we use this symbol for namespace reasons.  */
# define stpcpy(dest, src) __stpcpy ((dest), (src))

# define __stpcpy_a_small(dest, src, srclen) \
  (__extension__ ({ union {						      \
		      unsigned int __ui;				      \
		      unsigned short int __usi;				      \
		      unsigned char __uc;				      \
		      char __c;						      \
		    } *__u = (void *) (dest);				      \
		    switch (srclen)					      \
		      {							      \
		      case 1:						      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 2:						      \
			__u->__usi = __STRING_SMALL_GET16 (src, 0);	      \
			__u = __extension__ ((void *) __u + 1);		      \
			break;						      \
		      case 3:						      \
			__u->__usi = __STRING_SMALL_GET16 (src, 0);	      \
			__u = __extension__ ((void *) __u + 2);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 4:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 3);		      \
			break;						      \
		      case 5:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 6:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__usi = __STRING_SMALL_GET16 (src, 4);	      \
			__u = __extension__ ((void *) __u + 1);		      \
			break;						      \
		      case 7:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__usi = __STRING_SMALL_GET16 (src, 4);	      \
			__u = __extension__ ((void *) __u + 2);		      \
			__u->__uc = '\0';				      \
			break;						      \
		      case 8:						      \
			__u->__ui = __STRING_SMALL_GET32 (src, 0);	      \
			__u = __extension__ ((void *) __u + 4);		      \
			__u->__ui = __STRING_SMALL_GET32 (src, 4);	      \
			__u = __extension__ ((void *) __u + 3);		      \
			break;						      \
		      }							      \
		    (char *) __u; }))

__STRING_INLINE char *__mempcpy_by4 (char *__dest, __const char *__src,
				     size_t __srclen);

__STRING_INLINE char *
__mempcpy_by4 (char *__dest, __const char *__src, size_t __srclen)
{
  register char *__tmp = __dest;
  register unsigned long int __d0, __d1;
  __asm__ __volatile__
    ("1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b"
     : "=&r" (__d0), "=r" (__tmp), "=&r" (__src), "=&r" (__d1)
     : "1" (__tmp), "2" (__src), "3" (__srclen / 4)
     : "memory", "cc");
  return __tmp;
}

__STRING_INLINE char *__mempcpy_by2 (char *__dest, __const char *__src,
				     size_t __srclen);

__STRING_INLINE char *
__mempcpy_by2 (char *__dest, __const char *__src, size_t __srclen)
{
  register char *__tmp = __dest;
  register unsigned long int __d0, __d1;
  __asm__ __volatile__
    ("shrl	$1,%3\n\t"
     "jz	2f\n"                 /* only a word */
     "1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b\n"
     "2:\n\t"
     "movw	(%2),%w0\n\t"
     "movw	%w0,(%1)"
     : "=&q" (__d0), "=r" (__tmp), "=&r" (__src), "=&r" (__d1),
       "=m" ( *(struct { __extension__ char __x[__srclen]; } *)__dest)
     : "1" (__tmp), "2" (__src), "3" (__srclen / 2),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  return __tmp + 2;
}

__STRING_INLINE char *__mempcpy_byn (char *__dest, __const char *__src,
				     size_t __srclen);

__STRING_INLINE char *
__mempcpy_byn (char *__dest, __const char *__src, size_t __srclen)
{
  register unsigned long __d0, __d1;
  register char *__tmp = __dest;
  __asm__ __volatile__
    ("cld\n\t"
     "shrl	$1,%%ecx\n\t"
     "jnc	1f\n\t"
     "movsb\n"
     "1:\n\t"
     "shrl	$1,%%ecx\n\t"
     "jnc	2f\n\t"
     "movsw\n"
     "2:\n\t"
     "rep; movsl"
     : "=D" (__tmp), "=&c" (__d0), "=&S" (__d1),
       "=m" ( *(struct { __extension__ char __x[__srclen]; } *)__dest)
     : "0" (__tmp), "1" (__srclen), "2" (__src),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  return __tmp;
}

__STRING_INLINE char *__stpcpy_g (char *__dest, __const char *__src);

__STRING_INLINE char *
__stpcpy_g (char *__dest, __const char *__src)
{
  register char *__tmp = __dest;
  register char __dummy;
  __asm__ __volatile__
    (
     "1:\n\t"
     "movb	(%0),%b2\n\t"
     "leal	1(%0),%0\n\t"
     "movb	%b2,(%1)\n\t"
     "leal	1(%1),%1\n\t"
     "testb	%b2,%b2\n\t"
     "jne	1b"
     : "=&r" (__src), "=r" (__tmp), "=&q" (__dummy),
       "=m" ( *(struct { char __x[0xfffffff]; } *)__dest)
     : "0" (__src), "1" (__tmp),
       "m" ( *(struct { char __x[0xfffffff]; } *)__src)
     : "cc");
  return __tmp - 1;
}
#endif


/* Copy no more than N characters of SRC to DEST.  */
#define _HAVE_STRING_ARCH_strncpy 1
#define strncpy(dest, src, n) \
  (__extension__ (__builtin_constant_p (src)				      \
		  ? ((strlen (src) + 1 >= ((size_t) (n))		      \
		      ? (char *) memcpy ((char *) (dest),		      \
					 (__const char *) (src), n)	      \
		      : __strncpy_cg ((dest), (src), strlen (src) + 1, n)))   \
		  : __strncpy_gg ((dest), (src), n)))
#define __strncpy_cg(dest, src, srclen, n) \
  (((srclen) % 4 == 0)							      \
   ? __strncpy_by4 (dest, src, srclen, n)				      \
   : (((srclen) % 2 == 0)						      \
      ? __strncpy_by2 (dest, src, srclen, n)				      \
      : __strncpy_byn (dest, src, srclen, n)))

__STRING_INLINE char *__strncpy_by4 (char *__dest, __const char __src[],
				     size_t __srclen, size_t __n);

__STRING_INLINE char *
__strncpy_by4 (char *__dest, __const char __src[], size_t __srclen, size_t __n)
{
  register char *__tmp = __dest;
  register int __dummy1, __dummy2;
  __asm__ __volatile__
    ("1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b"
     : "=&r" (__dummy1), "=r" (__tmp), "=&r" (__src), "=&r" (__dummy2),
       "=m" ( *(struct { __extension__ char __x[__srclen]; } *)__dest)
     : "1" (__tmp), "2" (__src), "3" (__srclen / 4),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  (void) memset (__tmp, '\0', __n - __srclen);
  return __dest;
}

__STRING_INLINE char *__strncpy_by2 (char *__dest, __const char __src[],
				     size_t __srclen, size_t __n);

__STRING_INLINE char *
__strncpy_by2 (char *__dest, __const char __src[], size_t __srclen, size_t __n)
{
  register char *__tmp = __dest;
  register int __dummy1, __dummy2;
  __asm__ __volatile__
    ("shrl	$1,%3\n\t"
     "jz	2f\n"                 /* only a word */
     "1:\n\t"
     "movl	(%2),%0\n\t"
     "leal	4(%2),%2\n\t"
     "movl	%0,(%1)\n\t"
     "leal	4(%1),%1\n\t"
     "decl	%3\n\t"
     "jnz	1b\n"
     "2:\n\t"
     "movw	(%2),%w0\n\t"
     "movw	%w0,(%1)\n\t"
     : "=&q" (__dummy1), "=r" (__tmp), "=&r" (__src), "=&r" (__dummy2),
       "=m" ( *(struct { __extension__ char __x[__srclen]; } *)__dest)
     : "1" (__tmp), "2" (__src), "3" (__srclen / 2),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  (void) memset (__tmp + 2, '\0', __n - __srclen);
  return __dest;
}

__STRING_INLINE char *__strncpy_byn (char *__dest, __const char __src[],
				     size_t __srclen, size_t __n);

__STRING_INLINE char *
__strncpy_byn (char *__dest, __const char __src[], size_t __srclen, size_t __n)
{
  register unsigned long int __d0, __d1;
  register char *__tmp = __dest;
  __asm__ __volatile__
    ("cld\n\t"
     "shrl	$1,%1\n\t"
     "jnc	1f\n\t"
     "movsb\n"
     "1:\n\t"
     "shrl	$1,%1\n\t"
     "jnc	2f\n\t"
     "movsw\n"
     "2:\n\t"
     "rep; movsl"
     : "=D" (__tmp), "=&c" (__d0), "=&S" (__d1),
       "=m" ( *(struct { __extension__ char __x[__srclen]; } *)__dest)
     : "1" (__srclen), "0" (__tmp),"2" (__src),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  (void) memset (__tmp, '\0', __n - __srclen);
  return __dest;
}

__STRING_INLINE char *__strncpy_gg (char *__dest, __const char *__src,
				    size_t __n);

__STRING_INLINE char *
__strncpy_gg (char *__dest, __const char *__src, size_t __n)
{
  register char *__tmp = __dest;
  register char __dummy;
  if (__n > 0)
    __asm__ __volatile__
      ("1:\n\t"
       "movb	(%0),%2\n\t"
       "incl	%0\n\t"
       "movb	%2,(%1)\n\t"
       "incl	%1\n\t"
       "decl	%3\n\t"
       "je	3f\n\t"
       "testb	%2,%2\n\t"
       "jne	1b\n\t"
       "2:\n\t"
       "movb	%2,(%1)\n\t"
       "incl	%1\n\t"
       "decl	%3\n\t"
       "jne	2b\n\t"
       "3:"
       : "=&r" (__src), "=&r" (__tmp), "=&q" (__dummy), "=&r" (__n)
       : "0" (__src), "1" (__tmp), "3" (__n)
       : "memory", "cc");

  return __dest;
}


/* Append SRC onto DEST.  */
#define _HAVE_STRING_ARCH_strcat 1
#define strcat(dest, src) \
  (__extension__ (__builtin_constant_p (src)				      \
		  ? __strcat_c ((dest), (src), strlen (src) + 1)	      \
		  : __strcat_g ((dest), (src))))

__STRING_INLINE char *__strcat_c (char *__dest, __const char __src[],
				  size_t __srclen);

__STRING_INLINE char *
__strcat_c (char *__dest, __const char __src[], size_t __srclen)
{
#ifdef __i686__
  register unsigned long int __d0;
  register char *__tmp;
  __asm__ __volatile__
    ("repne; scasb"
     : "=D" (__tmp), "=&c" (__d0),
       "=m" ( *(struct { char __x[0xfffffff]; } *)__dest)
     : "0" (__dest), "1" (0xffffffff), "a" (0),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
  --__tmp;
#else
  register char *__tmp = __dest - 1;
  __asm__ __volatile__
    ("1:\n\t"
     "incl	%0\n\t"
     "cmpb	$0,(%0)\n\t"
     "jne	1b\n"
     : "=r" (__tmp),
       "=m" ( *(struct { char __x[0xfffffff]; } *)__dest)
     : "0" (__tmp),
       "m" ( *(struct { __extension__ char __x[__srclen]; } *)__src)
     : "cc");
#endif
  (void) memcpy (__tmp, __src, __srclen);
  return __dest;
}

__STRING_INLINE char *__strcat_g (char *__dest, __const char *__src);

__STRING_INLINE char *
__strcat_g (char *__dest, __const char *__src)
{
  register char *__tmp = __dest - 1;
  register char __dummy;
  __asm__ __volatile__
    ("1:\n\t"
     "incl	%1\n\t"
     "cmpb	$0,(%1)\n\t"
     "jne	1b\n"
     "2:\n\t"
     "movb	(%2),%b0\n\t"
     "incl	%2\n\t"
     "movb	%b0,(%1)\n\t"
     "incl	%1\n\t"
     "testb	%b0,%b0\n\t"
     "jne	2b\n"
     : "=&q" (__dummy), "=&r" (__tmp), "=&r" (__src),
       "=m" ( *(struct { char __x[0xfffffff]; } *)__dest)
     : "1"  (__tmp), "2"  (__src),
       "m" ( *(struct { char __x[0xfffffff]; } *)__src)
     : "memory", "cc");
  return __dest;
}


/* Append no more than N characters from SRC onto DEST.  */
#define _HAVE_STRING_ARCH_strncat 1
#define strncat(dest, src, n) \
  (__extension__ ({ char *__dest = (dest);				      \
		    __builtin_constant_p (src) && __builtin_constant_p (n)    \
		    ? (strlen (src) < ((size_t) (n))			      \
		       ? strcat (__dest, (src))				      \
		       : (*(char *)__mempcpy (strchr (__dest, '\0'),	      \
					       (__const char *) (src),	      \
					      (n)) = 0, __dest))	      \
		    : __strncat_g (__dest, (src), (n)); }))

__STRING_INLINE char *__strncat_g (char *__dest, __const char __src[],
				   size_t __n);

__STRING_INLINE char *
__strncat_g (char *__dest, __const char __src[], size_t __n)
{
  register char *__tmp = __dest;
  register char __dummy;
#ifdef __i686__
  __asm__ __volatile__
    ("repne; scasb\n"
     "movl %4, %3\n\t"
     "decl %1\n\t"
     "1:\n\t"
     "subl	$1,%3\n\t"
     "jc	2f\n\t"
     "movb	(%2),%b0\n\t"
     "movsb\n\t"
     "testb	%b0,%b0\n\t"
     "jne	1b\n\t"
     "decl	%1\n"
     "2:\n\t"
     "movb	$0,(%1)"
     : "=&a" (__dummy), "=&D" (__tmp), "=&S" (__src), "=&c" (__n)
     :  "g" (__n), "0" (0), "1" (__tmp), "2" (__src), "3" (0xffffffff)
     : "memory", "cc");
#else
  --__tmp;
  __asm__ __volatile__
    ("1:\n\t"
     "cmpb	$0,1(%1)\n\t"
     "leal	1(%1),%1\n\t"
     "jne	1b\n"
     "2:\n\t"
     "subl	$1,%3\n\t"
     "jc	3f\n\t"
     "movb	(%2),%b0\n\t"
     "leal	1(%2),%2\n\t"
     "movb	%b0,(%1)\n\t"
     "leal	1(%1),%1\n\t"
     "testb	%b0,%b0\n\t"
     "jne	2b\n\t"
     "decl	%1\n"
     "3:\n\t"
     "movb	$0,(%1)"
     : "=&q" (__dummy), "=&r" (__tmp), "=&r" (__src), "=&r" (__n)
     : "1" (__tmp), "2" (__src), "3" (__n)
     : "memory", "cc");
#endif
  return __dest;
}


/* Compare S1 and S2.  */
#define _HAVE_STRING_ARCH_strcmp 1
#define strcmp(s1, s2) \
  (__extension__ (__builtin_constant_p (s1) && __builtin_constant_p (s2)      \
		  && (sizeof ((s1)[0]) != 1 || strlen (s1) >= 4)	      \
		  && (sizeof ((s2)[0]) != 1 || strlen (s2) >= 4)	      \
		  ? memcmp ((__const char *) (s1), (__const char *) (s2),     \
			    (strlen (s1) < strlen (s2)			      \
			     ? strlen (s1) : strlen (s2)) + 1)		      \
		  : (__builtin_constant_p (s1) && sizeof ((s1)[0]) == 1	      \
		     && sizeof ((s2)[0]) == 1 && strlen (s1) < 4	      \
		     ? (__builtin_constant_p (s2) && sizeof ((s2)[0]) == 1    \
			? __strcmp_cc ((__const unsigned char *) (s1),	      \
				       (__const unsigned char *) (s2),	      \
				       strlen (s1))			      \
			: __strcmp_cg ((__const unsigned char *) (s1),	      \
				       (__const unsigned char *) (s2),	      \
				       strlen (s1)))			      \
		     : (__builtin_constant_p (s2) && sizeof ((s1)[0]) == 1    \
			&& sizeof ((s2)[0]) == 1 && strlen (s2) < 4	      \
			? (__builtin_constant_p (s1)			      \
			   ? __strcmp_cc ((__const unsigned char *) (s1),     \
					  (__const unsigned char *) (s2),     \
					  strlen (s2))			      \
			   : __strcmp_gc ((__const unsigned char *) (s1),     \
					  (__const unsigned char *) (s2),     \
					  strlen (s2)))			      \
			: __strcmp_gg ((s1), (s2))))))

#define __strcmp_cc(s1, s2, l) \
  (__extension__ ({ register int __result = (s1)[0] - (s2)[0];		      \
		    if (l > 0 && __result == 0)				      \
		      {							      \
			__result = (s1)[1] - (s2)[1];			      \
			if (l > 1 && __result == 0)			      \
			  {						      \
			    __result = (s1)[2] - (s2)[2];		      \
			    if (l > 2 && __result == 0)			      \
			      __result = (s1)[3] - (s2)[3];		      \
			  }						      \
		      }							      \
		    __result; }))

#define __strcmp_cg(s1, s2, l1) \
  (__extension__ ({ __const unsigned char *__s2 = (s2);			      \
		    register int __result = (s1)[0] - __s2[0];		      \
		    if (l1 > 0 && __result == 0)			      \
		      {							      \
			__result = (s1)[1] - __s2[1];			      \
			if (l1 > 1 && __result == 0)			      \
			  {						      \
			    __result = (s1)[2] - __s2[2];		      \
			    if (l1 > 2 && __result == 0)		      \
			      __result = (s1)[3] - __s2[3];		      \
			  }						      \
		      }							      \
		    __result; }))

#define __strcmp_gc(s1, s2, l2) \
  (__extension__ ({ __const unsigned char *__s1 = (s1);			      \
		    register int __result = __s1[0] - (s2)[0];		      \
		    if (l2 > 0 && __result == 0)			      \
		      {							      \
			__result = __s1[1] - (s2)[1];			      \
			if (l2 > 1 && __result == 0)			      \
			  {						      \
			    __result = __s1[2] - (s2)[2];		      \
			    if (l2 > 2 && __result == 0)		      \
			      __result = __s1[3] - (s2)[3];		      \
			  }						      \
		      }							      \
		    __result; }))

__STRING_INLINE int __strcmp_gg (__const char *__s1, __const char *__s2);

__STRING_INLINE int
__strcmp_gg (__const char *__s1, __const char *__s2)
{
  register int __res;
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%1),%b0\n\t"
     "leal	1(%1),%1\n\t"
     "cmpb	%b0,(%2)\n\t"
     "jne	2f\n\t"
     "leal	1(%2),%2\n\t"
     "testb	%b0,%b0\n\t"
     "jne	1b\n\t"
     "xorl	%0,%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "movl	$1,%0\n\t"
     "jb	3f\n\t"
     "negl	%0\n"
     "3:"
     : "=q" (__res), "=&r" (__s1), "=&r" (__s2)
     : "1" (__s1), "2" (__s2),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s1),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s2)
     : "cc");
  return __res;
}


/* Compare N characters of S1 and S2.  */
#define _HAVE_STRING_ARCH_strncmp 1
#define strncmp(s1, s2, n) \
  (__extension__ (__builtin_constant_p (s1) && strlen (s1) < ((size_t) (n))   \
		  ? strcmp ((s1), (s2))					      \
		  : (__builtin_constant_p (s2) && strlen (s2) < ((size_t) (n))\
		     ? strcmp ((s1), (s2))				      \
		     : __strncmp_g ((s1), (s2), (n)))))

__STRING_INLINE int __strncmp_g (__const char *__s1, __const char *__s2,
				 size_t __n);

__STRING_INLINE int
__strncmp_g (__const char *__s1, __const char *__s2, size_t __n)
{
  register int __res;
  __asm__ __volatile__
    ("1:\n\t"
     "subl	$1,%3\n\t"
     "jc	2f\n\t"
     "movb	(%1),%b0\n\t"
     "incl	%1\n\t"
     "cmpb	%b0,(%2)\n\t"
     "jne	3f\n\t"
     "incl	%2\n\t"
     "testb	%b0,%b0\n\t"
     "jne	1b\n"
     "2:\n\t"
     "xorl	%0,%0\n\t"
     "jmp	4f\n"
     "3:\n\t"
     "movl	$1,%0\n\t"
     "jb	4f\n\t"
     "negl	%0\n"
     "4:"
     : "=q" (__res), "=&r" (__s1), "=&r" (__s2), "=&r" (__n)
     : "1"  (__s1), "2"  (__s2),  "3" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s2)
     : "cc");
  return __res;
}


/* Find the first occurrence of C in S.  */
#define _HAVE_STRING_ARCH_strchr 1
#define _USE_STRING_ARCH_strchr 1
#define strchr(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? ((c) == '\0'					      \
		     ? (char *) __rawmemchr ((s), (c))			      \
		     : __strchr_c ((s), ((c) & 0xff) << 8))		      \
		  : __strchr_g ((s), (c))))

__STRING_INLINE char *__strchr_c (__const char *__s, int __c);

__STRING_INLINE char *
__strchr_c (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%0),%%al\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "leal	1(%0),%0\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "xorl	%0,%0\n"
     "2:"
     : "=r" (__res), "=&a" (__d0)
     : "0" (__s), "1" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}

__STRING_INLINE char *__strchr_g (__const char *__s, int __c);

__STRING_INLINE char *
__strchr_g (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("movb	%%al,%%ah\n"
     "1:\n\t"
     "movb	(%0),%%al\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "leal	1(%0),%0\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "xorl	%0,%0\n"
     "2:"
     : "=r" (__res), "=&a" (__d0)
     : "0" (__s), "1" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}


/* Find the first occurrence of C in S or the final NUL byte.  */
#define _HAVE_STRING_ARCH_strchrnul 1
#define __strchrnul(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? ((c) == '\0'					      \
		     ? (char *) __rawmemchr ((s), c)			      \
		     : __strchrnul_c ((s), ((c) & 0xff) << 8))		      \
		  : __strchrnul_g ((s), c)))

__STRING_INLINE char *__strchrnul_c (__const char *__s, int __c);

__STRING_INLINE char *
__strchrnul_c (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%0),%%al\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "leal	1(%0),%0\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "decl	%0\n"
     "2:"
     : "=r" (__res), "=&a" (__d0)
     : "0" (__s), "1" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}

__STRING_INLINE char *__strchrnul_g (__const char *__s, int __c);

__STRING_INLINE char *
__strchrnul_g (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("movb	%%al,%%ah\n"
     "1:\n\t"
     "movb	(%0),%%al\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "leal	1(%0),%0\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "decl	%0\n"
     "2:"
     : "=r" (__res), "=&a" (__d0)
     : "0" (__s), "1" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}
#ifdef __USE_GNU
# define strchrnul(s, c) __strchrnul ((s), (c))
#endif


#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED
/* Find the first occurrence of C in S.  This is the BSD name.  */
# define _HAVE_STRING_ARCH_index 1
# define index(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? __strchr_c ((s), ((c) & 0xff) << 8)			      \
		  : __strchr_g ((s), (c))))
#endif


/* Find the last occurrence of C in S.  */
#define _HAVE_STRING_ARCH_strrchr 1
#define strrchr(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? __strrchr_c ((s), ((c) & 0xff) << 8)		      \
		  : __strrchr_g ((s), (c))))

#ifdef __i686__
__STRING_INLINE char *__strrchr_c (__const char *__s, int __c);

__STRING_INLINE char *
__strrchr_c (__const char *__s, int __c)
{
  register unsigned long int __d0, __d1;
  register char *__res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%h2,%b2\n\t"
     "cmove	%1,%0\n\t"
     "testb	%b2,%b2\n\t"
     "jne 1b"
     : "=d" (__res), "=&S" (__d0), "=&a" (__d1)
     : "0" (1), "1" (__s), "2" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}

__STRING_INLINE char *__strrchr_g (__const char *__s, int __c);

__STRING_INLINE char *
__strrchr_g (__const char *__s, int __c)
{
  register unsigned long int __d0, __d1;
  register char *__res;
  __asm__ __volatile__
    ("movb	%b2,%h2\n"
     "cld\n\t"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%h2,%b2\n\t"
     "cmove	%1,%0\n\t"
     "testb	%b2,%b2\n\t"
     "jne 1b"
     : "=d" (__res), "=&S" (__d0), "=&a" (__d1)
     : "0" (1), "1" (__s), "2" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}
#else
__STRING_INLINE char *__strrchr_c (__const char *__s, int __c);

__STRING_INLINE char *
__strrchr_c (__const char *__s, int __c)
{
  register unsigned long int __d0, __d1;
  register char *__res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "jne	2f\n\t"
     "leal	-1(%%esi),%0\n"
     "2:\n\t"
     "testb	%%al,%%al\n\t"
     "jne 1b"
     : "=d" (__res), "=&S" (__d0), "=&a" (__d1)
     : "0" (0), "1" (__s), "2" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}

__STRING_INLINE char *__strrchr_g (__const char *__s, int __c);

__STRING_INLINE char *
__strrchr_g (__const char *__s, int __c)
{
  register unsigned long int __d0, __d1;
  register char *__res;
  __asm__ __volatile__
    ("movb	%%al,%%ah\n"
     "cld\n\t"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "jne	2f\n\t"
     "leal	-1(%%esi),%0\n"
     "2:\n\t"
     "testb	%%al,%%al\n\t"
     "jne 1b"
     : "=r" (__res), "=&S" (__d0), "=&a" (__d1)
     : "0" (0), "1" (__s), "2" (__c),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}
#endif


#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED
/* Find the last occurrence of C in S.  This is the BSD name.  */
# define _HAVE_STRING_ARCH_rindex 1
# define rindex(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? __strrchr_c ((s), ((c) & 0xff) << 8)		      \
		  : __strrchr_g ((s), (c))))
#endif


/* Return the length of the initial segment of S which
   consists entirely of characters not in REJECT.  */
#define _HAVE_STRING_ARCH_strcspn 1
#define strcspn(s, reject) \
  (__extension__ (__builtin_constant_p (reject) && sizeof ((reject)[0]) == 1  \
		  ? ((reject)[0] == '\0'				      \
		     ? strlen (s)					      \
		     : ((reject)[1] == '\0'				      \
			? __strcspn_c1 ((s), (((reject)[0] << 8) & 0xff00))   \
			: __strcspn_cg ((s), (reject), strlen (reject))))     \
		  : __strcspn_g ((s), (reject))))

__STRING_INLINE size_t __strcspn_c1 (__const char *__s, int __reject);

#ifndef _FORCE_INLINES
__STRING_INLINE size_t
__strcspn_c1 (__const char *__s, int __reject)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%0),%%al\n\t"
     "leal	1(%0),%0\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n"
     "2:"
     : "=r" (__res), "=&a" (__d0)
     : "0" (__s), "1" (__reject),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
#endif

__STRING_INLINE size_t __strcspn_cg (__const char *__s, __const char __reject[],
				    size_t __reject_len);

__STRING_INLINE size_t
__strcspn_cg (__const char *__s, __const char __reject[], size_t __reject_len)
{
  register unsigned long int __d0, __d1, __d2;
  register __const char *__res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%6,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n"
     "2:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "0" (__s), "d" (__reject), "g" (__reject_len)
     : "memory", "cc");
  return (__res - 1) - __s;
}

__STRING_INLINE size_t __strcspn_g (__const char *__s, __const char *__reject);
#ifdef __PIC__

__STRING_INLINE size_t
__strcspn_g (__const char *__s, __const char *__reject)
{
  register unsigned long int __d0, __d1, __d2;
  register __const char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "movl	%4,%%edi\n\t"
     "cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%4,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n"
     "2:\n\t"
     "popl	%%ebx"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "r" (__reject), "0" (__s), "1" (0), "2" (0xffffffff)
     : "memory", "cc");
  return (__res - 1) - __s;
}
#else
__STRING_INLINE size_t
__strcspn_g (__const char *__s, __const char *__reject)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register __const char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%%ebx,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n"
     "2:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2), "=&d" (__d3)
     : "0" (__s), "1" (0), "2" (0xffffffff), "3" (__reject), "b" (__reject)
     /* Clobber memory, otherwise GCC cannot handle this.  */
     : "memory", "cc");
  return (__res - 1) - __s;
}
#endif


/* Return the length of the initial segment of S which
   consists entirely of characters in ACCEPT.  */
#define _HAVE_STRING_ARCH_strspn 1
#define strspn(s, accept) \
  (__extension__ (__builtin_constant_p (accept) && sizeof ((accept)[0]) == 1  \
		  ? ((accept)[0] == '\0'				      \
		     ? ((void) (s), 0)					      \
		     : ((accept)[1] == '\0'				      \
			? __strspn_c1 ((s), (((accept)[0] << 8 ) & 0xff00))   \
			: __strspn_cg ((s), (accept), strlen (accept))))      \
		  : __strspn_g ((s), (accept))))

#ifndef _FORCE_INLINES
__STRING_INLINE size_t __strspn_c1 (__const char *__s, int __accept);

__STRING_INLINE size_t
__strspn_c1 (__const char *__s, int __accept)
{
  register unsigned long int __d0;
  register char *__res;
  /* Please note that __accept never can be '\0'.  */
  __asm__ __volatile__
    ("1:\n\t"
     "movb	(%0),%b1\n\t"
     "leal	1(%0),%0\n\t"
     "cmpb	%h1,%b1\n\t"
     "je	1b"
     : "=r" (__res), "=&q" (__d0)
     : "0" (__s), "1" (__accept),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
#endif

__STRING_INLINE size_t __strspn_cg (__const char *__s, __const char __accept[],
				    size_t __accept_len);

__STRING_INLINE size_t
__strspn_cg (__const char *__s, __const char __accept[], size_t __accept_len)
{
  register unsigned long int __d0, __d1, __d2;
  register __const char *__res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%6,%%ecx\n\t"
     "repne; scasb\n\t"
     "je	1b\n"
     "2:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "0" (__s), "g" (__accept), "g" (__accept_len),
       /* Since we do not know how large the memory we access it, use a
	  really large amount.  */
       "m" ( *(struct { char __x[0xfffffff]; } *)__s),
       "m" ( *(struct { __extension__ char __x[__accept_len]; } *)__accept)
     : "cc");
  return (__res - 1) - __s;
}

__STRING_INLINE size_t __strspn_g (__const char *__s, __const char *__accept);
#ifdef __PIC__

__STRING_INLINE size_t
__strspn_g (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2;
  register __const char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%%edx,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "je	1b\n"
     "2:\n\t"
     "popl	%%ebx"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "d" (__accept), "0" (__s), "1" (0), "2" (0xffffffff), "3" (__accept)
     : "memory", "cc");
  return (__res - 1) - __s;
}
#else
__STRING_INLINE size_t
__strspn_g (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register __const char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%%ebx,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "je	1b\n"
     "2:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2), "=&d" (__d3)
     : "0" (__s), "1" (0), "2" (0xffffffff), "3" (__accept), "b" (__accept)
     : "memory", "cc");
  return (__res - 1) - __s;
}
#endif


/* Find the first occurrence in S of any character in ACCEPT.  */
#define _HAVE_STRING_ARCH_strpbrk 1
#define strpbrk(s, accept) \
  (__extension__ (__builtin_constant_p (accept) && sizeof ((accept)[0]) == 1  \
		  ? ((accept)[0] == '\0'				      \
		     ? ((void) (s), (char *) 0)				      \
		     : ((accept)[1] == '\0'				      \
			? strchr ((s), (accept)[0])			      \
			: __strpbrk_cg ((s), (accept), strlen (accept))))     \
		  : __strpbrk_g ((s), (accept))))

__STRING_INLINE char *__strpbrk_cg (__const char *__s, __const char __accept[],
				    size_t __accept_len);

__STRING_INLINE char *
__strpbrk_cg (__const char *__s, __const char __accept[], size_t __accept_len)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%6,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n\t"
     "decl	%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%0,%0\n"
     "3:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "0" (__s), "d" (__accept), "g" (__accept_len)
     : "memory", "cc");
  return __res;
}

__STRING_INLINE char *__strpbrk_g (__const char *__s, __const char *__accept);
#ifdef __PIC__

__STRING_INLINE char *
__strpbrk_g (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "movl	%%edx,%%edi\n\t"
     "cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%%edx,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n\t"
     "decl	%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%0,%0\n"
     "3:\n\t"
     "popl	%%ebx"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "d" (__accept), "0" (__s), "1" (0), "2" (0xffffffff)
     : "memory", "cc");
  return __res;
}
#else
__STRING_INLINE char *
__strpbrk_g (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("movl	%%ebx,%%edi\n\t"
     "cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "leal	-1(%%ecx),%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%%ebx,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n\t"
     "decl	%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%0,%0\n"
     "3:"
     : "=S" (__res), "=&a" (__d0), "=&c" (__d1), "=&d" (__d2), "=&D" (__d3)
     : "0" (__s), "1" (0), "2" (0xffffffff), "b" (__accept)
     : "memory", "cc");
  return __res;
}
#endif


/* Find the first occurrence of NEEDLE in HAYSTACK.  */
#define _HAVE_STRING_ARCH_strstr 1
#define strstr(haystack, needle) \
  (__extension__ (__builtin_constant_p (needle) && sizeof ((needle)[0]) == 1  \
		  ? ((needle)[0] == '\0'				      \
		     ? (haystack)					      \
		     : ((needle)[1] == '\0'				      \
			? strchr ((haystack), (needle)[0])		      \
			: __strstr_cg ((haystack), (needle),		      \
				       strlen (needle))))		      \
		  : __strstr_g ((haystack), (needle))))

/* Please note that this function need not handle NEEDLEs with a
   length shorter than two.  */
__STRING_INLINE char *__strstr_cg (__const char *__haystack, __const char __needle[],
				   size_t __needle_len);

__STRING_INLINE char *
__strstr_cg (__const char *__haystack, __const char __needle[],
	     size_t __needle_len)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("cld\n" \
     "1:\n\t"
     "movl	%6,%%edi\n\t"
     "movl	%5,%%eax\n\t"
     "movl	%4,%%ecx\n\t"
     "repe; cmpsb\n\t"
     "je	2f\n\t"
     "cmpb	$0,-1(%%esi)\n\t"
     "leal	1(%%eax),%5\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n"
     "2:"
     : "=&a" (__res), "=&S" (__d0), "=&D" (__d1), "=&c" (__d2)
     : "g" (__needle_len), "1" (__haystack), "d" (__needle)
     : "memory", "cc");
  return __res;
}

__STRING_INLINE char *__strstr_g (__const char *__haystack, __const char *__needle);
#ifdef __PIC__

__STRING_INLINE char *
__strstr_g (__const char *__haystack, __const char *__needle)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "pushl	%%ebx\n\t"
     "decl	%%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
     "movl	%%ecx,%%ebx\n"
     "1:\n\t"
     "movl	%%edx,%%edi\n\t"
     "movl	%%esi,%%eax\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repe; cmpsb\n\t"
     "je	2f\n\t"		/* also works for empty string, see above */
     "cmpb	$0,-1(%%esi)\n\t"
     "leal	1(%%eax),%%esi\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n"
     "2:\n\t"
     "popl	%%ebx"
     : "=&a" (__res), "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
     : "0" (0), "1" (0xffffffff), "2" (__haystack), "3" (__needle),
       "d" (__needle)
     : "memory", "cc");
  return __res;
}
#else
__STRING_INLINE char *
__strstr_g (__const char *__haystack, __const char *__needle)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
     "movl	%%ecx,%%edx\n"
     "1:\n\t"
     "movl	%%ebx,%%edi\n\t"
     "movl	%%esi,%%eax\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repe; cmpsb\n\t"
     "je	2f\n\t"		/* also works for empty string, see above */
     "cmpb	$0,-1(%%esi)\n\t"
     "leal	1(%%eax),%%esi\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n"
     "2:"
     : "=&a" (__res), "=&c" (__d0), "=&S" (__d1), "=&D" (__d2), "=&d" (__d3)
     : "0" (0), "1" (0xffffffff), "2" (__haystack), "3" (__needle),
       "b" (__needle)
     : "memory", "cc");
  return __res;
}
#endif


/* Bit find functions.  We define only the i686 version since for the other
   processors gcc generates good code.  */
#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED
# ifdef __i686__
#  define _HAVE_STRING_ARCH_ffs 1
#  define ffs(word) (__builtin_constant_p (word)			      \
		     ? __builtin_ffs (word)				      \
		     : ({ int __cnt, __tmp;				      \
			  __asm__ __volatile__				      \
			    ("bsfl %2,%0\n\t"				      \
			     "cmovel %1,%0"				      \
			     : "=&r" (__cnt), "=r" (__tmp)		      \
			     : "rm" (word), "1" (-1));			      \
			  __cnt + 1; }))

#  ifndef ffsl
#   define ffsl(word) ffs(word)
#  endif
# endif	/* i686 */
#endif	/* BSD || X/Open */

#ifndef _FORCE_INLINES
# undef __STRING_INLINE
#endif

#endif	/* use string inlines && GNU CC */
