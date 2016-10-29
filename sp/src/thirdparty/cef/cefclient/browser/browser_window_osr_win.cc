// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/browser_window_osr_win.h"

#include "cefclient/browser/main_message_loop.h"
#include "cefclient/browser/util_win.h"

namespace client {

BrowserWindowOsrWin::BrowserWindowOsrWin(BrowserWindow::Delegate* delegate,
                                         const std::string& startup_url,
                                         const OsrRenderer::Settings& settings)
    : BrowserWindow(delegate),
      transparent_(settings.transparent),
      osr_hwnd_(NULL),
      device_scale_factor_(client::GetDeviceScaleFactor()) {
  osr_window_ = new OsrWindowWin(this, settings);
  client_handler_ = new ClientHandlerOsr(this, osr_window_.get(), startup_url);
}

void BrowserWindowOsrWin::CreateBrowser(
    ClientWindowHandle parent_handle,
    const CefRect& rect,
    const CefBrowserSettings& settings,
    CefRefPtr<CefRequestContext> request_context) {
  REQUIRE_MAIN_THREAD();

  // Create the new browser and native window on the UI thread.
  RECT wnd_rect = {rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
  osr_window_->CreateBrowser(parent_handle, wnd_rect, client_handler_, settings,
                             request_context, client_handler_->startup_url());
}

void BrowserWindowOsrWin::GetPopupConfig(CefWindowHandle temp_handle,
                                         CefWindowInfo& windowInfo,
                                         CefRefPtr<CefClient>& client,
                                         CefBrowserSettings& settings) {
  // Note: This method may be called on any thread.
  windowInfo.SetAsWindowless(temp_handle, transparent_);
  client = client_handler_;
}

void BrowserWindowOsrWin::ShowPopup(ClientWindowHandle parent_handle,
                                    int x, int y, size_t width, size_t height) {
  REQUIRE_MAIN_THREAD();
  if (osr_window_)
    osr_window_->ShowPopup(parent_handle, x, y, width, height);
}

void BrowserWindowOsrWin::Show() {
  REQUIRE_MAIN_THREAD();
  if (osr_window_)
    osr_window_->Show();
}

void BrowserWindowOsrWin::Hide() {
  REQUIRE_MAIN_THREAD();
  if (osr_window_)
    osr_window_->Hide();
}

void BrowserWindowOsrWin::SetBounds(int x, int y, size_t width, size_t height) {
  REQUIRE_MAIN_THREAD();
  if (osr_window_)
    osr_window_->SetBounds(x, y, width, height);
}

void BrowserWindowOsrWin::SetFocus(bool focus) {
  REQUIRE_MAIN_THREAD();
  if (osr_window_ && focus)
    osr_window_->SetFocus();
}

void BrowserWindowOsrWin::SetDeviceScaleFactor(float device_scale_factor) {
  REQUIRE_MAIN_THREAD();
  if (device_scale_factor == device_scale_factor_)
    return;

  // Apply some sanity checks.
  if (device_scale_factor < 1.0f || device_scale_factor > 4.0f)
    return;

  device_scale_factor_ = device_scale_factor;
  if (osr_window_)
    osr_window_->SetDeviceScaleFactor(device_scale_factor);
}

float BrowserWindowOsrWin::GetDeviceScaleFactor() const {
  REQUIRE_MAIN_THREAD();
  return device_scale_factor_;
}

ClientWindowHandle BrowserWindowOsrWin::GetWindowHandle() const {
  REQUIRE_MAIN_THREAD();
  return osr_hwnd_;
}

void BrowserWindowOsrWin::OnBrowserClosed(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();

  // Release the OSR window reference. It will be deleted on the UI thread.
  osr_window_ = NULL;

  BrowserWindow::OnBrowserClosed(browser);
}

void BrowserWindowOsrWin::OnOsrNativeWindowCreated(HWND hwnd) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!osr_hwnd_);
  osr_hwnd_ = hwnd;
}

}  // namespace client
