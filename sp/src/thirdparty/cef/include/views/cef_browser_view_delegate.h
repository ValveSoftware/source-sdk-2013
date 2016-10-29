// Copyright (c) 2016 Marshall A. Greenblatt. All rights reserved.
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

#ifndef CEF_INCLUDE_VIEWS_CEF_BROWSER_VIEW_DELEGATE_H_
#define CEF_INCLUDE_VIEWS_CEF_BROWSER_VIEW_DELEGATE_H_
#pragma once

#include "include/cef_client.h"
#include "include/views/cef_view_delegate.h"

class CefBrowser;
class CefBrowserView;

///
// Implement this interface to handle BrowserView events. The methods of this
// class will be called on the browser process UI thread unless otherwise
// indicated.
///
/*--cef(source=client)--*/
class CefBrowserViewDelegate : public CefViewDelegate {
 public:
  ///
  // Called when |browser| associated with |browser_view| is created. This
  // method will be called after CefLifeSpanHandler::OnAfterCreated() is called
  // for |browser| and before OnPopupBrowserViewCreated() is called for
  // |browser|'s parent delegate if |browser| is a popup.
  ///
  /*--cef()--*/
  virtual void OnBrowserCreated(CefRefPtr<CefBrowserView> browser_view,
                                CefRefPtr<CefBrowser> browser) {}

  ///
  // Called when |browser| associated with |browser_view| is destroyed. Release
  // all references to |browser| and do not attempt to execute any methods on
  // |browser| after this callback returns. This method will be called before
  // CefLifeSpanHandler::OnBeforeClose() is called for |browser|.
  ///
  /*--cef()--*/
  virtual void OnBrowserDestroyed(CefRefPtr<CefBrowserView> browser_view,
                                  CefRefPtr<CefBrowser> browser) {}

  ///
  // Called before a new popup BrowserView is created. The popup originated
  // from |browser_view|. |settings| and |client| are the values returned from
  // CefLifeSpanHandler::OnBeforePopup(). |is_devtools| will be true if the
  // popup will be a DevTools browser. Return the delegate that will be used for
  // the new popup BrowserView.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefBrowserViewDelegate> GetDelegateForPopupBrowserView(
      CefRefPtr<CefBrowserView> browser_view,
      const CefBrowserSettings& settings,
      CefRefPtr<CefClient> client,
      bool is_devtools) {
    return this;
  }

  ///
  // Called after |popup_browser_view| is created. This method will be called
  // after CefLifeSpanHandler::OnAfterCreated() and OnBrowserCreated() are
  // called for the new popup browser. The popup originated from |browser_view|.
  // |is_devtools| will be true if the popup is a DevTools browser. Optionally
  // add |popup_browser_view| to the views hierarchy yourself and return true.
  // Otherwise return false and a default CefWindow will be created for the
  // popup.
  ///
  /*--cef()--*/
  virtual bool OnPopupBrowserViewCreated(
      CefRefPtr<CefBrowserView> browser_view,
      CefRefPtr<CefBrowserView> popup_browser_view,
      bool is_devtools) {
    return false;
  }
};

#endif  // CEF_INCLUDE_VIEWS_CEF_BROWSER_VIEW_DELEGATE_H_
