// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/root_window_views.h"

#include "include/base/cef_bind.h"
#include "include/base/cef_build.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_app.h"
#include "cefclient/browser/client_handler_std.h"

namespace client {

RootWindowViews::RootWindowViews()
    : delegate_(NULL),
      with_controls_(false),
      is_popup_(false),
      initialized_(false),
      window_destroyed_(false),
      browser_destroyed_(false) {
}

RootWindowViews::~RootWindowViews() {
  REQUIRE_MAIN_THREAD();
}

void RootWindowViews::Init(RootWindow::Delegate* delegate,
                           bool with_controls,
                           bool with_osr,
                           const CefRect& bounds,
                           const CefBrowserSettings& settings,
                           const std::string& url) {
  DCHECK(delegate);
  DCHECK(!with_osr);  // Windowless rendering is not supported.
  DCHECK(!initialized_);

  delegate_ = delegate;
  with_controls_ = with_controls;
  initial_bounds_ = bounds;
  CreateClientHandler(url);
  initialized_ = true;

  // Continue initialization on the main thread.
  InitOnMainThread(settings, url);
}

void RootWindowViews::InitAsPopup(RootWindow::Delegate* delegate,
                                  bool with_controls,
                                  bool with_osr,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings) {
  DCHECK(delegate);
  DCHECK(!with_osr);  // Windowless rendering is not supported.
  DCHECK(!initialized_);

  delegate_ = delegate;
  with_controls_ = with_controls;
  is_popup_ = true;

  if (popupFeatures.xSet)
    initial_bounds_.x = popupFeatures.x;
  if (popupFeatures.ySet)
    initial_bounds_.y = popupFeatures.y;
  if (popupFeatures.widthSet)
    initial_bounds_.width = popupFeatures.width;
  if (popupFeatures.heightSet)
    initial_bounds_.height = popupFeatures.height;

  CreateClientHandler(std::string());
  initialized_ = true;

  // The Window will be created in ViewsWindow::OnPopupBrowserViewCreated().
  client = client_handler_;
}

void RootWindowViews::Show(ShowMode mode) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&RootWindowViews::Show, this, mode));
    return;
  }

  if (!window_)
    return;

  window_->Show();

  switch (mode) {
    case ShowMinimized:
      window_->Minimize();
      break;
    case ShowMaximized:
      window_->Maximize();
      break;
    default:
      break;
  }
}

void RootWindowViews::Hide() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&RootWindowViews::Hide, this));
    return;
  }

  if (window_)
    window_->Hide();
}

void RootWindowViews::SetBounds(int x, int y, size_t width, size_t height) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::SetBounds, this, x, y, width, height));
    return;
  }

  if (window_) {
    window_->SetBounds(
        CefRect(x, y, static_cast<int>(width), static_cast<int>(height)));
  }
}

void RootWindowViews::Close(bool force) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::Close, this, force));
    return;
  }

  if (window_)
    window_->Close(force);
}

void RootWindowViews::SetDeviceScaleFactor(float device_scale_factor) {
  REQUIRE_MAIN_THREAD();
  // Windowless rendering is not supported.
  NOTREACHED();
}

float RootWindowViews::GetDeviceScaleFactor() const {
  REQUIRE_MAIN_THREAD();
  // Windowless rendering is not supported.
  NOTREACHED();
  return 0.0;
}

CefRefPtr<CefBrowser> RootWindowViews::GetBrowser() const {
  REQUIRE_MAIN_THREAD();
  return browser_;
}

ClientWindowHandle RootWindowViews::GetWindowHandle() const {
  REQUIRE_MAIN_THREAD();
#if defined(OS_LINUX)
  // ClientWindowHandle is a GtkWidget* on Linux and we don't have one of those.
  return NULL;
#else
  if (browser_)
    return browser_->GetHost()->GetWindowHandle();
  return kNullWindowHandle;
#endif
}

bool RootWindowViews::WithControls() {
  CEF_REQUIRE_UI_THREAD();
  return with_controls_;
}

CefRect RootWindowViews::GetWindowBounds() {
  CEF_REQUIRE_UI_THREAD();
  return initial_bounds_;
}

void RootWindowViews::OnViewsWindowCreated(CefRefPtr<ViewsWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK(!window_);
  window_ = window;
}

void RootWindowViews::OnViewsWindowDestroyed(CefRefPtr<ViewsWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  window_ = NULL;

  // Continue on the main thread.
  MAIN_POST_CLOSURE(
      base::Bind(&RootWindowViews::NotifyViewsWindowDestroyed, this));
}

ViewsWindow::Delegate* RootWindowViews::GetDelegateForPopup(
    CefRefPtr<CefClient> client) {
  CEF_REQUIRE_UI_THREAD();
  // |handler| was created in RootWindowViews::InitAsPopup().
  ClientHandlerStd* handler = static_cast<ClientHandlerStd*>(client.get());
  RootWindowViews* root_window =
      static_cast<RootWindowViews*>(handler->delegate());
  return root_window;
}

void RootWindowViews::OnTest(int test_id) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(base::Bind(&RootWindowViews::OnTest, this, test_id));
    return;
  }

  delegate_->OnTest(this, test_id);
}

void RootWindowViews::OnExit() {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(base::Bind(&RootWindowViews::OnExit, this));
    return;
  }

  delegate_->OnExit(this);
}

void RootWindowViews::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!browser_);
  browser_ = browser;
}

void RootWindowViews::OnBrowserClosing(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  // Nothing to do here.
}

void RootWindowViews::OnBrowserClosed(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  if (browser_) {
    DCHECK_EQ(browser->GetIdentifier(), browser_->GetIdentifier());
    browser_ = NULL;
  }

  client_handler_->DetachDelegate();
  client_handler_ = NULL;

  browser_destroyed_ = true;
  NotifyDestroyedIfDone();
}

void RootWindowViews::OnSetAddress(const std::string& url) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::OnSetAddress, this, url));
    return;
  }

  if (window_ && with_controls_)
    window_->SetAddress(url);
}

void RootWindowViews::OnSetTitle(const std::string& title) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&RootWindowViews::OnSetTitle, this, title));
    return;
  }

  if (window_)
    window_->SetTitle(title);
}

void RootWindowViews::OnSetFavicon(CefRefPtr<CefImage> image) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::OnSetFavicon, this, image));
    return;
  }

  if (window_)
    window_->SetFavicon(image);
}

void RootWindowViews::OnSetFullscreen(bool fullscreen) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::OnSetFullscreen, this, fullscreen));
    return;
  }

  if (window_)
    window_->SetFullscreen(fullscreen);
}

void RootWindowViews::OnSetLoadingState(bool isLoading,
                                        bool canGoBack,
                                        bool canGoForward) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::OnSetLoadingState, this, isLoading,
                   canGoBack, canGoForward));
    return;
  }

  if (window_) {
    if (with_controls_)
      window_->SetLoadingState(isLoading, canGoBack, canGoForward);

    if (isLoading) {
      // Reset to the default window icon when loading begins.
      window_->SetFavicon(delegate_->GetDefaultWindowIcon());
    }
  }
}

void RootWindowViews::OnSetDraggableRegions(
    const std::vector<CefDraggableRegion>& regions) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::OnSetDraggableRegions, this, regions));
    return;
  }

  if (window_)
    window_->SetDraggableRegions(regions);
}

void RootWindowViews::CreateClientHandler(const std::string& url) {
  DCHECK(!client_handler_);

  client_handler_ = new ClientHandlerStd(this, url);
  client_handler_->set_download_favicon_images(true);
}

void RootWindowViews::InitOnMainThread(
    const CefBrowserSettings& settings,
    const std::string& startup_url) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&RootWindowViews::InitOnMainThread, this, settings,
                   startup_url));
    return;
  }

  CreateViewsWindow(settings, startup_url, delegate_->GetRequestContext(this));
}

void RootWindowViews::CreateViewsWindow(
    const CefBrowserSettings& settings,
    const std::string& startup_url,
    CefRefPtr<CefRequestContext> request_context) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&RootWindowViews::CreateViewsWindow, this, settings,
                   startup_url, request_context));
    return;
  }

  DCHECK(!window_);

  // Create the ViewsWindow. It will show itself after creation.
  ViewsWindow::Create(this, client_handler_, startup_url, settings,
                      request_context);
}

void RootWindowViews::NotifyViewsWindowDestroyed() {
  REQUIRE_MAIN_THREAD();
  window_destroyed_ = true;
  NotifyDestroyedIfDone();
}

void RootWindowViews::NotifyDestroyedIfDone() {
  // Notify once both the window and the browser have been destroyed.
  if (window_destroyed_ && browser_destroyed_)
    delegate_->OnRootWindowDestroyed(this);
}

}  // namespace client
