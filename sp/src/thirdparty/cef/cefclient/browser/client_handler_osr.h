// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_OSR_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_OSR_H_
#pragma once

#include "cefclient/browser/client_handler.h"

namespace client {

// Client handler implementation for windowless browsers. There will only ever
// be one browser per handler instance.
class ClientHandlerOsr : public ClientHandler,
                         public CefRenderHandler {
 public:
  // Implement this interface to receive notification of ClientHandlerOsr
  // events. The methods of this class will be called on the CEF UI thread.
  class OsrDelegate {
   public:
    // These methods match the CefLifeSpanHandler interface.
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) = 0;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) = 0;

    // These methods match the CefRenderHandler interface.
    virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                   CefRect& rect) = 0;
    virtual bool GetViewRect(CefRefPtr<CefBrowser> browser,
                             CefRect& rect) = 0;
    virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                int viewX,
                                int viewY,
                                int& screenX,
                                int& screenY) = 0;
    virtual bool GetScreenInfo(CefRefPtr<CefBrowser> browser,
                               CefScreenInfo& screen_info) = 0;
    virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) = 0;
    virtual void OnPopupSize(CefRefPtr<CefBrowser> browser,
                             const CefRect& rect) = 0;
    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                         PaintElementType type,
                         const RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height) = 0;
    virtual void OnCursorChange(
        CefRefPtr<CefBrowser> browser,
        CefCursorHandle cursor,
        CefRenderHandler::CursorType type,
        const CefCursorInfo& custom_cursor_info) = 0;
    virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefDragData> drag_data,
                               CefRenderHandler::DragOperationsMask allowed_ops,
                               int x, int y) = 0;
    virtual void UpdateDragCursor(
        CefRefPtr<CefBrowser> browser,
        CefRenderHandler::DragOperation operation) = 0;

   protected:
    virtual ~OsrDelegate() {}
  };

  ClientHandlerOsr(Delegate* delegate,
                   OsrDelegate* osr_delegate,
                   const std::string& startup_url);

  // This object may outlive the OsrDelegate object so it's necessary for the
  // OsrDelegate to detach itself before destruction.
  void DetachOsrDelegate();

  // CefClient methods.
  CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE {
    return this;
  }

  // CefLifeSpanHandler methods.
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

  // CefRenderHandler methods.
  bool GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                         CefRect& rect) OVERRIDE;
  bool GetViewRect(CefRefPtr<CefBrowser> browser,
                   CefRect& rect) OVERRIDE;
  bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
                      int viewX,
                      int viewY,
                      int& screenX,
                      int& screenY) OVERRIDE;
  bool GetScreenInfo(CefRefPtr<CefBrowser> browser,
                     CefScreenInfo& screen_info) OVERRIDE;
  void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) OVERRIDE;
  void OnPopupSize(CefRefPtr<CefBrowser> browser,
                   const CefRect& rect) OVERRIDE;
  void OnPaint(CefRefPtr<CefBrowser> browser,
               CefRenderHandler::PaintElementType type,
               const CefRenderHandler::RectList& dirtyRects,
               const void* buffer,
               int width,
               int height) OVERRIDE;
  void OnCursorChange(CefRefPtr<CefBrowser> browser,
                      CefCursorHandle cursor,
                      CursorType type,
                      const CefCursorInfo& custom_cursor_info) OVERRIDE;
  bool StartDragging(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefDragData> drag_data,
                     CefRenderHandler::DragOperationsMask allowed_ops,
                     int x, int y) OVERRIDE;
  void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                        CefRenderHandler::DragOperation operation) OVERRIDE;

 private:
  // Only accessed on the UI thread.
  OsrDelegate* osr_delegate_;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(ClientHandlerOsr);
  DISALLOW_COPY_AND_ASSIGN(ClientHandlerOsr);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_OSR_H_
