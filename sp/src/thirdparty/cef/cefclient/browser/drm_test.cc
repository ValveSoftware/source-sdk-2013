// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/drm_test.h"

#include <algorithm>
#include <string>

#include "include/cef_parser.h"
#include "include/cef_web_plugin.h"

namespace client {
namespace drm_test {

namespace {

// Application-specific error codes.
const int kMessageFormatError = 1;
const int kCdmLoadError = 2;

const char kTestUrl[] = "http://tests/drm";
const char kWidevineCdmPathKey[] = "widevine_cdm_path";

// Callback executed once CDM registration is complete.
class CdmCallback : public CefRegisterCdmCallback {
 public:
  CdmCallback(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback)
    : callback_(callback) {
  }

  void OnCdmRegistrationComplete(cef_cdm_registration_error_t result,
                                 const CefString& error_message) OVERRIDE {
    if (result == CEF_CDM_REGISTRATION_ERROR_NONE)
      callback_->Success("");
    else
      callback_->Failure(kCdmLoadError, error_message);
    callback_ = nullptr;
  }

 private:
  CefRefPtr<CefMessageRouterBrowserSide::Callback> callback_;

  IMPLEMENT_REFCOUNTING(CdmCallback);
  DISALLOW_COPY_AND_ASSIGN(CdmCallback);
};

// Handle messages in the browser process.
class Handler : public CefMessageRouterBrowserSide::Handler {
 public:
  Handler() {}

  // Called due to cefQuery execution in binding.html.
  virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) OVERRIDE {
    // Only handle messages from the test URL.
    const std::string& url = frame->GetURL();
    if (url.find(kTestUrl) != 0)
      return false;

    // Parse |request| as a JSON dictionary.
    CefRefPtr<CefDictionaryValue> request_dict = ParseJSON(request);
    if (!request_dict) {
      callback->Failure(kMessageFormatError, "Incorrect message format");
      return true;
    }

    // Verify the "widevine_cdm_path" key.
    if (!VerifyKey(request_dict, kWidevineCdmPathKey, VTYPE_STRING, callback))
      return true;

    const std::string& widevine_cdm_path =
        request_dict->GetString(kWidevineCdmPathKey);
    if (widevine_cdm_path.empty()) {
      callback->Failure(kMessageFormatError, "Empty widevine CDM path");
      return true;
    }

    // Register the Widvine CDM.
    CefRegisterWidevineCdm(widevine_cdm_path, new CdmCallback(callback));
    return true;
  }

 private:
  // Convert a JSON string to a dictionary value.
  static CefRefPtr<CefDictionaryValue> ParseJSON(const CefString& string) {
    CefRefPtr<CefValue> value = CefParseJSON(string, JSON_PARSER_RFC);
    if (value.get() && value->GetType() == VTYPE_DICTIONARY)
      return value->GetDictionary();
    return NULL;
  }

  // Verify that |key| exists in |dictionary| and has type |value_type|. Fails
  // |callback| and returns false on failure.
  static bool VerifyKey(CefRefPtr<CefDictionaryValue> dictionary,
                        const char* key,
                        cef_value_type_t value_type,
                        CefRefPtr<Callback> callback) {
    if (!dictionary->HasKey(key) || dictionary->GetType(key) != value_type) {
      callback->Failure(kMessageFormatError,
                        "Missing or incorrectly formatted message key: " +
                        std::string(key));
      return false;
    }
    return true;
  }
};

}  // namespace

void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers) {
  handlers.insert(new Handler());
}

}  // namespace drm_test
}  // namespace client
