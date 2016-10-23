// Copyright (c) 2015 Marshall A. Greenblatt. All rights reserved.
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
// The contents of this file must follow a specific format in order to
// support the CEF translator tool. See the translator.README.txt file in the
// tools directory for more information.
//

#ifndef CEF_INCLUDE_CEF_RESPONSE_FILTER_H_
#define CEF_INCLUDE_CEF_RESPONSE_FILTER_H_
#pragma once

#include "include/cef_base.h"

///
// Implement this interface to filter resource response content. The methods of
// this class will be called on the browser process IO thread.
///
/*--cef(source=client)--*/
class CefResponseFilter : public virtual CefBase {
 public:
  typedef cef_response_filter_status_t FilterStatus;

  ///
  // Initialize the response filter. Will only be called a single time. The
  // filter will not be installed if this method returns false.
  ///
  /*--cef()--*/
  virtual bool InitFilter() =0;

  ///
  // Called to filter a chunk of data. |data_in| is the input buffer containing
  // |data_in_size| bytes of pre-filter data (|data_in| will be NULL if
  // |data_in_size| is zero). |data_out| is the output buffer that can accept up
  // to |data_out_size| bytes of filtered output data. Set |data_in_read| to the
  // number of bytes that were read from |data_in|. Set |data_out_written| to
  // the number of bytes that were written into |data_out|. If some or all of
  // the pre-filter data was read successfully but more data is needed in order
  // to continue filtering (filtered output is pending) return
  // RESPONSE_FILTER_NEED_MORE_DATA. If some or all of the pre-filter data was
  // read successfully and all available filtered output has been written return
  // RESPONSE_FILTER_DONE. If an error occurs during filtering return
  // RESPONSE_FILTER_ERROR. This method will be called repeatedly until there is
  // no more data to filter (resource response is complete), |data_in_read|
  // matches |data_in_size| (all available pre-filter bytes have been read), and
  // the method returns RESPONSE_FILTER_DONE or RESPONSE_FILTER_ERROR. Do not
  // keep a reference to the buffers passed to this method.
  ///
  /*--cef(optional_param=data_in,default_retval=RESPONSE_FILTER_ERROR)--*/
  virtual FilterStatus Filter(void* data_in,
                              size_t data_in_size,
                              size_t& data_in_read,
                              void* data_out,
                              size_t data_out_size,
                              size_t& data_out_written) =0;
};

#endif  // CEF_INCLUDE_CEF_RESPONSE_FILTER_H_
