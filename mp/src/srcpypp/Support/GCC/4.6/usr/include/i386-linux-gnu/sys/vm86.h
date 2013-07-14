/* Copyright (C) 1996, 1999, 2003 Free Software Foundation, Inc.
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

#ifndef _SYS_VM86_H

#define _SYS_VM86_H	1
#include <features.h>

/* Get constants and data types from kernel header file.  */
#include <asm/vm86.h>

__BEGIN_DECLS

/* Enter virtual 8086 mode.  */
extern int vm86 (unsigned long int __subfunction,
		 struct vm86plus_struct *__info) __THROW;

__END_DECLS

#endif	/* _SYS_VM86_H */
