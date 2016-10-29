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

#ifndef CEF_INCLUDE_CEF_RESOURCE_BUNDLE_H_
#define CEF_INCLUDE_CEF_RESOURCE_BUNDLE_H_
#pragma once

#include "include/cef_base.h"

///
// Class used for retrieving resources from the resource bundle (*.pak) files
// loaded by CEF during startup or via the CefResourceBundleHandler returned
// from CefApp::GetResourceBundleHandler. See CefSettings for additional options
// related to resource bundle loading. The methods of this class may be called
// on any thread unless otherwise indicated.
///
/*--cef(source=library,no_debugct_check)--*/
class CefResourceBundle : public virtual CefBase {
 public:
  typedef cef_scale_factor_t ScaleFactor;

  ///
  // Returns the global resource bundle instance.
  ///
  /*--cef()--*/
  static CefRefPtr<CefResourceBundle> GetGlobal();

  ///
  // Returns the localized string for the specified |string_id| or an empty
  // string if the value is not found. Include cef_pack_strings.h for a listing
  // of valid string ID values.
  ///
  /*--cef()--*/
  virtual CefString GetLocalizedString(int string_id) =0;

  ///
  // Retrieves the contents of the specified scale independent |resource_id|.
  // If the value is found then |data| and |data_size| will be populated and
  // this method will return true. If the value is not found then this method
  // will return false. The returned |data| pointer will remain resident in
  // memory and should not be freed. Include cef_pack_resources.h for a listing
  // of valid resource ID values.
  ///
  /*--cef()--*/
  virtual bool GetDataResource(int resource_id,
                               void*& data,
                               size_t& data_size) =0;

  ///
  // Retrieves the contents of the specified |resource_id| nearest the scale
  // factor |scale_factor|. Use a |scale_factor| value of SCALE_FACTOR_NONE for
  // scale independent resources or call GetDataResource instead. If the value
  // is found then |data| and |data_size| will be populated and this method will
  // return true. If the value is not found then this method will return false.
  // The returned |data| pointer will remain resident in memory and should not
  // be freed. Include cef_pack_resources.h for a listing of valid resource ID
  // values.
  ///
  /*--cef()--*/
  virtual bool GetDataResourceForScale(int resource_id,
                                       ScaleFactor scale_factor,
                                       void*& data,
                                       size_t& data_size) =0;
};

#endif  // CEF_INCLUDE_CEF_RESOURCE_BUNDLE_H_
