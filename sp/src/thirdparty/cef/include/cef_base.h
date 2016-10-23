// Copyright (c) 2012 Marshall A. Greenblatt. All rights reserved.
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


#ifndef CEF_INCLUDE_CEF_BASE_H_
#define CEF_INCLUDE_CEF_BASE_H_
#pragma once

#include "include/base/cef_atomic_ref_count.h"
#include "include/base/cef_build.h"
#include "include/base/cef_macros.h"

// Bring in common C++ type definitions used by CEF consumers.
#include "include/internal/cef_ptr.h"
#include "include/internal/cef_types_wrappers.h"
#if defined(OS_WIN)
#include "include/internal/cef_win.h"
#elif defined(OS_MACOSX)
#include "include/internal/cef_mac.h"
#elif defined(OS_LINUX)
#include "include/internal/cef_linux.h"
#endif

///
// Interface defining the reference count implementation methods. All framework
// classes must extend the CefBase class.
///
class CefBase {
 public:
  ///
  // Called to increment the reference count for the object. Should be called
  // for every new copy of a pointer to a given object.
  ///
  virtual void AddRef() const =0;

  ///
  // Called to decrement the reference count for the object. Returns true if
  // the reference count is 0, in which case the object should self-delete.
  ///
  virtual bool Release() const =0;

  ///
  // Returns true if the reference count is 1.
  ///
  virtual bool HasOneRef() const =0;

 protected:
  virtual ~CefBase() {}
};

///
// Class that implements atomic reference counting.
///
class CefRefCount {
 public:
  CefRefCount() : ref_count_(0) {}

  ///
  // Increment the reference count.
  ///
  void AddRef() const {
    base::AtomicRefCountInc(&ref_count_); 
  }

  ///
  // Decrement the reference count. Returns true if the reference count is 0.
  ///
  bool Release() const {
    return !base::AtomicRefCountDec(&ref_count_);
  }

  ///
  // Returns true if the reference count is 1.
  ///
  bool HasOneRef() const {
    return base::AtomicRefCountIsOne(&ref_count_);
  }

 private:
  mutable base::AtomicRefCount ref_count_;
  DISALLOW_COPY_AND_ASSIGN(CefRefCount);
};

///
// Macro that provides a reference counting implementation for classes extending
// CefBase.
///
#define IMPLEMENT_REFCOUNTING(ClassName)            \
  public:                                           \
    void AddRef() const OVERRIDE {                  \
      ref_count_.AddRef();                          \
    }                                               \
    bool Release() const OVERRIDE {                 \
      if (ref_count_.Release()) {                   \
        delete static_cast<const ClassName*>(this); \
        return true;                                \
      }                                             \
      return false;                                 \
    }                                               \
    bool HasOneRef() const OVERRIDE {               \
      return ref_count_.HasOneRef();                \
    }                                               \
  private:                                          \
    CefRefCount ref_count_;

///
// Macro that provides a locking implementation. Use the Lock() and Unlock()
// methods to protect a section of code from simultaneous access by multiple
// threads. The AutoLock class is a helper that will hold the lock while in
// scope.
//
// THIS MACRO IS DEPRECATED. Use an explicit base::Lock member variable and
// base::AutoLock instead. For example:
//
// #include "include/base/cef_lock.h"
//
// // Class declaration.
// class MyClass : public CefBase {
//  public:
//   MyClass() : value_(0) {}
//   // Method that may be called on multiple threads.
//   void IncrementValue();
//  private:
//   // Value that may be accessed on multiple theads.
//   int value_;
//   // Lock used to protect access to |value_|.
//   base::Lock lock_;
//   IMPLEMENT_REFCOUNTING(MyClass);
// };
//
// // Class implementation.
// void MyClass::IncrementValue() {
//   // Acquire the lock for the scope of this method.
//   base::AutoLock lock_scope(lock_);
//   // |value_| can now be modified safely.
//   value_++;
// }
///
#define IMPLEMENT_LOCKING(ClassName)                                       \
  public:                                                                  \
    class AutoLock {                                                       \
     public:                                                               \
      explicit AutoLock(ClassName* base) : base_(base) { base_->Lock(); }  \
      ~AutoLock() { base_->Unlock(); }                                     \
     private:                                                              \
      ClassName* base_;                                                    \
      DISALLOW_COPY_AND_ASSIGN(AutoLock);                                  \
    };                                                                     \
    void Lock() { lock_.Acquire(); }                                       \
    void Unlock() { lock_.Release(); }                                     \
  private:                                                                 \
    base::Lock lock_;

#endif  // CEF_INCLUDE_CEF_BASE_H_
