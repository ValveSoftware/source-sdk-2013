// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/wrapper/cef_closure_task.h"
#include "include/base/cef_callback.h"

namespace {

class CefClosureTask : public CefTask {
 public:
  explicit CefClosureTask(const base::Closure& closure)
      : closure_(closure) {
  }

  // CefTask method
  virtual void Execute() OVERRIDE {
    closure_.Run();
    closure_.Reset();
  }

 private:
  base::Closure closure_;

  IMPLEMENT_REFCOUNTING(CefClosureTask);
  DISALLOW_COPY_AND_ASSIGN(CefClosureTask);
};

}  // namespace

CefRefPtr<CefTask> CefCreateClosureTask(const base::Closure& closure) {
  return new CefClosureTask(closure);
}

bool CefPostTask(CefThreadId threadId, const base::Closure& closure) {
  return CefPostTask(threadId, new CefClosureTask(closure));
}

bool CefPostDelayedTask(CefThreadId threadId, const base::Closure& closure,
                        int64 delay_ms) {
  return CefPostDelayedTask(threadId, new CefClosureTask(closure), delay_ms);
}
