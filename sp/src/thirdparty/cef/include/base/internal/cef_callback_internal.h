// Copyright (c) 2012 Google Inc. All rights reserved.
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

// Do not include this header file directly. Use base/cef_bind.h or
// base/cef_callback.h instead.

// This file contains utility functions and classes that help the
// implementation, and management of the Callback objects.

#ifndef CEF_INCLUDE_BASE_INTERNAL_CEF_CALLBACK_INTERNAL_H_
#define CEF_INCLUDE_BASE_INTERNAL_CEF_CALLBACK_INTERNAL_H_

#include <stddef.h>

#include "include/base/cef_atomic_ref_count.h"
#include "include/base/cef_macros.h"
#include "include/base/cef_ref_counted.h"
#include "include/base/cef_scoped_ptr.h"
#include "include/base/cef_template_util.h"

template <typename T>
class ScopedVector;

namespace base {
namespace cef_internal {
class CallbackBase;

// At the base level, the only task is to add reference counting data. Don't use
// RefCountedThreadSafe since it requires the destructor to be a virtual method.
// Creating a vtable for every BindState template instantiation results in a lot
// of bloat. Its only task is to call the destructor which can be done with a
// function pointer.
class BindStateBase {
 protected:
  explicit BindStateBase(void (*destructor)(BindStateBase*))
      : ref_count_(0), destructor_(destructor) {}
  ~BindStateBase() {}

 private:
  friend class scoped_refptr<BindStateBase>;
  friend class CallbackBase;

  void AddRef();
  void Release();

  AtomicRefCount ref_count_;

  // Pointer to a function that will properly destroy |this|.
  void (*destructor_)(BindStateBase*);

  DISALLOW_COPY_AND_ASSIGN(BindStateBase);
};

// Holds the Callback methods that don't require specialization to reduce
// template bloat.
class CallbackBase {
 public:
  // Returns true if Callback is null (doesn't refer to anything).
  bool is_null() const { return bind_state_.get() == NULL; }

  // Returns the Callback into an uninitialized state.
  void Reset();

 protected:
  // In C++, it is safe to cast function pointers to function pointers of
  // another type. It is not okay to use void*. We create a InvokeFuncStorage
  // that that can store our function pointer, and then cast it back to
  // the original type on usage.
  typedef void(*InvokeFuncStorage)(void);

  // Returns true if this callback equals |other|. |other| may be null.
  bool Equals(const CallbackBase& other) const;

  // Allow initializing of |bind_state_| via the constructor to avoid default
  // initialization of the scoped_refptr.  We do not also initialize
  // |polymorphic_invoke_| here because doing a normal assignment in the
  // derived Callback templates makes for much nicer compiler errors.
  explicit CallbackBase(BindStateBase* bind_state);

  // Force the destructor to be instantiated inside this translation unit so
  // that our subclasses will not get inlined versions.  Avoids more template
  // bloat.
  ~CallbackBase();

  scoped_refptr<BindStateBase> bind_state_;
  InvokeFuncStorage polymorphic_invoke_;
};

// A helper template to determine if given type is non-const move-only-type,
// i.e. if a value of the given type should be passed via .Pass() in a
// destructive way.
template <typename T> struct IsMoveOnlyType {
  template <typename U>
  static YesType Test(const typename U::MoveOnlyTypeForCPP03*);

  template <typename U>
  static NoType Test(...);

  static const bool value = sizeof(Test<T>(0)) == sizeof(YesType) &&
                            !is_const<T>::value;
};

// This is a typetraits object that's used to take an argument type, and
// extract a suitable type for storing and forwarding arguments.
//
// In particular, it strips off references, and converts arrays to
// pointers for storage; and it avoids accidentally trying to create a
// "reference of a reference" if the argument is a reference type.
//
// This array type becomes an issue for storage because we are passing bound
// parameters by const reference. In this case, we end up passing an actual
// array type in the initializer list which C++ does not allow.  This will
// break passing of C-string literals.
template <typename T, bool is_move_only = IsMoveOnlyType<T>::value>
struct CallbackParamTraits {
  typedef const T& ForwardType;
  typedef T StorageType;
};

// The Storage should almost be impossible to trigger unless someone manually
// specifies type of the bind parameters.  However, in case they do,
// this will guard against us accidentally storing a reference parameter.
//
// The ForwardType should only be used for unbound arguments.
template <typename T>
struct CallbackParamTraits<T&, false> {
  typedef T& ForwardType;
  typedef T StorageType;
};

// Note that for array types, we implicitly add a const in the conversion. This
// means that it is not possible to bind array arguments to functions that take
// a non-const pointer. Trying to specialize the template based on a "const
// T[n]" does not seem to match correctly, so we are stuck with this
// restriction.
template <typename T, size_t n>
struct CallbackParamTraits<T[n], false> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};

// See comment for CallbackParamTraits<T[n]>.
template <typename T>
struct CallbackParamTraits<T[], false> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};

// Parameter traits for movable-but-not-copyable scopers.
//
// Callback<>/Bind() understands movable-but-not-copyable semantics where
// the type cannot be copied but can still have its state destructively
// transferred (aka. moved) to another instance of the same type by calling a
// helper function.  When used with Bind(), this signifies transferal of the
// object's state to the target function.
//
// For these types, the ForwardType must not be a const reference, or a
// reference.  A const reference is inappropriate, and would break const
// correctness, because we are implementing a destructive move.  A non-const
// reference cannot be used with temporaries which means the result of a
// function or a cast would not be usable with Callback<> or Bind().
template <typename T>
struct CallbackParamTraits<T, true> {
  typedef T ForwardType;
  typedef T StorageType;
};

// CallbackForward() is a very limited simulation of C++11's std::forward()
// used by the Callback/Bind system for a set of movable-but-not-copyable
// types.  It is needed because forwarding a movable-but-not-copyable
// argument to another function requires us to invoke the proper move
// operator to create a rvalue version of the type.  The supported types are
// whitelisted below as overloads of the CallbackForward() function. The
// default template compiles out to be a no-op.
//
// In C++11, std::forward would replace all uses of this function.  However, it
// is impossible to implement a general std::forward with C++11 due to a lack
// of rvalue references.
//
// In addition to Callback/Bind, this is used by PostTaskAndReplyWithResult to
// simulate std::forward() and forward the result of one Callback as a
// parameter to another callback. This is to support Callbacks that return
// the movable-but-not-copyable types whitelisted above.
template <typename T>
typename enable_if<!IsMoveOnlyType<T>::value, T>::type& CallbackForward(T& t) {
  return t;
}

template <typename T>
typename enable_if<IsMoveOnlyType<T>::value, T>::type CallbackForward(T& t) {
  return t.Pass();
}

}  // namespace cef_internal
}  // namespace base

#endif  // CEF_INCLUDE_BASE_INTERNAL_CEF_CALLBACK_INTERNAL_H_
