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
//
// CancelableCallback is a wrapper around base::Callback that allows
// cancellation of a callback. CancelableCallback takes a reference on the
// wrapped callback until this object is destroyed or Reset()/Cancel() are
// called.
//
// NOTE:
//
// Calling CancelableCallback::Cancel() brings the object back to its natural,
// default-constructed state, i.e., CancelableCallback::callback() will return
// a null callback.
//
// THREAD-SAFETY:
//
// CancelableCallback objects must be created on, posted to, cancelled on, and
// destroyed on the same thread.
//
//
// EXAMPLE USAGE:
//
// In the following example, the test is verifying that RunIntensiveTest()
// Quit()s the message loop within 4 seconds. The cancelable callback is posted
// to the message loop, the intensive test runs, the message loop is run,
// then the callback is cancelled.
//
// void TimeoutCallback(const std::string& timeout_message) {
//   FAIL() << timeout_message;
//   MessageLoop::current()->QuitWhenIdle();
// }
//
// CancelableClosure timeout(base::Bind(&TimeoutCallback, "Test timed out."));
// MessageLoop::current()->PostDelayedTask(FROM_HERE, timeout.callback(),
//                                         4000)  // 4 seconds to run.
// RunIntensiveTest();
// MessageLoop::current()->Run();
// timeout.Cancel();  // Hopefully this is hit before the timeout callback runs.
//

#ifndef CEF_INCLUDE_BASE_CEF_CANCELABLE_CALLBACK_H_
#define CEF_INCLUDE_BASE_CEF_CANCELABLE_CALLBACK_H_
#pragma once

#if defined(BASE_CANCELABLE_CALLBACK_H_)
// Do nothing if the Chromium header has already been included.
// This can happen in cases where Chromium code is used directly by the
// client application. When using Chromium code directly always include
// the Chromium header first to avoid type conflicts.
#elif defined(USING_CHROMIUM_INCLUDES)
// When building CEF include the Chromium header directly.
#include "base/cancelable_callback.h"
#else  // !USING_CHROMIUM_INCLUDES
// The following is substantially similar to the Chromium implementation.
// If the Chromium implementation diverges the below implementation should be
// updated to match.

#include "include/base/cef_bind.h"
#include "include/base/cef_callback.h"
#include "include/base/cef_build.h"
#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/base/cef_weak_ptr.h"
#include "include/base/internal/cef_callback_internal.h"

namespace base {

template <typename Sig>
class CancelableCallback;

template <>
class CancelableCallback<void(void)> {
 public:
  CancelableCallback() : weak_factory_(this) {}

  // |callback| must not be null.
  explicit CancelableCallback(const base::Callback<void(void)>& callback)
      : weak_factory_(this),
        callback_(callback) {
    DCHECK(!callback.is_null());
    InitializeForwarder();
  }

  ~CancelableCallback() {}

  // Cancels and drops the reference to the wrapped callback.
  void Cancel() {
    weak_factory_.InvalidateWeakPtrs();
    forwarder_.Reset();
    callback_.Reset();
  }

  // Returns true if the wrapped callback has been cancelled.
  bool IsCancelled() const {
    return callback_.is_null();
  }

  // Sets |callback| as the closure that may be cancelled. |callback| may not
  // be null. Outstanding and any previously wrapped callbacks are cancelled.
  void Reset(const base::Callback<void(void)>& callback) {
    DCHECK(!callback.is_null());

    // Outstanding tasks (e.g., posted to a message loop) must not be called.
    Cancel();

    // |forwarder_| is no longer valid after Cancel(), so re-bind.
    InitializeForwarder();

    callback_ = callback;
  }

  // Returns a callback that can be disabled by calling Cancel().
  const base::Callback<void(void)>& callback() const {
    return forwarder_;
  }

 private:
  void Forward() {
    callback_.Run();
  }

  // Helper method to bind |forwarder_| using a weak pointer from
  // |weak_factory_|.
  void InitializeForwarder() {
    forwarder_ = base::Bind(&CancelableCallback<void(void)>::Forward,
                            weak_factory_.GetWeakPtr());
  }

  // Used to ensure Forward() is not run when this object is destroyed.
  base::WeakPtrFactory<CancelableCallback<void(void)> > weak_factory_;

  // The wrapper closure.
  base::Callback<void(void)> forwarder_;

  // The stored closure that may be cancelled.
  base::Callback<void(void)> callback_;

  DISALLOW_COPY_AND_ASSIGN(CancelableCallback);
};

template <typename A1>
class CancelableCallback<void(A1)> {
 public:
  CancelableCallback() : weak_factory_(this) {}

  // |callback| must not be null.
  explicit CancelableCallback(const base::Callback<void(A1)>& callback)
      : weak_factory_(this),
        callback_(callback) {
    DCHECK(!callback.is_null());
    InitializeForwarder();
  }

  ~CancelableCallback() {}

  // Cancels and drops the reference to the wrapped callback.
  void Cancel() {
    weak_factory_.InvalidateWeakPtrs();
    forwarder_.Reset();
    callback_.Reset();
  }

  // Returns true if the wrapped callback has been cancelled.
  bool IsCancelled() const {
    return callback_.is_null();
  }

  // Sets |callback| as the closure that may be cancelled. |callback| may not
  // be null. Outstanding and any previously wrapped callbacks are cancelled.
  void Reset(const base::Callback<void(A1)>& callback) {
    DCHECK(!callback.is_null());

    // Outstanding tasks (e.g., posted to a message loop) must not be called.
    Cancel();

    // |forwarder_| is no longer valid after Cancel(), so re-bind.
    InitializeForwarder();

    callback_ = callback;
  }

  // Returns a callback that can be disabled by calling Cancel().
  const base::Callback<void(A1)>& callback() const {
    return forwarder_;
  }

 private:
  void Forward(A1 a1) const {
    callback_.Run(a1);
  }

  // Helper method to bind |forwarder_| using a weak pointer from
  // |weak_factory_|.
  void InitializeForwarder() {
    forwarder_ = base::Bind(&CancelableCallback<void(A1)>::Forward,
                            weak_factory_.GetWeakPtr());
  }

  // Used to ensure Forward() is not run when this object is destroyed.
  base::WeakPtrFactory<CancelableCallback<void(A1)> > weak_factory_;

  // The wrapper closure.
  base::Callback<void(A1)> forwarder_;

  // The stored closure that may be cancelled.
  base::Callback<void(A1)> callback_;

  DISALLOW_COPY_AND_ASSIGN(CancelableCallback);
};

template <typename A1, typename A2>
class CancelableCallback<void(A1, A2)> {
 public:
  CancelableCallback() : weak_factory_(this) {}

  // |callback| must not be null.
  explicit CancelableCallback(const base::Callback<void(A1, A2)>& callback)
      : weak_factory_(this),
        callback_(callback) {
    DCHECK(!callback.is_null());
    InitializeForwarder();
  }

  ~CancelableCallback() {}

  // Cancels and drops the reference to the wrapped callback.
  void Cancel() {
    weak_factory_.InvalidateWeakPtrs();
    forwarder_.Reset();
    callback_.Reset();
  }

  // Returns true if the wrapped callback has been cancelled.
  bool IsCancelled() const {
    return callback_.is_null();
  }

  // Sets |callback| as the closure that may be cancelled. |callback| may not
  // be null. Outstanding and any previously wrapped callbacks are cancelled.
  void Reset(const base::Callback<void(A1, A2)>& callback) {
    DCHECK(!callback.is_null());

    // Outstanding tasks (e.g., posted to a message loop) must not be called.
    Cancel();

    // |forwarder_| is no longer valid after Cancel(), so re-bind.
    InitializeForwarder();

    callback_ = callback;
  }

  // Returns a callback that can be disabled by calling Cancel().
  const base::Callback<void(A1, A2)>& callback() const {
    return forwarder_;
  }

 private:
  void Forward(A1 a1, A2 a2) const {
    callback_.Run(a1, a2);
  }

  // Helper method to bind |forwarder_| using a weak pointer from
  // |weak_factory_|.
  void InitializeForwarder() {
    forwarder_ = base::Bind(&CancelableCallback<void(A1, A2)>::Forward,
                            weak_factory_.GetWeakPtr());
  }

  // Used to ensure Forward() is not run when this object is destroyed.
  base::WeakPtrFactory<CancelableCallback<void(A1, A2)> > weak_factory_;

  // The wrapper closure.
  base::Callback<void(A1, A2)> forwarder_;

  // The stored closure that may be cancelled.
  base::Callback<void(A1, A2)> callback_;

  DISALLOW_COPY_AND_ASSIGN(CancelableCallback);
};

typedef CancelableCallback<void(void)> CancelableClosure;

}  // namespace base

#endif  // !USING_CHROMIUM_INCLUDES

#endif  // CEF_INCLUDE_BASE_CEF_CANCELABLE_CALLBACK_H_
