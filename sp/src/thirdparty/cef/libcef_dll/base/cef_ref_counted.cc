// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/base/cef_ref_counted.h"
#include "include/base/cef_thread_collision_warner.h"

namespace base {

namespace cef_subtle {

bool RefCountedThreadSafeBase::HasOneRef() const {
  return AtomicRefCountIsOne(
      &const_cast<RefCountedThreadSafeBase*>(this)->ref_count_);
}

RefCountedThreadSafeBase::RefCountedThreadSafeBase() : ref_count_(0) {
#if DCHECK_IS_ON()
  in_dtor_ = false;
#endif
}

RefCountedThreadSafeBase::~RefCountedThreadSafeBase() {
#if DCHECK_IS_ON()
  DCHECK(in_dtor_) << "RefCountedThreadSafe object deleted without "
                      "calling Release()";
#endif
}

void RefCountedThreadSafeBase::AddRef() const {
#if DCHECK_IS_ON()
  DCHECK(!in_dtor_);
#endif
  AtomicRefCountInc(&ref_count_);
}

bool RefCountedThreadSafeBase::Release() const {
#if DCHECK_IS_ON()
  DCHECK(!in_dtor_);
  DCHECK(!AtomicRefCountIsZero(&ref_count_));
#endif
  if (!AtomicRefCountDec(&ref_count_)) {
#if DCHECK_IS_ON()
    in_dtor_ = true;
#endif
    return true;
  }
  return false;
}

}  // namespace cef_subtle

}  // namespace base
