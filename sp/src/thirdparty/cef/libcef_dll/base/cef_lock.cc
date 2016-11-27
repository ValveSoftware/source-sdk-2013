// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is used for debugging assertion support.  The Lock class
// is functionally a wrapper around the LockImpl class, so the only
// real intelligence in the class is in the debugging logic.

#include "include/base/cef_lock.h"
#include "include/base/cef_logging.h"

#if DCHECK_IS_ON()

namespace base {
namespace cef_internal {

Lock::Lock() : lock_() {
}

Lock::~Lock() {
  DCHECK(owning_thread_ref_.is_null());
}

void Lock::AssertAcquired() const {
  DCHECK(owning_thread_ref_ == PlatformThread::CurrentRef());
}

void Lock::CheckHeldAndUnmark() {
  DCHECK(owning_thread_ref_ == PlatformThread::CurrentRef());
  owning_thread_ref_ = PlatformThreadRef();
}

void Lock::CheckUnheldAndMark() {
  // Hitting this DCHECK means that your code is trying to re-enter a lock that
  // is already held. The Chromium Lock implementation is not reentrant.
  // See "Why can the holder of a Lock not reacquire it?" at
  // http://www.chromium.org/developers/lock-and-condition-variable for more
  // information.
  DCHECK(owning_thread_ref_.is_null());
  owning_thread_ref_ = PlatformThread::CurrentRef();
}

}  // namespace cef_internal
}  // namespace base

#endif  // DCHECK_IS_ON()
