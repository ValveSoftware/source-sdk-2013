// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_OSR_WINDOW_WIN_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_OSR_WINDOW_WIN_H_
#pragma once

#include "include/base/cef_bind.h"
#include "include/base/cef_ref_counted.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "cefclient/browser/client_handler_osr.h"
#include "cefclient/browser/osr_dragdrop_win.h"
#include "cefclient/browser/osr_renderer.h"

namespace client {

// Represents the native parent window for an off-screen browser. This object
// must live on the CEF UI thread in order to handle CefRenderHandler callbacks.
// The methods of this class are thread-safe unless otherwise indicated.
class OsrWindowWin :
    public base::RefCountedThreadSafe<OsrWindowWin, CefDeleteOnUIThread>,
    public ClientHandlerOsr::OsrDelegate
 #if defined(CEF_USE_ATL)
    , public OsrDragEvents
#endif
{
 public:
  // This interface is implemented by the owner of the OsrWindowWin. The
  // methods of this class will be called on the main thread.
  class Delegate {
   public:
    // Called after the native window has been created.
    virtual void OnOsrNativeWindowCreated(HWND hwnd) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // |delegate| must outlive this object.
  OsrWindowWin(Delegate* delegate,
               const OsrRenderer::Settings& settings);

  // Create a new browser and native window.
  void CreateBrowser(HWND parent_hwnd,
                     const RECT& rect,
                     CefRefPtr<CefClient> handler,
                     const CefBrowserSettings& settings,
                     CefRefPtr<CefRequestContext> request_context,
                     const std::string& startup_url);
  
  // Show the popup window with correct parent and bounds in parent coordinates.
  void ShowPopup(HWND parent_hwnd, int x, int y, size_t width, size_t height);

  void Show();
  void Hide();
  void SetBounds(int x, int y, size_t width, size_t height);
  void SetFocus();
  void SetDeviceScaleFactor(float device_scale_factor);

 private:
  // Only allow deletion via scoped_refptr.
  friend struct CefDeleteOnThread<TID_UI>;
  friend class base::RefCountedThreadSafe<OsrWindowWin, CefDeleteOnUIThread>;

  ~OsrWindowWin();

  // Manage native window lifespan.
  void Create(HWND parent_hwnd, const RECT& rect);
  void Destroy();

  // Manage GL context lifespan.
  void EnableGL();
  void DisableGL();

  // Redraw what is currently in the texture.
  void Invalidate();
  void Render();

  void NotifyNativeWindowCreated(HWND hwnd);

  static void RegisterOsrClass(HINSTANCE hInstance,
                               HBRUSH background_brush);
  static LRESULT CALLBACK OsrWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                     LPARAM lParam);

  // WndProc message handlers.
  void OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
  void OnSize();
  void OnFocus(bool setFocus);
  void OnCaptureLost();
  void OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam);
  void OnPaint();
  bool OnEraseBkgnd();

  // Manage popup bounds.
  bool IsOverPopupWidget(int x, int y) const;
  int GetPopupXOffset() const;
  int GetPopupYOffset() const;
  void ApplyPopupOffset(int& x, int& y) const;

  // ClientHandlerOsr::OsrDelegate methods.
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
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
                      CefRenderHandler::CursorType type,
                      const CefCursorInfo& custom_cursor_info) OVERRIDE;
  bool StartDragging(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefDragData> drag_data,
                     CefRenderHandler::DragOperationsMask allowed_ops,
                     int x, int y) OVERRIDE;
  void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                        CefRenderHandler::DragOperation operation) OVERRIDE;

#if defined(CEF_USE_ATL)
  // OsrDragEvents methods.
  CefBrowserHost::DragOperationsMask OnDragEnter(
      CefRefPtr<CefDragData> drag_data,
      CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) OVERRIDE;
  CefBrowserHost::DragOperationsMask OnDragOver(CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) OVERRIDE;
  void OnDragLeave() OVERRIDE;
  CefBrowserHost::DragOperationsMask OnDrop(CefMouseEvent ev,
      CefBrowserHost::DragOperationsMask effect) OVERRIDE;
#endif  // defined(CEF_USE_ATL)

  // Only accessed on the main thread.
  Delegate* delegate_;

  // The below members are only accessed on the UI thread.
  OsrRenderer renderer_;
  HWND hwnd_;
  HDC hdc_;
  HGLRC hrc_;

  RECT client_rect_;
  float device_scale_factor_;

  CefRefPtr<CefBrowser> browser_;

#if defined(CEF_USE_ATL)
  CComPtr<DropTargetWin> drop_target_;
  CefRenderHandler::DragOperation current_drag_op_;
#endif

  bool painting_popup_;
  bool render_task_pending_;
  bool hidden_;

  // Mouse state tracking.
  POINT last_mouse_pos_;
  POINT current_mouse_pos_;
  bool mouse_rotation_;
  bool mouse_tracking_;
  int last_click_x_;
  int last_click_y_;
  CefBrowserHost::MouseButtonType last_click_button_;
  int last_click_count_;
  double last_click_time_;
  bool last_mouse_down_on_view_;

  DISALLOW_COPY_AND_ASSIGN(OsrWindowWin);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_OSR_WINDOW_WIN_H_
