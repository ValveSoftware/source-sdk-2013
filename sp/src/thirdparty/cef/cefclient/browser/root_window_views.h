// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_VIEWS_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_VIEWS_H_
#pragma once

#include <string>

#include "include/base/cef_scoped_ptr.h"
#include "cefclient/browser/client_handler.h"
#include "cefclient/browser/root_window.h"
#include "cefclient/browser/views_window.h"

namespace client {

// Views framework implementation of a top-level window in the browser process.
// The methods of this class must be called on the main thread unless otherwise
// indicated.
class RootWindowViews : public RootWindow,
                        public ClientHandler::Delegate,
                        public ViewsWindow::Delegate {
 public:
  // Constructor may be called on any thread.
  RootWindowViews();
  ~RootWindowViews();

  // RootWindow methods:
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

  // ViewsWindow::Delegate methods:
  bool WithControls() OVERRIDE;
  CefRect GetWindowBounds() OVERRIDE;
  void OnViewsWindowCreated(CefRefPtr<ViewsWindow> window) OVERRIDE;
  void OnViewsWindowDestroyed(CefRefPtr<ViewsWindow> window) OVERRIDE;
  ViewsWindow::Delegate* GetDelegateForPopup(
      CefRefPtr<CefClient> client) OVERRIDE;
  virtual void OnTest(int test_id) OVERRIDE;
  virtual void OnExit() OVERRIDE;

 protected:
  // ClientHandler::Delegate methods:
  void OnBrowserCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserClosing(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBrowserClosed(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnSetAddress(const std::string& url) OVERRIDE;
  void OnSetTitle(const std::string& title) OVERRIDE;
  void OnSetFavicon(CefRefPtr<CefImage> image) OVERRIDE;
  void OnSetFullscreen(bool fullscreen) OVERRIDE;
  void OnSetLoadingState(bool isLoading,
                         bool canGoBack,
                         bool canGoForward) OVERRIDE;
  void OnSetDraggableRegions(
      const std::vector<CefDraggableRegion>& regions) OVERRIDE;

 private:
  void CreateClientHandler(const std::string& url);

  void InitOnMainThread(const CefBrowserSettings& settings,
                        const std::string& startup_url);
  void CreateViewsWindow(const CefBrowserSettings& settings,
                         const std::string& startup_url,
                         CefRefPtr<CefRequestContext> request_context);

  void NotifyViewsWindowDestroyed();
  void NotifyDestroyedIfDone();

  // After initialization all members are only accessed on the main thread
  // unless otherwise indicated.
  // Members set during initialization.
  RootWindow::Delegate* delegate_;
  bool with_controls_;
  bool is_popup_;
  CefRect initial_bounds_;
  CefRefPtr<ClientHandler> client_handler_;

  bool initialized_;
  bool window_destroyed_;
  bool browser_destroyed_;

  CefRefPtr<CefBrowser> browser_;

  // Only accessed on the browser process UI thread.
  CefRefPtr<ViewsWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowViews);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_ROOT_WINDOW_VIEWS_H_
