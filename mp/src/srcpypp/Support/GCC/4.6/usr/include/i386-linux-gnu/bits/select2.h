/* Checking macros for select functions.
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

#ifndef _SYS_SELECT_H
# error "Never include <bits/select2.h> directly; use <sys/select.h> instead."
#endif

/* Helper functions to issue warnings and errors when needed.  */
extern unsigned long int __fdelt_chk (unsigned long int __d);
extern unsigned long int __fdelt_warn (unsigned long int __d)
  __warnattr ("bit outside of fd_set selected");
#undef __FD_ELT
#define	__FD_ELT(d) \
  __extension__								    \
  ({ unsigned long int __d = (d);					    \
     (__builtin_constant_p (__d)					    \
      ? (__d >= __FD_SETSIZE						    \
	 ? __fdelt_warn (__d) : (__d / __NFDBITS))			    \
      : __fdelt_chk (__d)); })
