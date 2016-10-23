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

// Do not include this header file directly. Use base/cef_bind.h instead.

// Specializations of RunnableAdapter<> for Windows specific calling
// conventions.  Please see base/bind_internal.h for more info.

#ifndef CEF_INCLUDE_BASE_INTERNAL_CEF_BIND_INTERNAL_WIN_H_
#define CEF_INCLUDE_BASE_INTERNAL_CEF_BIND_INTERNAL_WIN_H_

// In the x64 architecture in Windows, __fastcall, __stdcall, etc, are all
// the same as __cdecl which would turn the following specializations into
// multiple definitions.
#if !defined(ARCH_CPU_X86_64)

namespace base {
namespace cef_internal {

template <typename Functor>
class RunnableAdapter;

// __stdcall Function: Arity 0.
template <typename R>
class RunnableAdapter<R(__stdcall *)()> {
 public:
  typedef R (RunType)();

  explicit RunnableAdapter(R(__stdcall *function)())
      : function_(function) {
  }

  R Run() {
    return function_();
  }

 private:
  R (__stdcall *function_)();
};

// __fastcall Function: Arity 0.
template <typename R>
class RunnableAdapter<R(__fastcall *)()> {
 public:
  typedef R (RunType)();

  explicit RunnableAdapter(R(__fastcall *function)())
      : function_(function) {
  }

  R Run() {
    return function_();
  }

 private:
  R (__fastcall *function_)();
};

// __stdcall Function: Arity 1.
template <typename R, typename A1>
class RunnableAdapter<R(__stdcall *)(A1)> {
 public:
  typedef R (RunType)(A1);

  explicit RunnableAdapter(R(__stdcall *function)(A1))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1) {
    return function_(a1);
  }

 private:
  R (__stdcall *function_)(A1);
};

// __fastcall Function: Arity 1.
template <typename R, typename A1>
class RunnableAdapter<R(__fastcall *)(A1)> {
 public:
  typedef R (RunType)(A1);

  explicit RunnableAdapter(R(__fastcall *function)(A1))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1) {
    return function_(a1);
  }

 private:
  R (__fastcall *function_)(A1);
};

// __stdcall Function: Arity 2.
template <typename R, typename A1, typename A2>
class RunnableAdapter<R(__stdcall *)(A1, A2)> {
 public:
  typedef R (RunType)(A1, A2);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return function_(a1, a2);
  }

 private:
  R (__stdcall *function_)(A1, A2);
};

// __fastcall Function: Arity 2.
template <typename R, typename A1, typename A2>
class RunnableAdapter<R(__fastcall *)(A1, A2)> {
 public:
  typedef R (RunType)(A1, A2);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return function_(a1, a2);
  }

 private:
  R (__fastcall *function_)(A1, A2);
};

// __stdcall Function: Arity 3.
template <typename R, typename A1, typename A2, typename A3>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3)> {
 public:
  typedef R (RunType)(A1, A2, A3);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return function_(a1, a2, a3);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3);
};

// __fastcall Function: Arity 3.
template <typename R, typename A1, typename A2, typename A3>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3)> {
 public:
  typedef R (RunType)(A1, A2, A3);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return function_(a1, a2, a3);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3);
};

// __stdcall Function: Arity 4.
template <typename R, typename A1, typename A2, typename A3, typename A4>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return function_(a1, a2, a3, a4);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4);
};

// __fastcall Function: Arity 4.
template <typename R, typename A1, typename A2, typename A3, typename A4>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return function_(a1, a2, a3, a4);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4);
};

// __stdcall Function: Arity 5.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return function_(a1, a2, a3, a4, a5);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5);
};

// __fastcall Function: Arity 5.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return function_(a1, a2, a3, a4, a5);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5);
};

// __stdcall Function: Arity 6.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5, A6))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return function_(a1, a2, a3, a4, a5, a6);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5, A6);
};

// __fastcall Function: Arity 6.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5, A6))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return function_(a1, a2, a3, a4, a5, a6);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5, A6);
};

// __stdcall Function: Arity 7.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6, A7);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5, A6, A7))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return function_(a1, a2, a3, a4, a5, a6, a7);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5, A6, A7);
};

// __fastcall Function: Arity 7.
template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6, A7);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5, A6, A7))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return function_(a1, a2, a3, a4, a5, a6, a7);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5, A6, A7);
};

}  // namespace cef_internal
}  // namespace base

#endif  // !defined(ARCH_CPU_X86_64)

#endif  // CEF_INCLUDE_BASE_INTERNAL_CEF_BIND_INTERNAL_WIN_H_
