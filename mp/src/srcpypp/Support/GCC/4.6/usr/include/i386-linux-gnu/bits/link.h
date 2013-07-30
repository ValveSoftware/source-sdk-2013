/* Copyright (C) 2004, 2005, 2009 Free Software Foundation, Inc.
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

#ifndef	_LINK_H
# error "Never include <bits/link.h> directly; use <link.h> instead."
#endif


/* Registers for entry into PLT on IA-32.  */
typedef struct La_i86_regs
{
  uint32_t lr_edx;
  uint32_t lr_ecx;
  uint32_t lr_eax;
  uint32_t lr_ebp;
  uint32_t lr_esp;
} La_i86_regs;

/* Return values for calls from PLT on IA-32.  */
typedef struct La_i86_retval
{
  uint32_t lrv_eax;
  uint32_t lrv_edx;
  long double lrv_st0;
  long double lrv_st1;
} La_i86_retval;


__BEGIN_DECLS

extern Elf32_Addr la_i86_gnu_pltenter (Elf32_Sym *__sym, unsigned int __ndx,
				       uintptr_t *__refcook,
				       uintptr_t *__defcook,
				       La_i86_regs *__regs,
				       unsigned int *__flags,
				       const char *__symname,
				       long int *__framesizep);
extern unsigned int la_i86_gnu_pltexit (Elf32_Sym *__sym, unsigned int __ndx,
					uintptr_t *__refcook,
					uintptr_t *__defcook,
					const La_i86_regs *__inregs,
					La_i86_retval *__outregs,
					const char *__symname);

__END_DECLS
