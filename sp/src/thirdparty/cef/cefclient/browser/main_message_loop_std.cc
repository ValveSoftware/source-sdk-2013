// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/main_message_loop_std.h"

#include "include/cef_app.h"

namespace client {

MainMessageLoopStd::MainMessageLoopStd() {
}

int MainMessageLoopStd::Run() {
  CefRunMessageLoop();
  return 0;
}

void MainMessageLoopStd::Quit() {
  CefQuitMessageLoop();
}

void MainMessageLoopStd::PostTask(CefRefPtr<CefTask> task) {
  CefPostTask(TID_UI, task);
}

bool MainMessageLoopStd::RunsTasksOnCurrentThread() const {
  return CefCurrentlyOn(TID_UI);
}

#if defined(OS_WIN)
void MainMessageLoopStd::SetCurrentModelessDialog(HWND hWndDialog) {
  // Nothing to do here. The Chromium message loop implementation will
  // internally route dialog messages.
}
#endif

}  // namespace client
