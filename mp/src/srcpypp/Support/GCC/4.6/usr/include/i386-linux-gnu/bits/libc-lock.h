/* libc-internal interface for mutex locks.  NPTL version.
   Copyright (C) 1996-2003, 2005, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <pthread.h>
#define __need_NULL
#include <stddef.h>


/* Fortunately Linux now has a mean to do locking which is realtime
   safe without the aid of the thread library.  We also need no fancy
   options like error checking mutexes etc.  We only need simple
   locks, maybe recursive.  This can be easily and cheaply implemented
   using futexes.  We will use them everywhere except in ld.so since
   ld.so might be used on old kernels with a different libc.so.  */
#ifdef _LIBC
# include <lowlevellock.h>
# include <tls.h>
# include <pthread-functions.h>
# include <errno.h> /* For EBUSY.  */
# include <gnu/option-groups.h> /* For __OPTION_EGLIBC_BIG_MACROS.  */
#endif

/* Mutex type.  */
#if defined _LIBC || defined _IO_MTSAFE_IO
# if (defined NOT_IN_libc && !defined IS_IN_libpthread) || !defined _LIBC
typedef pthread_mutex_t __libc_lock_t;
typedef struct { pthread_mutex_t mutex; } __libc_lock_recursive_t;
# else
typedef int __libc_lock_t;
typedef struct { int lock; int cnt; void *owner; } __libc_lock_recursive_t;
# endif
typedef struct { pthread_mutex_t mutex; } __rtld_lock_recursive_t;
# ifdef __USE_UNIX98
typedef pthread_rwlock_t __libc_rwlock_t;
# else
typedef struct __libc_rwlock_opaque__ __libc_rwlock_t;
# endif
#else
typedef struct __libc_lock_opaque__ __libc_lock_t;
typedef struct __libc_lock_recursive_opaque__ __libc_lock_recursive_t;
typedef struct __libc_rwlock_opaque__ __libc_rwlock_t;
#endif

/* Type for key to thread-specific data.  */
typedef pthread_key_t __libc_key_t;

/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */
#define __libc_lock_define(CLASS,NAME) \
  CLASS __libc_lock_t NAME;
#define __libc_rwlock_define(CLASS,NAME) \
  CLASS __libc_rwlock_t NAME;
#define __libc_lock_define_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME;
#define __rtld_lock_define_recursive(CLASS,NAME) \
  CLASS __rtld_lock_recursive_t NAME;

/* Define an initialized lock variable NAME with storage class CLASS.

   For the C library we take a deeper look at the initializer.  For
   this implementation all fields are initialized to zero.  Therefore
   we don't initialize the variable which allows putting it into the
   BSS section.  (Except on PA-RISC and other odd architectures, where
   initialized locks must be set to one due to the lack of normal
   atomic operations.) */

#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if LLL_LOCK_INITIALIZER == 0
#  define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME;
# else
#  define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME = LLL_LOCK_INITIALIZER;
# endif
#else
# if __LT_SPINLOCK_INIT == 0
#  define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME;
# else
#  define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME = PTHREAD_MUTEX_INITIALIZER;
# endif
#endif

#define __libc_rwlock_define_initialized(CLASS,NAME) \
  CLASS __libc_rwlock_t NAME = PTHREAD_RWLOCK_INITIALIZER;

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if LLL_LOCK_INITIALIZER == 0
#  define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME;
# else
#  define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME = _LIBC_LOCK_RECURSIVE_INITIALIZER;
# endif
# define _LIBC_LOCK_RECURSIVE_INITIALIZER \
  { LLL_LOCK_INITIALIZER, 0, NULL }
#else
# define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME = _LIBC_LOCK_RECURSIVE_INITIALIZER;
# define _LIBC_LOCK_RECURSIVE_INITIALIZER \
  {PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP}
#endif

#define __rtld_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __rtld_lock_recursive_t NAME = _RTLD_LOCK_RECURSIVE_INITIALIZER;
#define _RTLD_LOCK_RECURSIVE_INITIALIZER \
  {PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP}

#define __rtld_lock_initialize(NAME) \
  (void) ((NAME) = (__rtld_lock_recursive_t) _RTLD_LOCK_RECURSIVE_INITIALIZER)

/* If we check for a weakly referenced symbol and then perform a
   normal jump to it te code generated for some platforms in case of
   PIC is unnecessarily slow.  What would happen is that the function
   is first referenced as data and then it is called indirectly
   through the PLT.  We can make this a direct jump.  */
#ifdef __PIC__
# define __libc_maybe_call(FUNC, ARGS, ELSE) \
  (__extension__ ({ __typeof (FUNC) *_fn = (FUNC); \
                    _fn != NULL ? (*_fn) ARGS : ELSE; }))
#else
# define __libc_maybe_call(FUNC, ARGS, ELSE) \
  (FUNC != NULL ? FUNC ARGS : ELSE)
#endif

/* Call thread functions through the function pointer table.  */
#if defined SHARED && !defined NOT_IN_libc
# define PTFAVAIL(NAME) __libc_pthread_functions_init
# define __libc_ptf_call(FUNC, ARGS, ELSE) \
  (__libc_pthread_functions_init ? PTHFCT_CALL (ptr_##FUNC, ARGS) : ELSE)
# define __libc_ptf_call_always(FUNC, ARGS) \
  PTHFCT_CALL (ptr_##FUNC, ARGS)
#else
# define PTFAVAIL(NAME) (NAME != NULL)
# define __libc_ptf_call(FUNC, ARGS, ELSE) \
  __libc_maybe_call (FUNC, ARGS, ELSE)
# define __libc_ptf_call_always(FUNC, ARGS) \
  FUNC ARGS
#endif


/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# define __libc_lock_init(NAME) ((NAME) = LLL_LOCK_INITIALIZER, 0)
#else
# define __libc_lock_init(NAME) \
  __libc_maybe_call (__pthread_mutex_init, (&(NAME), NULL), 0)
#endif
#if defined SHARED && !defined NOT_IN_libc
/* ((NAME) = (__libc_rwlock_t) PTHREAD_RWLOCK_INITIALIZER, 0) is
   inefficient.  */
# define __libc_rwlock_init(NAME) \
  (__builtin_memset (&(NAME), '\0', sizeof (NAME)), 0)
#else
# define __libc_rwlock_init(NAME) \
  __libc_maybe_call (__pthread_rwlock_init, (&(NAME), NULL), 0)
#endif

/* Same as last but this time we initialize a recursive mutex.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# define __libc_lock_init_recursive(NAME) \
  ((NAME) = (__libc_lock_recursive_t) _LIBC_LOCK_RECURSIVE_INITIALIZER, 0)
#else
# define __libc_lock_init_recursive(NAME) \
  do {									      \
    if (__pthread_mutex_init != NULL)					      \
      {									      \
	pthread_mutexattr_t __attr;					      \
	__pthread_mutexattr_init (&__attr);				      \
	__pthread_mutexattr_settype (&__attr, PTHREAD_MUTEX_RECURSIVE_NP);    \
	__pthread_mutex_init (&(NAME).mutex, &__attr);			      \
	__pthread_mutexattr_destroy (&__attr);				      \
      }									      \
  } while (0)
#endif

#define __rtld_lock_init_recursive(NAME) \
  do {									      \
    if (__pthread_mutex_init != NULL)					      \
      {									      \
	pthread_mutexattr_t __attr;					      \
	__pthread_mutexattr_init (&__attr);				      \
	__pthread_mutexattr_settype (&__attr, PTHREAD_MUTEX_RECURSIVE_NP);    \
	__pthread_mutex_init (&(NAME).mutex, &__attr);			      \
	__pthread_mutexattr_destroy (&__attr);				      \
      }									      \
  } while (0)

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# define __libc_lock_fini(NAME) ((void) 0)
#else
# define __libc_lock_fini(NAME) \
  __libc_maybe_call (__pthread_mutex_destroy, (&(NAME)), 0)
#endif
#if defined SHARED && !defined NOT_IN_libc
# define __libc_rwlock_fini(NAME) ((void) 0)
#else
# define __libc_rwlock_fini(NAME) \
  __libc_maybe_call (__pthread_rwlock_destroy, (&(NAME)), 0)
#endif

/* Finalize recursive named lock.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# define __libc_lock_fini_recursive(NAME) ((void) 0)
#else
# define __libc_lock_fini_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_destroy, (&(NAME)), 0)
#endif

/* Lock the named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern void __libc_lock_lock_fn (__libc_lock_t *);
libc_hidden_proto (__libc_lock_lock_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
# define __libc_lock_lock(NAME) \
  ({ lll_lock (NAME, LLL_PRIVATE); 0; })
# else
#  define __libc_lock_lock(NAME)		\
  __libc_lock_lock_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_lock(NAME) \
  __libc_maybe_call (__pthread_mutex_lock, (&(NAME)), 0)
#endif
#define __libc_rwlock_rdlock(NAME) \
  __libc_ptf_call (__pthread_rwlock_rdlock, (&(NAME)), 0)
#define __libc_rwlock_wrlock(NAME) \
  __libc_ptf_call (__pthread_rwlock_wrlock, (&(NAME)), 0)

/* Lock the recursive named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern void __libc_lock_lock_recursive_fn (__libc_lock_recursive_t *);
libc_hidden_proto (__libc_lock_lock_recursive_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
# define __libc_lock_lock_recursive(NAME) \
  do {									      \
    void *self = THREAD_SELF;						      \
    if ((NAME).owner != self)						      \
      {									      \
	lll_lock ((NAME).lock, LLL_PRIVATE);				      \
	(NAME).owner = self;						      \
      }									      \
    ++(NAME).cnt;							      \
  } while (0)
# else
# define __libc_lock_lock_recursive(NAME)				\
  __libc_lock_lock_recursive_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_lock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_lock, (&(NAME).mutex), 0)
#endif

/* Try to lock the named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern int __libc_lock_trylock_fn (__libc_lock_t *);
libc_hidden_proto (__libc_lock_trylock_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
# define __libc_lock_trylock(NAME) \
  lll_trylock (NAME)
# else
# define __libc_lock_trylock(NAME) \
  __libc_lock_trylock_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_trylock(NAME) \
  __libc_maybe_call (__pthread_mutex_trylock, (&(NAME)), 0)
#endif
#define __libc_rwlock_tryrdlock(NAME) \
  __libc_maybe_call (__pthread_rwlock_tryrdlock, (&(NAME)), 0)
#define __libc_rwlock_trywrlock(NAME) \
  __libc_maybe_call (__pthread_rwlock_trywrlock, (&(NAME)), 0)

/* Try to lock the recursive named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern int __libc_lock_trylock_recursive_fn (__libc_lock_recursive_t *);
libc_hidden_proto (__libc_lock_trylock_recursive_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
# define __libc_lock_trylock_recursive(NAME) \
  ({									      \
    int result = 0;							      \
    void *self = THREAD_SELF;						      \
    if ((NAME).owner != self)						      \
      {									      \
	if (lll_trylock ((NAME).lock) == 0)				      \
	  {								      \
	    (NAME).owner = self;					      \
	    (NAME).cnt = 1;						      \
	  }								      \
	else								      \
	  result = EBUSY;						      \
      }									      \
    else								      \
      ++(NAME).cnt;							      \
    result;								      \
  })
# else
# define __libc_lock_trylock_recursive(NAME) \
  __libc_lock_trylock_recursive_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_trylock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_trylock, (&(NAME)), 0)
#endif

#define __rtld_lock_trylock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_trylock, (&(NAME).mutex), 0)

/* Unlock the named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS, or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern void __libc_lock_unlock_fn (__libc_lock_t *);
libc_hidden_proto (__libc_lock_unlock_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
# define __libc_lock_unlock(NAME) \
  lll_unlock (NAME, LLL_PRIVATE)
# else
# define __libc_lock_unlock(NAME) \
  __libc_lock_unlock_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_unlock(NAME) \
  __libc_maybe_call (__pthread_mutex_unlock, (&(NAME)), 0)
#endif
#define __libc_rwlock_unlock(NAME) \
  __libc_ptf_call (__pthread_rwlock_unlock, (&(NAME)), 0)

/* Unlock the recursive named lock variable.  */
#if defined _LIBC && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# if __OPTION_EGLIBC_BIG_MACROS != 1
/* EGLIBC: Declare wrapper function for a big macro if either
   !__OPTION_EGLIBC_BIG_MACROS, or we are using a back door from
   small-macros-fns.c (__OPTION_EGLIBC_BIG_MACROS == 2).  */
extern void __libc_lock_unlock_recursive_fn (__libc_lock_recursive_t *);
libc_hidden_proto (__libc_lock_unlock_recursive_fn);
# endif /* __OPTION_EGLIBC_BIG_MACROS != 1 */
# if __OPTION_EGLIBC_BIG_MACROS
/* We do no error checking here.  */
# define __libc_lock_unlock_recursive(NAME) \
  do {									      \
    if (--(NAME).cnt == 0)						      \
      {									      \
	(NAME).owner = NULL;						      \
	lll_unlock ((NAME).lock, LLL_PRIVATE);				      \
      }									      \
  } while (0)
# else
# define __libc_lock_unlock_recursive(NAME) \
  __libc_lock_unlock_recursive_fn (&(NAME))
# endif /* __OPTION_EGLIBC_BIG_MACROS */
#else
# define __libc_lock_unlock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_unlock, (&(NAME)), 0)
#endif

#if defined _LIBC && defined SHARED
# define __rtld_lock_default_lock_recursive(lock) \
  ++((pthread_mutex_t *)(lock))->__data.__count;

# define __rtld_lock_default_unlock_recursive(lock) \
  --((pthread_mutex_t *)(lock))->__data.__count;

# define __rtld_lock_lock_recursive(NAME) \
  GL(dl_rtld_lock_recursive) (&(NAME).mutex)

# define __rtld_lock_unlock_recursive(NAME) \
  GL(dl_rtld_unlock_recursive) (&(NAME).mutex)
#else
# define __rtld_lock_lock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_lock, (&(NAME).mutex), 0)

# define __rtld_lock_unlock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_unlock, (&(NAME).mutex), 0)
#endif

/* Define once control variable.  */
#if PTHREAD_ONCE_INIT == 0
/* Special case for static variables where we can avoid the initialization
   if it is zero.  */
# define __libc_once_define(CLASS, NAME) \
  CLASS pthread_once_t NAME
#else
# define __libc_once_define(CLASS, NAME) \
  CLASS pthread_once_t NAME = PTHREAD_ONCE_INIT
#endif

/* Call handler iff the first call.  */
#define __libc_once(ONCE_CONTROL, INIT_FUNCTION) \
  do {									      \
    if (PTFAVAIL (__pthread_once))					      \
      __libc_ptf_call_always (__pthread_once, (&(ONCE_CONTROL),		      \
					       INIT_FUNCTION));		      \
    else if ((ONCE_CONTROL) == PTHREAD_ONCE_INIT) {			      \
      INIT_FUNCTION ();							      \
      (ONCE_CONTROL) |= 2;						      \
    }									      \
  } while (0)


/* Note that for I/O cleanup handling we are using the old-style
   cancel handling.  It does not have to be integrated with C++ snce
   no C++ code is called in the middle.  The old-style handling is
   faster and the support is not going away.  */
extern void _pthread_cleanup_push (struct _pthread_cleanup_buffer *buffer,
                                   void (*routine) (void *), void *arg);
extern void _pthread_cleanup_pop (struct _pthread_cleanup_buffer *buffer,
                                  int execute);
extern void _pthread_cleanup_push_defer (struct _pthread_cleanup_buffer *buffer,
                                         void (*routine) (void *), void *arg);
extern void _pthread_cleanup_pop_restore (struct _pthread_cleanup_buffer *buffer,
                                          int execute);

/* Start critical region with cleanup.  */
#define __libc_cleanup_region_start(DOIT, FCT, ARG) \
  { struct _pthread_cleanup_buffer _buffer;				      \
    int _avail;								      \
    if (DOIT) {								      \
      _avail = PTFAVAIL (_pthread_cleanup_push_defer);			      \
      if (_avail) {							      \
	__libc_ptf_call_always (_pthread_cleanup_push_defer, (&_buffer, FCT,  \
							      ARG));	      \
      } else {								      \
	_buffer.__routine = (FCT);					      \
	_buffer.__arg = (ARG);						      \
      }									      \
    } else {								      \
      _avail = 0;							      \
    }

/* End critical region with cleanup.  */
#define __libc_cleanup_region_end(DOIT) \
    if (_avail) {							      \
      __libc_ptf_call_always (_pthread_cleanup_pop_restore, (&_buffer, DOIT));\
    } else if (DOIT)							      \
      _buffer.__routine (_buffer.__arg);				      \
  }

/* Sometimes we have to exit the block in the middle.  */
#define __libc_cleanup_end(DOIT) \
    if (_avail) {							      \
      __libc_ptf_call_always (_pthread_cleanup_pop_restore, (&_buffer, DOIT));\
    } else if (DOIT)							      \
      _buffer.__routine (_buffer.__arg)


/* Normal cleanup handling, based on C cleanup attribute.  */
__extern_inline void
__libc_cleanup_routine (struct __pthread_cleanup_frame *f)
{
  if (f->__do_it)
    f->__cancel_routine (f->__cancel_arg);
}

#define __libc_cleanup_push(fct, arg) \
  do {									      \
    struct __pthread_cleanup_frame __clframe				      \
      __attribute__ ((__cleanup__ (__libc_cleanup_routine)))		      \
      = { .__cancel_routine = (fct), .__cancel_arg = (arg),		      \
          .__do_it = 1 };

#define __libc_cleanup_pop(execute) \
    __clframe.__do_it = (execute);					      \
  } while (0)


/* Create thread-specific key.  */
#define __libc_key_create(KEY, DESTRUCTOR) \
  __libc_ptf_call (__pthread_key_create, (KEY, DESTRUCTOR), 1)

/* Get thread-specific data.  */
#define __libc_getspecific(KEY) \
  __libc_ptf_call (__pthread_getspecific, (KEY), NULL)

/* Set thread-specific data.  */
#define __libc_setspecific(KEY, VALUE) \
  __libc_ptf_call (__pthread_setspecific, (KEY, VALUE), 0)


/* Register handlers to execute before and after `fork'.  Note that the
   last parameter is NULL.  The handlers registered by the libc are
   never removed so this is OK.  */
#define __libc_atfork(PREPARE, PARENT, CHILD) \
  __register_atfork (PREPARE, PARENT, CHILD, NULL)
extern int __register_atfork (void (*__prepare) (void),
			      void (*__parent) (void),
			      void (*__child) (void),
			      void *__dso_handle);

/* Functions that are used by this file and are internal to the GNU C
   library.  */

extern int __pthread_mutex_init (pthread_mutex_t *__mutex,
				 __const pthread_mutexattr_t *__mutex_attr);

extern int __pthread_mutex_destroy (pthread_mutex_t *__mutex);

extern int __pthread_mutex_trylock (pthread_mutex_t *__mutex);

extern int __pthread_mutex_lock (pthread_mutex_t *__mutex);

extern int __pthread_mutex_unlock (pthread_mutex_t *__mutex);

extern int __pthread_mutexattr_init (pthread_mutexattr_t *__attr);

extern int __pthread_mutexattr_destroy (pthread_mutexattr_t *__attr);

extern int __pthread_mutexattr_settype (pthread_mutexattr_t *__attr,
					int __kind);

#ifdef __USE_UNIX98
extern int __pthread_rwlock_init (pthread_rwlock_t *__rwlock,
				  __const pthread_rwlockattr_t *__attr);

extern int __pthread_rwlock_destroy (pthread_rwlock_t *__rwlock);

extern int __pthread_rwlock_rdlock (pthread_rwlock_t *__rwlock);

extern int __pthread_rwlock_tryrdlock (pthread_rwlock_t *__rwlock);

extern int __pthread_rwlock_wrlock (pthread_rwlock_t *__rwlock);

extern int __pthread_rwlock_trywrlock (pthread_rwlock_t *__rwlock);

extern int __pthread_rwlock_unlock (pthread_rwlock_t *__rwlock);
#endif

extern int __pthread_key_create (pthread_key_t *__key,
				 void (*__destr_function) (void *));

extern int __pthread_setspecific (pthread_key_t __key,
				  __const void *__pointer);

extern void *__pthread_getspecific (pthread_key_t __key);

extern int __pthread_once (pthread_once_t *__once_control,
			   void (*__init_routine) (void));

extern int __pthread_atfork (void (*__prepare) (void),
			     void (*__parent) (void),
			     void (*__child) (void));



/* Make the pthread functions weak so that we can elide them from
   single-threaded processes.  */
#ifndef __NO_WEAK_PTHREAD_ALIASES
# ifdef weak_extern
#  if _LIBC
#   include <bp-sym.h>
#  else
#   define BP_SYM(sym) sym
#  endif
weak_extern (BP_SYM (__pthread_mutex_init))
weak_extern (BP_SYM (__pthread_mutex_destroy))
weak_extern (BP_SYM (__pthread_mutex_lock))
weak_extern (BP_SYM (__pthread_mutex_trylock))
weak_extern (BP_SYM (__pthread_mutex_unlock))
weak_extern (BP_SYM (__pthread_mutexattr_init))
weak_extern (BP_SYM (__pthread_mutexattr_destroy))
weak_extern (BP_SYM (__pthread_mutexattr_settype))
weak_extern (BP_SYM (__pthread_rwlock_init))
weak_extern (BP_SYM (__pthread_rwlock_destroy))
weak_extern (BP_SYM (__pthread_rwlock_rdlock))
weak_extern (BP_SYM (__pthread_rwlock_tryrdlock))
weak_extern (BP_SYM (__pthread_rwlock_wrlock))
weak_extern (BP_SYM (__pthread_rwlock_trywrlock))
weak_extern (BP_SYM (__pthread_rwlock_unlock))
weak_extern (BP_SYM (__pthread_key_create))
weak_extern (BP_SYM (__pthread_setspecific))
weak_extern (BP_SYM (__pthread_getspecific))
weak_extern (BP_SYM (__pthread_once))
weak_extern (__pthread_initialize)
weak_extern (__pthread_atfork)
weak_extern (BP_SYM (_pthread_cleanup_push_defer))
weak_extern (BP_SYM (_pthread_cleanup_pop_restore))
weak_extern (BP_SYM (pthread_setcancelstate))
# else
#  pragma weak __pthread_mutex_init
#  pragma weak __pthread_mutex_destroy
#  pragma weak __pthread_mutex_lock
#  pragma weak __pthread_mutex_trylock
#  pragma weak __pthread_mutex_unlock
#  pragma weak __pthread_mutexattr_init
#  pragma weak __pthread_mutexattr_destroy
#  pragma weak __pthread_mutexattr_settype
#  pragma weak __pthread_rwlock_destroy
#  pragma weak __pthread_rwlock_rdlock
#  pragma weak __pthread_rwlock_tryrdlock
#  pragma weak __pthread_rwlock_wrlock
#  pragma weak __pthread_rwlock_trywrlock
#  pragma weak __pthread_rwlock_unlock
#  pragma weak __pthread_key_create
#  pragma weak __pthread_setspecific
#  pragma weak __pthread_getspecific
#  pragma weak __pthread_once
#  pragma weak __pthread_initialize
#  pragma weak __pthread_atfork
#  pragma weak _pthread_cleanup_push_defer
#  pragma weak _pthread_cleanup_pop_restore
#  pragma weak pthread_setcancelstate
# endif
#endif

#endif	/* bits/libc-lock.h */
