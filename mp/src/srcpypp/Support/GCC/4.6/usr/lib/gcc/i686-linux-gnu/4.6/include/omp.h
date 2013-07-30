/* Copyright (C) 2005, 2007, 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef OMP_H
#define OMP_H 1

#ifndef _LIBGOMP_OMP_LOCK_DEFINED
#define _LIBGOMP_OMP_LOCK_DEFINED 1
/* These two structures get edited by the libgomp build process to 
   reflect the shape of the two types.  Their internals are private
   to the library.  */

typedef struct
{
  unsigned char _x[4] 
    __attribute__((__aligned__(4)));
} omp_lock_t;

typedef struct
{
  unsigned char _x[8 + sizeof (void *)] 
    __attribute__((__aligned__(sizeof (void *))));
} omp_nest_lock_t;
#endif

typedef enum omp_sched_t
{
  omp_sched_static = 1,
  omp_sched_dynamic = 2,
  omp_sched_guided = 3,
  omp_sched_auto = 4
} omp_sched_t;

#ifdef __cplusplus
extern "C" {
# define __GOMP_NOTHROW throw ()
#else
# define __GOMP_NOTHROW __attribute__((__nothrow__))
#endif

extern void omp_set_num_threads (int) __GOMP_NOTHROW;
extern int omp_get_num_threads (void) __GOMP_NOTHROW;
extern int omp_get_max_threads (void) __GOMP_NOTHROW;
extern int omp_get_thread_num (void) __GOMP_NOTHROW;
extern int omp_get_num_procs (void) __GOMP_NOTHROW;

extern int omp_in_parallel (void) __GOMP_NOTHROW;

extern void omp_set_dynamic (int) __GOMP_NOTHROW;
extern int omp_get_dynamic (void) __GOMP_NOTHROW;

extern void omp_set_nested (int) __GOMP_NOTHROW;
extern int omp_get_nested (void) __GOMP_NOTHROW;

extern void omp_init_lock (omp_lock_t *) __GOMP_NOTHROW;
extern void omp_destroy_lock (omp_lock_t *) __GOMP_NOTHROW;
extern void omp_set_lock (omp_lock_t *) __GOMP_NOTHROW;
extern void omp_unset_lock (omp_lock_t *) __GOMP_NOTHROW;
extern int omp_test_lock (omp_lock_t *) __GOMP_NOTHROW;

extern void omp_init_nest_lock (omp_nest_lock_t *) __GOMP_NOTHROW;
extern void omp_destroy_nest_lock (omp_nest_lock_t *) __GOMP_NOTHROW;
extern void omp_set_nest_lock (omp_nest_lock_t *) __GOMP_NOTHROW;
extern void omp_unset_nest_lock (omp_nest_lock_t *) __GOMP_NOTHROW;
extern int omp_test_nest_lock (omp_nest_lock_t *) __GOMP_NOTHROW;

extern double omp_get_wtime (void) __GOMP_NOTHROW;
extern double omp_get_wtick (void) __GOMP_NOTHROW;

void omp_set_schedule (omp_sched_t, int) __GOMP_NOTHROW;
void omp_get_schedule (omp_sched_t *, int *) __GOMP_NOTHROW;
int omp_get_thread_limit (void) __GOMP_NOTHROW;
void omp_set_max_active_levels (int) __GOMP_NOTHROW;
int omp_get_max_active_levels (void) __GOMP_NOTHROW;
int omp_get_level (void) __GOMP_NOTHROW;
int omp_get_ancestor_thread_num (int) __GOMP_NOTHROW;
int omp_get_team_size (int) __GOMP_NOTHROW;
int omp_get_active_level (void) __GOMP_NOTHROW;

#ifdef __cplusplus
}
#endif

#endif /* OMP_H */
