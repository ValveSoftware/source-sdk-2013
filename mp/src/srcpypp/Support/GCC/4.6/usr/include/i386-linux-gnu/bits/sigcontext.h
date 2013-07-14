/* Copyright (C) 1996, 1997, 1998, 2011 Free Software Foundation, Inc.
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

#if !defined _SIGNAL_H && !defined _SYS_UCONTEXT_H
# error "Never use <bits/sigcontext.h> directly; include <signal.h> instead."
#endif

#ifndef sigcontext_struct
/* Kernel headers before 2.1.1 define a struct sigcontext_struct, but
   we need sigcontext.  */
# define sigcontext_struct sigcontext

# include <asm/sigcontext.h>

/* The Linux kernel headers redefine NULL wrongly, so cleanup afterwards.  */
# define __need_NULL
# include <stddef.h>
#endif
