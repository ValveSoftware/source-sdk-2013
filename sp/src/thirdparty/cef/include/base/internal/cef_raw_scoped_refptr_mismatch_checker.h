// Copyright (c) 2011 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Do not include this header file directly. Use base/cef_callback.h instead.

#ifndef CEF_INCLUDE_BASE_INTERNAL_CEF_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
#define CEF_INCLUDE_BASE_INTERNAL_CEF_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_

#include "include/base/cef_build.h"
#include "include/base/cef_ref_counted.h"
#include "include/base/cef_template_util.h"
#include "include/base/cef_tuple.h"

// It is dangerous to post a task with a T* argument where T is a subtype of
// RefCounted(Base|ThreadSafeBase), since by the time the parameter is used, the
// object may already have been deleted since it was not held with a
// scoped_refptr. Example: http://crbug.com/27191
// The following set of traits are designed to generate a compile error
// whenever this antipattern is attempted.

namespace base {

namespace cef_internal {

template <typename T>
struct NeedsScopedRefptrButGetsRawPtr {
#if defined(OS_WIN)
  enum {
    value = base::false_type::value
  };
#else
  enum {
    // Human readable translation: you needed to be a scoped_refptr if you are a
    // raw pointer type and are convertible to a RefCounted(Base|ThreadSafeBase)
    // type.
    value = (is_pointer<T>::value &&
             (is_convertible<T, subtle::RefCountedBase*>::value ||
              is_convertible<T, subtle::RefCountedThreadSafeBase*>::value))
  };
#endif
};

template <typename Params>
struct ParamsUseScopedRefptrCorrectly {
  enum { value = 0 };
};

template <>
struct ParamsUseScopedRefptrCorrectly<Tuple0> {
  enum { value = 1 };
};

template <typename A>
struct ParamsUseScopedRefptrCorrectly<Tuple1<A> > {
  enum { value = !NeedsScopedRefptrButGetsRawPtr<A>::value };
};

template <typename A, typename B>
struct ParamsUseScopedRefptrCorrectly<Tuple2<A, B> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value) };
};

template <typename A, typename B, typename C>
struct ParamsUseScopedRefptrCorrectly<Tuple3<A, B, C> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value) };
};

template <typename A, typename B, typename C, typename D>
struct ParamsUseScopedRefptrCorrectly<Tuple4<A, B, C, D> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value) };
};

template <typename A, typename B, typename C, typename D, typename E>
struct ParamsUseScopedRefptrCorrectly<Tuple5<A, B, C, D, E> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F>
struct ParamsUseScopedRefptrCorrectly<Tuple6<A, B, C, D, E, F> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F, typename G>
struct ParamsUseScopedRefptrCorrectly<Tuple7<A, B, C, D, E, F, G> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value ||
                   NeedsScopedRefptrButGetsRawPtr<G>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F, typename G, typename H>
struct ParamsUseScopedRefptrCorrectly<Tuple8<A, B, C, D, E, F, G, H> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value ||
                   NeedsScopedRefptrButGetsRawPtr<G>::value ||
                   NeedsScopedRefptrButGetsRawPtr<H>::value) };
};

}  // namespace cef_internal

}  // namespace base

#endif  // CEF_INCLUDE_BASE_INTERNAL_CEF_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
