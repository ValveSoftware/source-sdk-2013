// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_OSR_DRAGDROP_EVENTS_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_OSR_DRAGDROP_EVENTS_H_
#pragma once

#include "include/cef_render_handler.h"
#include "cefclient/browser/client_handler.h"

namespace client {

class OsrDragEvents {
 public:
  virtual CefBrowserHost::DragOperationsMask OnDragEnter(
      CefRefPtr<CefDragData> drag_data,
      CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) = 0;

  virtual CefBrowserHost::DragOperationsMask OnDragOver(
      CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) = 0;

  virtual void OnDragLeave() = 0;

  virtual CefBrowserHost::DragOperationsMask OnDrop(
      CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) = 0;

 protected:
  virtual ~OsrDragEvents() {}
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_OSR_DRAGDROP_EVENTS_H_
