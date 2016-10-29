// Copyright (c) 2014 Marshall A. Greenblatt. All rights reserved.
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
// ---------------------------------------------------------------------------
//
// The contents of this file are only available to applications that link
// against the libcef_dll_wrapper target.
//

#ifndef CEF_INCLUDE_WRAPPER_CEF_HELPERS_H_
#define CEF_INCLUDE_WRAPPER_CEF_HELPERS_H_
#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "include/base/cef_bind.h"
#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/cef_task.h"

#define CEF_REQUIRE_UI_THREAD()       DCHECK(CefCurrentlyOn(TID_UI));
#define CEF_REQUIRE_IO_THREAD()       DCHECK(CefCurrentlyOn(TID_IO));
#define CEF_REQUIRE_FILE_THREAD()     DCHECK(CefCurrentlyOn(TID_FILE));
#define CEF_REQUIRE_RENDERER_THREAD() DCHECK(CefCurrentlyOn(TID_RENDERER));


// Use this struct in conjuction with refcounted types to ensure that an
// object is deleted on the specified thread. For example:
//
// class Foo : public base::RefCountedThreadSafe<Foo, CefDeleteOnUIThread> {
//  public:
//   Foo();
//   void DoSomething();
//
//  private:
//   // Allow deletion via scoped_refptr only.
//   friend struct CefDeleteOnThread<TID_UI>;
//   friend class base::RefCountedThreadSafe<Foo, CefDeleteOnUIThread>;
//
//   virtual ~Foo() {}
// };
//
// base::scoped_refptr<Foo> foo = new Foo();
// foo->DoSomething();
// foo = NULL;  // Deletion of |foo| will occur on the UI thread.
//
template<CefThreadId thread>
struct CefDeleteOnThread {
  template<typename T>
  static void Destruct(const T* x) {
    if (CefCurrentlyOn(thread)) {
      delete x;
    } else {
      CefPostTask(thread,
                  base::Bind(&CefDeleteOnThread<thread>::Destruct<T>, x));
    }
  }
};

struct CefDeleteOnUIThread : public CefDeleteOnThread<TID_UI> { };
struct CefDeleteOnIOThread : public CefDeleteOnThread<TID_IO> { };
struct CefDeleteOnFileThread : public CefDeleteOnThread<TID_FILE> { };
struct CefDeleteOnRendererThread : public CefDeleteOnThread<TID_RENDERER> { };


///
// Helper class to manage a scoped copy of |argv|.
///
class CefScopedArgArray {
 public:
  CefScopedArgArray(int argc, char* argv[]) {
    // argv should have (argc + 1) elements, the last one always being NULL.
    array_ = new char*[argc + 1];
    for (int i = 0; i < argc; ++i) {
      values_.push_back(argv[i]);
      array_[i] = const_cast<char*>(values_[i].c_str());
    }
    array_[argc] = NULL;
  }
  ~CefScopedArgArray() {
    delete [] array_;
  }

  char** array() const { return array_; }

 private:
  char** array_;

  // Keep values in a vector separate from |array_| because various users may
  // modify |array_| and we still want to clean up memory properly.
  std::vector<std::string> values_;
  
  DISALLOW_COPY_AND_ASSIGN(CefScopedArgArray);
};

#endif  // CEF_INCLUDE_WRAPPER_CEF_HELPERS_H_
