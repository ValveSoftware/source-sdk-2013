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
//
// ---------------------------------------------------------------------------
//
// The contents of this file are only available to applications that link
// against the libcef_dll_wrapper target.
//

#ifndef CEF_INCLUDE_WRAPPER_CEF_STREAM_RESOURCE_HANDLER_H_
#define CEF_INCLUDE_WRAPPER_CEF_STREAM_RESOURCE_HANDLER_H_
#pragma once

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/base/cef_scoped_ptr.h"
#include "include/cef_base.h"
#include "include/cef_resource_handler.h"
#include "include/cef_response.h"

class CefStreamReader;

///
// Implementation of the CefResourceHandler class for reading from a CefStream.
///
class CefStreamResourceHandler : public CefResourceHandler {
 public:
  ///
  // Create a new object with default response values.
  ///
  CefStreamResourceHandler(const CefString& mime_type,
                           CefRefPtr<CefStreamReader> stream);
  ///
  // Create a new object with explicit response values.
  ///
  CefStreamResourceHandler(int status_code,
                           const CefString& status_text,
                           const CefString& mime_type,
                           CefResponse::HeaderMap header_map,
                           CefRefPtr<CefStreamReader> stream);

  virtual ~CefStreamResourceHandler();

  // CefResourceHandler methods.
  virtual bool ProcessRequest(CefRefPtr<CefRequest> request,
                              CefRefPtr<CefCallback> callback) OVERRIDE;
  virtual void GetResponseHeaders(CefRefPtr<CefResponse> response,
                                  int64& response_length,
                                  CefString& redirectUrl) OVERRIDE;
  virtual bool ReadResponse(void* data_out,
                            int bytes_to_read,
                            int& bytes_read,
                            CefRefPtr<CefCallback> callback) OVERRIDE;
  virtual void Cancel() OVERRIDE;

 private:
  void ReadOnFileThread(int bytes_to_read,
                        CefRefPtr<CefCallback> callback);

  const int status_code_;
  const CefString status_text_;
  const CefString mime_type_;
  const CefResponse::HeaderMap header_map_;
  const CefRefPtr<CefStreamReader> stream_;
  bool read_on_file_thread_;

  class Buffer;
  SCOPED_PTR(Buffer) buffer_;
#if DCHECK_IS_ON()
  // Used in debug builds to verify that |buffer_| isn't being accessed on
  // multiple threads at the same time.
  bool buffer_owned_by_file_thread_;
#endif

  IMPLEMENT_REFCOUNTING(CefStreamResourceHandler);
  DISALLOW_COPY_AND_ASSIGN(CefStreamResourceHandler);
};

#endif  // CEF_INCLUDE_WRAPPER_CEF_STREAM_RESOURCE_HANDLER_H_
