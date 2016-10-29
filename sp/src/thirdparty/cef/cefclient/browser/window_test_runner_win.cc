// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/window_test_runner_win.h"

#include "cefclient/browser/main_message_loop.h"

namespace client {
namespace window_test {

namespace {

HWND GetRootHwnd(CefRefPtr<CefBrowser> browser) {
  return ::GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
}

// Toggles the current display state.
void Toggle(HWND root_hwnd, UINT nCmdShow) {
  // Retrieve current window placement information.
  WINDOWPLACEMENT placement;
  ::GetWindowPlacement(root_hwnd, &placement);

  if (placement.showCmd == nCmdShow)
    ::ShowWindow(root_hwnd, SW_RESTORE);
  else
    ::ShowWindow(root_hwnd, nCmdShow);
}

void SetPosImpl(CefRefPtr<CefBrowser> browser,
                int x, int y, int width, int height) {
  HWND root_hwnd = GetRootHwnd(browser);
  if (!root_hwnd)
    return;

  // Retrieve current window placement information.
  WINDOWPLACEMENT placement;
  ::GetWindowPlacement(root_hwnd, &placement);

  // Retrieve information about the display that contains the window.
  HMONITOR monitor = MonitorFromRect(&placement.rcNormalPosition,
                                     MONITOR_DEFAULTTONEAREST);
  MONITORINFO info;
  info.cbSize = sizeof(info);
  GetMonitorInfo(monitor, &info);

  // Make sure the window is inside the display.
  CefRect display_rect(
      info.rcWork.left,
      info.rcWork.top,
      info.rcWork.right - info.rcWork.left,
      info.rcWork.bottom - info.rcWork.top);
  CefRect window_rect(x, y, width, height);
  WindowTestRunner::ModifyBounds(display_rect, window_rect);

  if (placement.showCmd == SW_MINIMIZE || placement.showCmd == SW_MAXIMIZE) {
    // The window is currently minimized or maximized. Restore it to the desired
    // position.
    placement.rcNormalPosition.left = window_rect.x;
    placement.rcNormalPosition.right = window_rect.x + window_rect.width;
    placement.rcNormalPosition.top = window_rect.y;
    placement.rcNormalPosition.bottom = window_rect.y + window_rect.height;
    ::SetWindowPlacement(root_hwnd, &placement);
    ::ShowWindow(root_hwnd, SW_RESTORE);
  } else {
    // Set the window position.
    ::SetWindowPos(root_hwnd, NULL, window_rect.x, window_rect.y,
                   window_rect.width, window_rect.height, SWP_NOZORDER);
  }
}

void MinimizeImpl(CefRefPtr<CefBrowser> browser) {
  HWND root_hwnd = GetRootHwnd(browser);
  if (!root_hwnd)
    return;
  Toggle(root_hwnd, SW_MINIMIZE);
}

void MaximizeImpl(CefRefPtr<CefBrowser> browser) {
  HWND root_hwnd = GetRootHwnd(browser);
  if (!root_hwnd)
    return;
  Toggle(root_hwnd, SW_MAXIMIZE);
}

void RestoreImpl(CefRefPtr<CefBrowser> browser) {
  HWND root_hwnd = GetRootHwnd(browser);
  if (!root_hwnd)
    return;
  ::ShowWindow(root_hwnd, SW_RESTORE);
}

}  // namespace

WindowTestRunnerWin::WindowTestRunnerWin() {
}

void WindowTestRunnerWin::SetPos(CefRefPtr<CefBrowser> browser,
                                 int x, int y, int width, int height) {
  if (CURRENTLY_ON_MAIN_THREAD()) {
    SetPosImpl(browser, x, y, width, height);
  } else {
    // Execute on the main application thread.
    MAIN_POST_CLOSURE(base::Bind(SetPosImpl, browser, x, y, width, height));
  }
}

void WindowTestRunnerWin::Minimize(CefRefPtr<CefBrowser> browser) {
  if (CURRENTLY_ON_MAIN_THREAD()) {
    MinimizeImpl(browser);
  } else {
    // Execute on the main application thread.
    MAIN_POST_CLOSURE(base::Bind(MinimizeImpl, browser));
  }
}

void WindowTestRunnerWin::Maximize(CefRefPtr<CefBrowser> browser) {
  if (CURRENTLY_ON_MAIN_THREAD()) {
    MaximizeImpl(browser);
  } else {
    // Execute on the main application thread.
    MAIN_POST_CLOSURE(base::Bind(MaximizeImpl, browser));
  }
}

void WindowTestRunnerWin::Restore(CefRefPtr<CefBrowser> browser) {
  if (CURRENTLY_ON_MAIN_THREAD()) {
    RestoreImpl(browser);
  } else {
    // Execute on the main application thread.
    MAIN_POST_CLOSURE(base::Bind(RestoreImpl, browser));
  }
}

}  // namespace window_test
}  // namespace client
