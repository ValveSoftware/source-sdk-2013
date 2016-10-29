// Copyright (c) 2014 Marshall A. Greenblatt. Portions copyright (c) 2011
// Google Inc. All rights reserved.
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

#ifndef CEF_INCLUDE_BASE_CEF_BIND_H_
#define CEF_INCLUDE_BASE_CEF_BIND_H_
#pragma once

#if defined(BASE_BIND_H_)
// Do nothing if the Chromium header has already been included.
// This can happen in cases where Chromium code is used directly by the
// client application. When using Chromium code directly always include
// the Chromium header first to avoid type conflicts.
#elif defined(USING_CHROMIUM_INCLUDES)
// When building CEF include the Chromium header directly.
#include "base/bind.h"
#else  // !USING_CHROMIUM_INCLUDES
// The following is substantially similar to the Chromium implementation.
// If the Chromium implementation diverges the below implementation should be
// updated to match.

#include "include/base/internal/cef_bind_internal.h"
#include "include/base/internal/cef_callback_internal.h"

// -----------------------------------------------------------------------------
// Usage documentation
// -----------------------------------------------------------------------------
//
// See base/cef_callback.h for documentation.
//
//
// -----------------------------------------------------------------------------
// Implementation notes
// -----------------------------------------------------------------------------
//
// If you're reading the implementation, before proceeding further, you should
// read the top comment of base/bind_internal.h for a definition of common
// terms and concepts.
//
// RETURN TYPES
//
// Though Bind()'s result is meant to be stored in a Callback<> type, it
// cannot actually return the exact type without requiring a large amount
// of extra template specializations. The problem is that in order to
// discern the correct specialization of Callback<>, Bind would need to
// unwrap the function signature to determine the signature's arity, and
// whether or not it is a method.
//
// Each unique combination of (arity, function_type, num_prebound) where
// function_type is one of {function, method, const_method} would require
// one specialization.  We eventually have to do a similar number of
// specializations anyways in the implementation (see the Invoker<>,
// classes).  However, it is avoidable in Bind if we return the result
// via an indirection like we do below.
//
// TODO(ajwong): We might be able to avoid this now, but need to test.
//
// It is possible to move most of the COMPILE_ASSERT asserts into BindState<>,
// but it feels a little nicer to have the asserts here so people do not
// need to crack open bind_internal.h.  On the other hand, it makes Bind()
// harder to read.

namespace base {

template <typename Functor>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void()>
            ::UnboundRunType>
Bind(Functor functor) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  typedef cef_internal::BindState<RunnableType, RunType, void()> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor)));
}

template <typename Functor, typename P1>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1));
}

template <typename Functor, typename P1, typename P2>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2));
}

template <typename Functor, typename P1, typename P2, typename P3>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType,
            typename cef_internal::CallbackParamTraits<P3>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType,
      typename cef_internal::CallbackParamTraits<P3>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2, p3));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType,
            typename cef_internal::CallbackParamTraits<P3>::StorageType,
            typename cef_internal::CallbackParamTraits<P4>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType,
      typename cef_internal::CallbackParamTraits<P3>::StorageType,
      typename cef_internal::CallbackParamTraits<P4>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2, p3, p4));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType,
            typename cef_internal::CallbackParamTraits<P3>::StorageType,
            typename cef_internal::CallbackParamTraits<P4>::StorageType,
            typename cef_internal::CallbackParamTraits<P5>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType,
      typename cef_internal::CallbackParamTraits<P3>::StorageType,
      typename cef_internal::CallbackParamTraits<P4>::StorageType,
      typename cef_internal::CallbackParamTraits<P5>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2, p3, p4, p5));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5, typename P6>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType,
            typename cef_internal::CallbackParamTraits<P3>::StorageType,
            typename cef_internal::CallbackParamTraits<P4>::StorageType,
            typename cef_internal::CallbackParamTraits<P5>::StorageType,
            typename cef_internal::CallbackParamTraits<P6>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A6Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P6>::value,
                 p6_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType,
      typename cef_internal::CallbackParamTraits<P3>::StorageType,
      typename cef_internal::CallbackParamTraits<P4>::StorageType,
      typename cef_internal::CallbackParamTraits<P5>::StorageType,
      typename cef_internal::CallbackParamTraits<P6>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2, p3, p4, p5, p6));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5, typename P6, typename P7>
base::Callback<
    typename cef_internal::BindState<
        typename cef_internal::FunctorTraits<Functor>::RunnableType,
        typename cef_internal::FunctorTraits<Functor>::RunType,
        void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
            typename cef_internal::CallbackParamTraits<P2>::StorageType,
            typename cef_internal::CallbackParamTraits<P3>::StorageType,
            typename cef_internal::CallbackParamTraits<P4>::StorageType,
            typename cef_internal::CallbackParamTraits<P5>::StorageType,
            typename cef_internal::CallbackParamTraits<P6>::StorageType,
            typename cef_internal::CallbackParamTraits<P7>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6, const P7& p7) {
  // Typedefs for how to store and run the functor.
  typedef typename cef_internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename cef_internal::FunctorTraits<Functor>::RunType RunType;

  // Use RunnableType::RunType instead of RunType above because our
  // checks should below for bound references need to know what the actual
  // functor is going to interpret the argument as.
  typedef cef_internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  // Do not allow binding a non-const reference parameter. Non-const reference
  // parameters are disallowed by the Google style guide.  Also, binding a
  // non-const reference parameter can make for subtle bugs because the
  // invoked function will receive a reference to the stored copy of the
  // argument and not the original.
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A6Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A7Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  // For methods, we need to be careful for parameter 1.  We do not require
  // a scoped_refptr because BindState<> itself takes care of AddRef() for
  // methods. We also disallow binding of an array as the method's target
  // object.
  COMPILE_ASSERT(
      cef_internal::HasIsMethodTag<RunnableType>::value ||
          !cef_internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P6>::value,
                 p6_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!cef_internal::NeedsScopedRefptrButGetsRawPtr<P7>::value,
                 p7_is_refcounted_type_and_needs_scoped_refptr);
  typedef cef_internal::BindState<RunnableType, RunType,
      void(typename cef_internal::CallbackParamTraits<P1>::StorageType,
      typename cef_internal::CallbackParamTraits<P2>::StorageType,
      typename cef_internal::CallbackParamTraits<P3>::StorageType,
      typename cef_internal::CallbackParamTraits<P4>::StorageType,
      typename cef_internal::CallbackParamTraits<P5>::StorageType,
      typename cef_internal::CallbackParamTraits<P6>::StorageType,
      typename cef_internal::CallbackParamTraits<P7>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(cef_internal::MakeRunnable(functor), p1, p2, p3, p4, p5, p6,
          p7));
}

}  // namespace base

#endif  // !USING_CHROMIUM_INCLUDES

#endif  // CEF_INCLUDE_BASE_CEF_BIND_H_
