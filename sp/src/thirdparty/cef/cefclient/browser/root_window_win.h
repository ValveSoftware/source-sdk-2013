// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
#pragma once

#include <windows.h>
#include <commdlg.h>

#include <string>

#include "include/base/cef_scoped_ptr.h"
#include "cefclient/browser/browser_window.h"
#include "cefclient/browser/root_window.h"

namespace client {

// Windows implementation of a top-level native window in the browser process.
// The methods of this class must be called on the main thread unless otherwise
// indicated.
class RootWindowWin : public RootWindow,
                      public BrowserWindow::Delegate {
 public:
  // Constructor may be called on any thread.
  RootWindowWin();
  ~RootWindowWin();

  // RootWindow methods.
  void Init(RootWindow::Delegate* delegate,
            bool with_controls,
            bool with_osr,
            const CefRect& rect,
            const CefBrowserSettings& settings,
            const std::string& url) OVERRIDE;
  void InitAsPopup(RootWindow::Delegate* delegate,
                   bool with_controls,
                   bool with_osr,
                   const CefPopupFeatures& popupFeatures,
                   CefWindowInfo& windowInfo,
                   CefRefPtr<CefClient>& client,
                   CefBrowserSettings& settings) OVERRIDE;
  void Show(ShowMode mode) OVERRIDE;
  void Hide() OVERRIDE;
  void SetBounds(int x, int y, size_t width, size_t height) OVERRIDE;
  void Close(bool force) OVERRIDE;
  void SetDeviceScaleFactor(float device_scale_factor) OVERRIDE;
  float GetDeviceScaleFactor() const OVERRIDE;
  CefRefPtr<CefBrowser> GetBrowser() const OVERRIDE;
  ClientWindowHandle GetWindowHandle() const OVERRIDE;

 private:
  void CreateBrowserWindow(const std::string& startup_url);
  void CreateRootWindow(const CefBrowserSettings& settings);

  // Register the root window class.
  static void RegisterRootClass(HINSTANCE hInstance,
                                const std::wstring& window_class,
                                HBRUSH background_brush);

  // Window procedure for the edit field.
  static LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam);

  // Window procedure for the find dialog.
  static LRESULT CALLBACK FindWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam);

  // Window procedure for the root window.
  static LRESULT CALLBACK RootWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam);

  // Event handlers.
  void OnPaint();
  void OnFocus();
  void OnSize(bool minimized);
  void OnMove();
  bool OnEraseBkgnd();
  bool OnCommand(UINT id);
  void OnFind();
  void OnFindEvent();
  void OnAbout();
  bool OnClose();
  void OnDestroyed();

  // BrowserWindow::Delegate methods.
  void OnBrowserCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserWindowDestroyed() OVERRIDE;
  void OnSetAddress(const std::string& url) OVERRIDE;
  void OnSetTitle(const std::string& title) OVERRIDE;
  void OnSetFullscreen(bool fullscreen) OVERRIDE;
  void OnSetLoadingState(bool isLoading,
                         bool canGoBack,
                         bool canGoForward) OVERRIDE;
  void OnSetDraggableRegions(
      const std::vector<CefDraggableRegion>& regions) OVERRIDE;

  void NotifyDestroyedIfDone();

  // After initialization all members are only accessed on the main thread.
  // Members set during initialization.
  RootWindow::Delegate* delegate_;
  bool with_controls_;
  bool with_osr_;
  bool is_popup_;
  RECT start_rect_;
  scoped_ptr<BrowserWindow> browser_window_;
  bool initialized_;

  // Main window.
  HWND hwnd_;

  // Draggable region.
  HRGN draggable_region_;

  // Font for buttons and text fields.
  HFONT font_;

  // Buttons.
  HWND back_hwnd_;
  HWND forward_hwnd_;
  HWND reload_hwnd_;
  HWND stop_hwnd_;

  // URL text field.
  HWND edit_hwnd_;
  WNDPROC edit_wndproc_old_;

  // Find dialog.
  HWND find_hwnd_;
  UINT find_message_id_;
  WNDPROC find_wndproc_old_;

  // Find dialog state.
  FINDREPLACE find_state_;
  WCHAR find_buff_[80];
  std::wstring find_what_last_;
  bool find_next_;
  bool find_match_case_last_;

  bool window_destroyed_;
  bool browser_destroyed_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowWin);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_WIN_H_
