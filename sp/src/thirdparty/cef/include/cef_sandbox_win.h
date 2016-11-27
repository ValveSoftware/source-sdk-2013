// Copyright (c) 2013 Marshall A. Greenblatt. All rights reserved.
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

#ifndef CEF_INCLUDE_CEF_SANDBOX_WIN_H_
#define CEF_INCLUDE_CEF_SANDBOX_WIN_H_
#pragma once

#include "include/base/cef_build.h"

#if defined(OS_WIN)

#ifdef __cplusplus
extern "C" {
#endif

// The sandbox is used to restrict sub-processes (renderer, plugin, GPU, etc)
// from directly accessing system resources. This helps to protect the user
// from untrusted and potentially malicious Web content.
// See http://www.chromium.org/developers/design-documents/sandbox for
// complete details.
//
// To enable the sandbox on Windows the following requirements must be met:
// 1. Use the same executable for the browser process and all sub-processes.
// 2. Link the executable with the cef_sandbox static library.
// 3. Call the cef_sandbox_info_create() function from within the executable
//    (not from a separate DLL) and pass the resulting pointer into both the
//    CefExecutProcess() and CefInitialize() functions via the
//    |windows_sandbox_info| parameter.

///
// Create the sandbox information object for this process. It is safe to create
// multiple of this object and to destroy the object immediately after passing
// into the CefExecutProcess() and/or CefInitialize() functions.
///
void* cef_sandbox_info_create();

///
// Destroy the specified sandbox information object.
///
void cef_sandbox_info_destroy(void* sandbox_info);

#ifdef __cplusplus
}

///
// Manages the life span of a sandbox information object.
///
class CefScopedSandboxInfo {
 public:
  CefScopedSandboxInfo() {
    sandbox_info_ = cef_sandbox_info_create();
  }
  ~CefScopedSandboxInfo() {
    cef_sandbox_info_destroy(sandbox_info_);
  }

  void* sandbox_info() const { return sandbox_info_; }

 private:
  void* sandbox_info_;
};
#endif  // __cplusplus

#endif  // defined(OS_WIN)

#endif  // CEF_INCLUDE_CEF_SANDBOX_WIN_H_
