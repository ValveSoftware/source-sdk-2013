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
// The contents of this file must follow a specific format in order to
// support the CEF translator tool. See the translator.README.txt file in the
// tools directory for more information.
//

#ifndef CEF_INCLUDE_CEF_RENDER_HANDLER_H_
#define CEF_INCLUDE_CEF_RENDER_HANDLER_H_
#pragma once

#include "include/cef_base.h"
#include "include/cef_browser.h"
#include "include/cef_drag_data.h"
#include <vector>

///
// Implement this interface to handle events when window rendering is disabled.
// The methods of this class will be called on the UI thread.
///
/*--cef(source=client)--*/
class CefRenderHandler : public virtual CefBase {
 public:
  typedef cef_cursor_type_t CursorType;
  typedef cef_drag_operations_mask_t DragOperation;
  typedef cef_drag_operations_mask_t DragOperationsMask;
  typedef cef_paint_element_type_t PaintElementType;
  typedef std::vector<CefRect> RectList;

  ///
  // Called to retrieve the root window rectangle in screen coordinates. Return
  // true if the rectangle was provided.
  ///
  /*--cef()--*/
  virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect) { return false; }

  ///
  // Called to retrieve the view rectangle which is relative to screen
  // coordinates. Return true if the rectangle was provided.
  ///
  /*--cef()--*/
  virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) =0;

  ///
  // Called to retrieve the translation from view coordinates to actual screen
  // coordinates. Return true if the screen coordinates were provided.
  ///
  /*--cef()--*/
  virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
                              int viewX,
                              int viewY,
                              int& screenX,
                              int& screenY) { return false; }

  ///
  // Called to allow the client to fill in the CefScreenInfo object with
  // appropriate values. Return true if the |screen_info| structure has been
  // modified.
  //
  // If the screen info rectangle is left empty the rectangle from GetViewRect
  // will be used. If the rectangle is still empty or invalid popups may not be
  // drawn correctly.
  ///
  /*--cef()--*/
  virtual bool GetScreenInfo(CefRefPtr<CefBrowser> browser,
                             CefScreenInfo& screen_info) { return false; }

  ///
  // Called when the browser wants to show or hide the popup widget. The popup
  // should be shown if |show| is true and hidden if |show| is false.
  ///
  /*--cef()--*/
  virtual void OnPopupShow(CefRefPtr<CefBrowser> browser,
                           bool show) {}

  ///
  // Called when the browser wants to move or resize the popup widget. |rect|
  // contains the new location and size in view coordinates.
  ///
  /*--cef()--*/
  virtual void OnPopupSize(CefRefPtr<CefBrowser> browser,
                           const CefRect& rect) {}

  ///
  // Called when an element should be painted. Pixel values passed to this
  // method are scaled relative to view coordinates based on the value of
  // CefScreenInfo.device_scale_factor returned from GetScreenInfo. |type|
  // indicates whether the element is the view or the popup widget. |buffer|
  // contains the pixel data for the whole image. |dirtyRects| contains the set
  // of rectangles in pixel coordinates that need to be repainted. |buffer| will
  // be |width|*|height|*4 bytes in size and represents a BGRA image with an
  // upper-left origin.
  ///
  /*--cef()--*/
  virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                       PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer,
                       int width, int height) =0;

  ///
  // Called when the browser's cursor has changed. If |type| is CT_CUSTOM then
  // |custom_cursor_info| will be populated with the custom cursor information.
  ///
  /*--cef()--*/
  virtual void OnCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor,
                              CursorType type,
                              const CefCursorInfo& custom_cursor_info) {}

  ///
  // Called when the user starts dragging content in the web view. Contextual
  // information about the dragged content is supplied by |drag_data|.
  // (|x|, |y|) is the drag start location in screen coordinates.
  // OS APIs that run a system message loop may be used within the
  // StartDragging call.
  //
  // Return false to abort the drag operation. Don't call any of
  // CefBrowserHost::DragSource*Ended* methods after returning false.
  //
  // Return true to handle the drag operation. Call
  // CefBrowserHost::DragSourceEndedAt and DragSourceSystemDragEnded either
  // synchronously or asynchronously to inform the web view that the drag
  // operation has ended.
  ///
  /*--cef()--*/
  virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> drag_data,
                             DragOperationsMask allowed_ops,
                             int x, int y) { return false; }

  ///
  // Called when the web view wants to update the mouse cursor during a
  // drag & drop operation. |operation| describes the allowed operation
  // (none, move, copy, link).
  ///
  /*--cef()--*/
  virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                                DragOperation operation) {}

  ///
  // Called when the scroll offset has changed.
  ///
  /*--cef()--*/
  virtual void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser,
                                     double x,
                                     double y) {}
};

#endif  // CEF_INCLUDE_CEF_RENDER_HANDLER_H_
