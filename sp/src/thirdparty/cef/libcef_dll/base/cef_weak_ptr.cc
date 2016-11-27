// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/base/cef_weak_ptr.h"

namespace base {
namespace cef_internal {

WeakReference::Flag::Flag() : is_valid_(true) {
  // Flags only become bound when checked for validity, or invalidated,
  // so that we can check that later validity/invalidation operations on
  // the same Flag take place on the same thread.
  thread_checker_.DetachFromThread();
}

void WeakReference::Flag::Invalidate() {
  // The flag being invalidated with a single ref implies that there are no
  // weak pointers in existence. Allow deletion on other thread in this case.
  DCHECK(thread_checker_.CalledOnValidThread() || HasOneRef())
      << "WeakPtrs must be invalidated on the same thread.";
  is_valid_ = false;
}

bool WeakReference::Flag::IsValid() const {
  DCHECK(thread_checker_.CalledOnValidThread())
      << "WeakPtrs must be checked on the same thread.";
  return is_valid_;
}

WeakReference::Flag::~Flag() {
}

WeakReference::WeakReference() {
}

WeakReference::WeakReference(const Flag* flag) : flag_(flag) {
}

WeakReference::~WeakReference() {
}

bool WeakReference::is_valid() const { return flag_.get() && flag_->IsValid(); }

WeakReferenceOwner::WeakReferenceOwner() {
}

WeakReferenceOwner::~WeakReferenceOwner() {
  Invalidate();
}

WeakReference WeakReferenceOwner::GetRef() const {
  // If we hold the last reference to the Flag then create a new one.
  if (!HasRefs())
    flag_ = new WeakReference::Flag();

  return WeakReference(flag_.get());
}

void WeakReferenceOwner::Invalidate() {
  if (flag_.get()) {
    flag_->Invalidate();
    flag_ = NULL;
  }
}

WeakPtrBase::WeakPtrBase() {
}

WeakPtrBase::~WeakPtrBase() {
}

WeakPtrBase::WeakPtrBase(const WeakReference& ref) : ref_(ref) {
}

}  // namespace cef_internal
}  // namespace base
