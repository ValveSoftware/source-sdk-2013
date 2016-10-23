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

#ifndef CEF_INCLUDE_VIEWS_CEF_WINDOW_DELEGATE_H_
#define CEF_INCLUDE_VIEWS_CEF_WINDOW_DELEGATE_H_
#pragma once

#include "include/views/cef_panel_delegate.h"

class CefWindow;

///
// Implement this interface to handle window events. The methods of this class
// will be called on the browser process UI thread unless otherwise indicated.
///
/*--cef(source=client)--*/
class CefWindowDelegate : public CefPanelDelegate {
 public:
  ///
  // Called when |window| is created.
  ///
  /*--cef()--*/
  virtual void OnWindowCreated(CefRefPtr<CefWindow> window) {}

  ///
  // Called when |window| is destroyed. Release all references to |window| and
  // do not attempt to execute any methods on |window| after this callback
  // returns.
  ///
  /*--cef()--*/
  virtual void OnWindowDestroyed(CefRefPtr<CefWindow> window) {}

  ///
  // Return true if |window| should be created without a frame or title bar. The
  // window will be resizable if CanResize() returns true. Use
  // CefWindow::SetDraggableRegions() to specify draggable regions.
  ///
  /*--cef()--*/
  virtual bool IsFrameless(CefRefPtr<CefWindow> window) { return false; }

  ///
  // Return true if |window| can be resized.
  ///
  /*--cef()--*/
  virtual bool CanResize(CefRefPtr<CefWindow> window) { return true; }

  ///
  // Return true if |window| can be maximized.
  ///
  /*--cef()--*/
  virtual bool CanMaximize(CefRefPtr<CefWindow> window) { return true; }

  ///
  // Return true if |window| can be minimized.
  ///
  /*--cef()--*/
  virtual bool CanMinimize(CefRefPtr<CefWindow> window) { return true; }

  ///
  // Return true if |window| can be closed. This will be called for user-
  // initiated window close actions and when CefWindow::Close() is called.
  ///
  /*--cef()--*/
  virtual bool CanClose(CefRefPtr<CefWindow> window) { return true; }
};

#endif  // CEF_INCLUDE_VIEWS_CEF_WINDOW_DELEGATE_H_
