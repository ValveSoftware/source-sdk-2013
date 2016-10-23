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

#ifndef CEF_INCLUDE_BASE_CEF_TEMPLATE_UTIL_H_
#define CEF_INCLUDE_BASE_CEF_TEMPLATE_UTIL_H_
#pragma once

#if defined(BASE_TEMPLATE_UTIL_H_)
// Do nothing if the Chromium header has already been included.
// This can happen in cases where Chromium code is used directly by the
// client application. When using Chromium code directly always include
// the Chromium header first to avoid type conflicts.
#elif defined(USING_CHROMIUM_INCLUDES)
// When building CEF include the Chromium header directly.
#include "base/template_util.h"
#else  // !USING_CHROMIUM_INCLUDES
// The following is substantially similar to the Chromium implementation.
// If the Chromium implementation diverges the below implementation should be
// updated to match.

#include <cstddef>  // For size_t.

#include "include/base/cef_build.h"

namespace base {

// template definitions from tr1

template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

template <class T, T v> const T integral_constant<T, v>::value;

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <class T> struct is_pointer : false_type {};
template <class T> struct is_pointer<T*> : true_type {};

// Member function pointer detection up to four params. Add more as needed
// below. This is built-in to C++ 11, and we can remove this when we switch.
template<typename T>
struct is_member_function_pointer : false_type {};

template <typename R, typename Z>
struct is_member_function_pointer<R(Z::*)()> : true_type {};
template <typename R, typename Z>
struct is_member_function_pointer<R(Z::*)() const> : true_type {};

template <typename R, typename Z, typename A>
struct is_member_function_pointer<R(Z::*)(A)> : true_type {};
template <typename R, typename Z, typename A>
struct is_member_function_pointer<R(Z::*)(A) const> : true_type {};

template <typename R, typename Z, typename A, typename B>
struct is_member_function_pointer<R(Z::*)(A, B)> : true_type {};
template <typename R, typename Z, typename A, typename B>
struct is_member_function_pointer<R(Z::*)(A, B) const> : true_type {};

template <typename R, typename Z, typename A, typename B, typename C>
struct is_member_function_pointer<R(Z::*)(A, B, C)> : true_type {};
template <typename R, typename Z, typename A, typename B, typename C>
struct is_member_function_pointer<R(Z::*)(A, B, C) const> : true_type {};

template <typename R, typename Z, typename A, typename B, typename C,
          typename D>
struct is_member_function_pointer<R(Z::*)(A, B, C, D)> : true_type {};
template <typename R, typename Z, typename A, typename B, typename C,
          typename D>
struct is_member_function_pointer<R(Z::*)(A, B, C, D) const> : true_type {};


template <class T, class U> struct is_same : public false_type {};
template <class T> struct is_same<T,T> : true_type {};

template<class> struct is_array : public false_type {};
template<class T, size_t n> struct is_array<T[n]> : public true_type {};
template<class T> struct is_array<T[]> : public true_type {};

template <class T> struct is_non_const_reference : false_type {};
template <class T> struct is_non_const_reference<T&> : true_type {};
template <class T> struct is_non_const_reference<const T&> : false_type {};

template <class T> struct is_const : false_type {};
template <class T> struct is_const<const T> : true_type {};

template <class T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};

namespace cef_internal {

// Types YesType and NoType are guaranteed such that sizeof(YesType) <
// sizeof(NoType).
typedef char YesType;

struct NoType {
  YesType dummy[2];
};

// This class is an implementation detail for is_convertible, and you
// don't need to know how it works to use is_convertible. For those
// who care: we declare two different functions, one whose argument is
// of type To and one with a variadic argument list. We give them
// return types of different size, so we can use sizeof to trick the
// compiler into telling us which function it would have chosen if we
// had called it with an argument of type From.  See Alexandrescu's
// _Modern C++ Design_ for more details on this sort of trick.

struct ConvertHelper {
  template <typename To>
  static YesType Test(To);

  template <typename To>
  static NoType Test(...);

  template <typename From>
  static From& Create();
};

// Used to determine if a type is a struct/union/class. Inspired by Boost's
// is_class type_trait implementation.
struct IsClassHelper {
  template <typename C>
  static YesType Test(void(C::*)(void));

  template <typename C>
  static NoType Test(...);
};

}  // namespace cef_internal

// Inherits from true_type if From is convertible to To, false_type otherwise.
//
// Note that if the type is convertible, this will be a true_type REGARDLESS
// of whether or not the conversion would emit a warning.
template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(cef_internal::ConvertHelper::Test<To>(
                                   cef_internal::ConvertHelper::Create<From>())) ==
                        sizeof(cef_internal::YesType)> {
};

template <typename T>
struct is_class
    : integral_constant<bool,
                        sizeof(cef_internal::IsClassHelper::Test<T>(0)) ==
                            sizeof(cef_internal::YesType)> {
};

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

}  // namespace base

#endif  // !USING_CHROMIUM_INCLUDES

#endif  // CEF_INCLUDE_BASE_CEF_TEMPLATE_UTIL_H_
