// Copyright (c) 2011 Marshall A. Greenblatt. All rights reserved.
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

#ifndef CEF_INCLUDE_CEF_REQUEST_HANDLER_H_
#define CEF_INCLUDE_CEF_REQUEST_HANDLER_H_
#pragma once

#include "include/cef_auth_callback.h"
#include "include/cef_base.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_resource_handler.h"
#include "include/cef_response.h"
#include "include/cef_response_filter.h"
#include "include/cef_request.h"
#include "include/cef_ssl_info.h"


///
// Callback interface used for asynchronous continuation of url requests.
///
/*--cef(source=library)--*/
class CefRequestCallback : public virtual CefBase {
 public:
  ///
  // Continue the url request. If |allow| is true the request will be continued.
  // Otherwise, the request will be canceled.
  ///
  /*--cef(capi_name=cont)--*/
  virtual void Continue(bool allow) =0;

  ///
  // Cancel the url request.
  ///
  /*--cef()--*/
  virtual void Cancel() =0;
};


///
// Implement this interface to handle events related to browser requests. The
// methods of this class will be called on the thread indicated.
///
/*--cef(source=client)--*/
class CefRequestHandler : public virtual CefBase {
 public:
  typedef cef_return_value_t ReturnValue;
  typedef cef_termination_status_t TerminationStatus;
  typedef cef_urlrequest_status_t URLRequestStatus;
  typedef cef_window_open_disposition_t WindowOpenDisposition;

  ///
  // Called on the UI thread before browser navigation. Return true to cancel
  // the navigation or false to allow the navigation to proceed. The |request|
  // object cannot be modified in this callback.
  // CefLoadHandler::OnLoadingStateChange will be called twice in all cases.
  // If the navigation is allowed CefLoadHandler::OnLoadStart and
  // CefLoadHandler::OnLoadEnd will be called. If the navigation is canceled
  // CefLoadHandler::OnLoadError will be called with an |errorCode| value of
  // ERR_ABORTED.
  ///
  /*--cef()--*/
  virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefRequest> request,
                              bool is_redirect) {
    return false;
  }

  ///
  // Called on the UI thread before OnBeforeBrowse in certain limited cases
  // where navigating a new or different browser might be desirable. This
  // includes user-initiated navigation that might open in a special way (e.g.
  // links clicked via middle-click or ctrl + left-click) and certain types of
  // cross-origin navigation initiated from the renderer process (e.g.
  // navigating the top-level frame to/from a file URL). The |browser| and
  // |frame| values represent the source of the navigation. The
  // |target_disposition| value indicates where the user intended to navigate
  // the browser based on standard Chromium behaviors (e.g. current tab,
  // new tab, etc). The |user_gesture| value will be true if the browser
  // navigated via explicit user gesture (e.g. clicking a link) or false if it
  // navigated automatically (e.g. via the DomContentLoaded event). Return true
  // to cancel the navigation or false to allow the navigation to proceed in the
  // source browser's top-level frame.
  ///
  /*--cef()--*/
  virtual bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                const CefString& target_url,
                                WindowOpenDisposition target_disposition,
                                bool user_gesture) {
    return false;
  }

  ///
  // Called on the IO thread before a resource request is loaded. The |request|
  // object may be modified. Return RV_CONTINUE to continue the request
  // immediately. Return RV_CONTINUE_ASYNC and call CefRequestCallback::
  // Continue() at a later time to continue or cancel the request
  // asynchronously. Return RV_CANCEL to cancel the request immediately.
  // 
  ///
  /*--cef(default_retval=RV_CONTINUE)--*/
  virtual ReturnValue OnBeforeResourceLoad(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefRequestCallback> callback) {
    return RV_CONTINUE;
  }

  ///
  // Called on the IO thread before a resource is loaded. To allow the resource
  // to load normally return NULL. To specify a handler for the resource return
  // a CefResourceHandler object. The |request| object should not be modified in
  // this callback.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefResourceHandler> GetResourceHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) {
    return NULL;
  }

  ///
  // Called on the IO thread when a resource load is redirected. The |request|
  // parameter will contain the old URL and other request-related information.
  // The |response| parameter will contain the response that resulted in the
  // redirect. The |new_url| parameter will contain the new URL and can be
  // changed if desired. The |request| object cannot be modified in this
  // callback.
  ///
  /*--cef()--*/
  virtual void OnResourceRedirect(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request,
                                  CefRefPtr<CefResponse> response,
                                  CefString& new_url) {}

  ///
  // Called on the IO thread when a resource response is received. To allow the
  // resource to load normally return false. To redirect or retry the resource
  // modify |request| (url, headers or post body) and return true. The
  // |response| object cannot be modified in this callback.
  ///
  /*--cef()--*/
  virtual bool OnResourceResponse(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request,
                                  CefRefPtr<CefResponse> response) {
    return false;
  }

  ///
  // Called on the IO thread to optionally filter resource response content.
  // |request| and |response| represent the request and response respectively
  // and cannot be modified in this callback.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefResponse> response) {
    return NULL;
  }

  ///
  // Called on the IO thread when a resource load has completed. |request| and
  // |response| represent the request and response respectively and cannot be
  // modified in this callback. |status| indicates the load completion status.
  // |received_content_length| is the number of response bytes actually read.
  ///
  /*--cef()--*/
  virtual void OnResourceLoadComplete(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefRequest> request,
                                      CefRefPtr<CefResponse> response,
                                      URLRequestStatus status,
                                      int64 received_content_length) {}

  ///
  // Called on the IO thread when the browser needs credentials from the user.
  // |isProxy| indicates whether the host is a proxy server. |host| contains the
  // hostname and |port| contains the port number. |realm| is the realm of the
  // challenge and may be empty. |scheme| is the authentication scheme used,
  // such as "basic" or "digest", and will be empty if the source of the request
  // is an FTP server. Return true to continue the request and call
  // CefAuthCallback::Continue() either in this method or at a later time when
  // the authentication information is available. Return false to cancel the
  // request immediately.
  ///
  /*--cef(optional_param=realm,optional_param=scheme)--*/
  virtual bool GetAuthCredentials(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  bool isProxy,
                                  const CefString& host,
                                  int port,
                                  const CefString& realm,
                                  const CefString& scheme,
                                  CefRefPtr<CefAuthCallback> callback) {
    return false;
  }

  ///
  // Called on the IO thread when JavaScript requests a specific storage quota
  // size via the webkitStorageInfo.requestQuota function. |origin_url| is the
  // origin of the page making the request. |new_size| is the requested quota
  // size in bytes. Return true to continue the request and call
  // CefRequestCallback::Continue() either in this method or at a later time to
  // grant or deny the request. Return false to cancel the request immediately.
  ///
  /*--cef()--*/
  virtual bool OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                              const CefString& origin_url,
                              int64 new_size,
                              CefRefPtr<CefRequestCallback> callback) {
    return false;
  }

  ///
  // Called on the UI thread to handle requests for URLs with an unknown
  // protocol component. Set |allow_os_execution| to true to attempt execution
  // via the registered OS protocol handler, if any.
  // SECURITY WARNING: YOU SHOULD USE THIS METHOD TO ENFORCE RESTRICTIONS BASED
  // ON SCHEME, HOST OR OTHER URL ANALYSIS BEFORE ALLOWING OS EXECUTION.
  ///
  /*--cef()--*/
  virtual void OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                   const CefString& url,
                                   bool& allow_os_execution) {}

  ///
  // Called on the UI thread to handle requests for URLs with an invalid
  // SSL certificate. Return true and call CefRequestCallback::Continue() either
  // in this method or at a later time to continue or cancel the request. Return
  // false to cancel the request immediately. If
  // CefSettings.ignore_certificate_errors is set all invalid certificates will
  // be accepted without calling this method.
  ///
  /*--cef()--*/
  virtual bool OnCertificateError(
      CefRefPtr<CefBrowser> browser,
      cef_errorcode_t cert_error,
      const CefString& request_url,
      CefRefPtr<CefSSLInfo> ssl_info,
      CefRefPtr<CefRequestCallback> callback) {
    return false;
  }

  ///
  // Called on the browser process UI thread when a plugin has crashed.
  // |plugin_path| is the path of the plugin that crashed.
  ///
  /*--cef()--*/
  virtual void OnPluginCrashed(CefRefPtr<CefBrowser> browser,
                               const CefString& plugin_path) {}

  ///
  // Called on the browser process UI thread when the render view associated
  // with |browser| is ready to receive/handle IPC messages in the render
  // process.
  ///
  /*--cef()--*/
  virtual void OnRenderViewReady(CefRefPtr<CefBrowser> browser) {}

  ///
  // Called on the browser process UI thread when the render process
  // terminates unexpectedly. |status| indicates how the process
  // terminated.
  ///
  /*--cef()--*/
  virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                         TerminationStatus status) {}
};

#endif  // CEF_INCLUDE_CEF_REQUEST_HANDLER_H_
