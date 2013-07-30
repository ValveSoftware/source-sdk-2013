/* Copyright (C) 2007, 2009 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.

/* Implemented from the specification included in the Intel C++ Compiler
   User Guide and Reference, version 10.0.  */

#ifndef _NMMINTRIN_H_INCLUDED
#define _NMMINTRIN_H_INCLUDED

#ifndef __SSE4_2__
# error "SSE4.2 instruction set not enabled"
#else
/* We just include SSE4.1 header file.  */
#include <smmintrin.h>
#endif /* __SSE4_2__ */

#endif /* _NMMINTRIN_H_INCLUDED */
