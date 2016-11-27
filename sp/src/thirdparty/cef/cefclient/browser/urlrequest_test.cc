// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/urlrequest_test.h"

#include <string>

#include "include/base/cef_bind.h"
#include "include/base/cef_callback.h"
#include "include/base/cef_logging.h"
#include "include/cef_urlrequest.h"
#include "include/wrapper/cef_helpers.h"

namespace client {
namespace urlrequest_test {

namespace {

const char kTestUrl[] = "http://tests/urlrequest";
const char kTestMessageName[] = "URLRequestTest";

// Implementation of CefURLRequestClient that stores response information. Only
// accessed on the UI thread.
class RequestClient : public CefURLRequestClient {
 public:
  // Callback to be executed on request completion.
  typedef base::Callback<void(CefURLRequest::ErrorCode /*error_code*/,
                              const std::string& /*download_data*/)> Callback;

  explicit RequestClient(const Callback& callback)
      : callback_(callback) {
    CEF_REQUIRE_UI_THREAD();
    DCHECK(!callback_.is_null());
  }

  void Detach() {
    CEF_REQUIRE_UI_THREAD();
    if (!callback_.is_null())
      callback_.Reset();
  }

  void OnRequestComplete(CefRefPtr<CefURLRequest> request) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();
    if (!callback_.is_null()) {
      callback_.Run(request->GetRequestError(), download_data_);
      callback_.Reset();
    }
  }

  void OnUploadProgress(CefRefPtr<CefURLRequest> request,
                        int64 current,
                        int64 total) OVERRIDE {
  }

  void OnDownloadProgress(CefRefPtr<CefURLRequest> request,
                          int64 current,
                          int64 total) OVERRIDE {
  }

  void OnDownloadData(CefRefPtr<CefURLRequest> request,
                      const void* data,
                      size_t data_length) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();
    download_data_ += std::string(static_cast<const char*>(data), data_length);
  }

   bool GetAuthCredentials(bool isProxy,
                           const CefString& host,
                           int port,
                           const CefString& realm,
                           const CefString& scheme,
                           CefRefPtr<CefAuthCallback> callback) OVERRIDE {
     return false;
   }

 private:
  Callback callback_;
  std::string download_data_;

  IMPLEMENT_REFCOUNTING(RequestClient);
  DISALLOW_COPY_AND_ASSIGN(RequestClient);
};

// Handle messages in the browser process. Only accessed on the UI thread.
class Handler : public CefMessageRouterBrowserSide::Handler {
 public:
  Handler() {
    CEF_REQUIRE_UI_THREAD();
  }

  ~Handler() {
    CancelPendingRequest();
  }

  // Called due to cefQuery execution in urlrequest.html.
  bool OnQuery(CefRefPtr<CefBrowser> browser,
               CefRefPtr<CefFrame> frame,
               int64 query_id,
               const CefString& request,
               bool persistent,
               CefRefPtr<Callback> callback) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();

    // Only handle messages from the test URL.
    std::string url = frame->GetURL();
    if (url.find(kTestUrl) != 0)
      return false;

    const std::string& message_name = request;
    if (message_name.find(kTestMessageName) == 0) {
      url = message_name.substr(sizeof(kTestMessageName));

      CancelPendingRequest();

      DCHECK(!callback_.get());
      DCHECK(!urlrequest_.get());

      callback_ = callback;

      // Create a CefRequest for the specified URL.
      CefRefPtr<CefRequest> cef_request = CefRequest::Create();
      cef_request->SetURL(url);
      cef_request->SetMethod("GET");

      // Callback to be executed on request completion.
      // It's safe to use base::Unretained() here because there is only one
      // RequestClient pending at any given time and we explicitly detach the
      // callback in the Handler destructor.
      const RequestClient::Callback& request_callback =
          base::Bind(&Handler::OnRequestComplete, base::Unretained(this));

      // Create and start the new CefURLRequest.
      urlrequest_ = CefURLRequest::Create(cef_request,
                                          new RequestClient(request_callback),
                                          NULL);

      return true;
    }

    return false;
  }

 private:
  // Cancel the currently pending URL request, if any.
  void CancelPendingRequest() {
    CEF_REQUIRE_UI_THREAD();

    if (urlrequest_.get()) {
      // Don't execute the callback when we explicitly cancel the request.
      static_cast<RequestClient*>(urlrequest_->GetClient().get())->Detach();

      urlrequest_->Cancel();
      urlrequest_ = NULL;
    }

    if (callback_.get()) {
      // Must always execute |callback_| before deleting it.
      callback_->Failure(ERR_ABORTED, test_runner::GetErrorString(ERR_ABORTED));
      callback_ = NULL;
    }
  }

  void OnRequestComplete(CefURLRequest::ErrorCode error_code,
                         const std::string& download_data) {
    CEF_REQUIRE_UI_THREAD();
    
    if (error_code == ERR_NONE)
      callback_->Success(download_data);
    else
      callback_->Failure(error_code, test_runner::GetErrorString(error_code));

    callback_ = NULL;
    urlrequest_ = NULL;
  }

  CefRefPtr<Callback> callback_;
  CefRefPtr<CefURLRequest> urlrequest_;

  DISALLOW_COPY_AND_ASSIGN(Handler);
};

}  // namespace

void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers) {
  handlers.insert(new Handler());
}

}  // namespace urlrequest_test
}  // namespace client
