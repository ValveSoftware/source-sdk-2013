/* Copyright (C) 1998, 1999, 2000, 2002, 2003 Free Software Foundation, Inc.
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

#ifndef _STROPTS_H
#define _STROPTS_H	1

#include <features.h>
#include <bits/types.h>
#include <bits/xtitypes.h>

#ifndef __gid_t_defined
typedef __gid_t gid_t;
# define __gid_t_defined
#endif

#ifndef __uid_t_defined
typedef __uid_t uid_t;
# define __uid_t_defined
#endif

typedef __t_uscalar_t t_uscalar_t;

/* Get system specific contants.  */
#include <bits/stropts.h>


__BEGIN_DECLS

/* Test whether FILDES is associated with a STREAM-based file.  */
extern int isastream (int __fildes) __THROW;

/* Receive next message from a STREAMS file.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int getmsg (int __fildes, struct strbuf *__restrict __ctlptr,
		   struct strbuf *__restrict __dataptr,
		   int *__restrict __flagsp);

/* Receive next message from a STREAMS file, with *FLAGSP allowing to
   control which message.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int getpmsg (int __fildes, struct strbuf *__restrict __ctlptr,
		    struct strbuf *__restrict __dataptr,
		    int *__restrict __bandp, int *__restrict __flagsp);

/* Perform the I/O control operation specified by REQUEST on FD.
   One argument may follow; its presence and type depend on REQUEST.
   Return value depends on REQUEST.  Usually -1 indicates error.  */
extern int ioctl (int __fd, unsigned long int __request, ...) __THROW;

/* Send a message on a STREAM.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int putmsg (int __fildes, __const struct strbuf *__ctlptr,
		   __const struct strbuf *__dataptr, int __flags);

/* Send a message on a STREAM to the BAND.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int putpmsg (int __fildes, __const struct strbuf *__ctlptr,
		    __const struct strbuf *__dataptr, int __band, int __flags);

/* Attach a STREAMS-based file descriptor FILDES to a file PATH in the
   file system name space.  */
extern int fattach (int __fildes, __const char *__path) __THROW;

/* Detach a name PATH from a STREAMS-based file descriptor.  */
extern int fdetach (__const char *__path) __THROW;

__END_DECLS

#endif /* stropts.h */
