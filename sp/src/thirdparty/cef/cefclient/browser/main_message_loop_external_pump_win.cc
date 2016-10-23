// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/main_message_loop_external_pump.h"

#include <CommCtrl.h>

#include "include/cef_app.h"
#include "cefclient/browser/util_win.h"

namespace client {

namespace {

// Message sent to get an additional time slice for pumping (processing) another
// task (a series of such messages creates a continuous task pump).
static const int kMsgHaveWork = WM_USER + 1;

class MainMessageLoopExternalPumpWin : public MainMessageLoopExternalPump {
 public:
  MainMessageLoopExternalPumpWin();
  ~MainMessageLoopExternalPumpWin();

  // MainMessageLoopStd methods:
  void Quit() OVERRIDE;
  int Run() OVERRIDE;

  // MainMessageLoopExternalPump methods:
  void OnScheduleMessagePumpWork(int64 delay_ms) OVERRIDE;

 protected:
  // MainMessageLoopExternalPump methods:
  void SetTimer(int64 delay_ms) OVERRIDE;
  void KillTimer() OVERRIDE;
  bool IsTimerPending() OVERRIDE { return timer_pending_; }

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);

  // True if a timer event is currently pending.
  bool timer_pending_;

  // HWND owned by the thread that CefDoMessageLoopWork should be invoked on.
  HWND main_thread_target_;
};

MainMessageLoopExternalPumpWin::MainMessageLoopExternalPumpWin()
  : timer_pending_(false),
    main_thread_target_(NULL) {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  const wchar_t* const kClassName = L"CEFMainTargetHWND";

  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = kClassName;
  RegisterClassEx(&wcex);

  // Create the message handling window.
  main_thread_target_ = CreateWindowW(kClassName, NULL, WS_OVERLAPPEDWINDOW,
      0, 0, 0, 0, HWND_MESSAGE , NULL, hInstance, NULL);
  DCHECK(main_thread_target_);
  SetUserDataPtr(main_thread_target_, this);
}

MainMessageLoopExternalPumpWin::~MainMessageLoopExternalPumpWin() {
  KillTimer();
  if (main_thread_target_)
    DestroyWindow(main_thread_target_);
}

void MainMessageLoopExternalPumpWin::Quit() {
  PostMessage(NULL, WM_QUIT, 0, 0);
}

int MainMessageLoopExternalPumpWin::Run() {
  // Run the message loop.
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  KillTimer();

  // We need to run the message pump until it is idle. However we don't have
  // that information here so we run the message loop "for a while".
  for (int i = 0; i < 10; ++i) {
    // Do some work.
    CefDoMessageLoopWork();
    
    // Sleep to allow the CEF proc to do work.
    Sleep(50);
  }
  
  return 0;
}

void MainMessageLoopExternalPumpWin::OnScheduleMessagePumpWork(int64 delay_ms) {
  // This method may be called on any thread.
  PostMessage(main_thread_target_, kMsgHaveWork, 0,
              static_cast<LPARAM>(delay_ms));
}

void MainMessageLoopExternalPumpWin::SetTimer(int64 delay_ms) {
  DCHECK(!timer_pending_);
  DCHECK_GT(delay_ms, 0);
  timer_pending_ = true;
  ::SetTimer(main_thread_target_, 1, static_cast<UINT>(delay_ms), NULL);
}

void MainMessageLoopExternalPumpWin::KillTimer() {
  if (timer_pending_) {
    ::KillTimer(main_thread_target_, 1);
    timer_pending_ = false;
  }
}

// static
LRESULT CALLBACK MainMessageLoopExternalPumpWin::WndProc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (msg == WM_TIMER || msg == kMsgHaveWork) {
    MainMessageLoopExternalPumpWin* message_loop =
        GetUserDataPtr<MainMessageLoopExternalPumpWin*>(hwnd);
    if (msg == kMsgHaveWork) {
      // OnScheduleMessagePumpWork() request.
      const int64 delay_ms = static_cast<int64>(lparam);
      message_loop->OnScheduleWork(delay_ms);
    } else {
      // Timer timed out.
      message_loop->OnTimerTimeout();
    }
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

} // namespace

// static
SCOPED_PTR(MainMessageLoopExternalPump)
MainMessageLoopExternalPump::Create() {
  return SCOPED_PTR(MainMessageLoopExternalPump)(
      new MainMessageLoopExternalPumpWin());
}

}  // namespace client
