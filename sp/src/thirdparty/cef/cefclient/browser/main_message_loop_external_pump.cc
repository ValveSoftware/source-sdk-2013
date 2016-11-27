// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/main_message_loop_external_pump.h"

#include <climits>

#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"
#include "cefclient/browser/main_message_loop.h"

namespace client {

namespace {

// Special timer delay placeholder value. Intentionally 32-bit for Windows and
// OS X platform API compatibility.
const int32 kTimerDelayPlaceholder = INT_MAX;

// The maximum number of milliseconds we're willing to wait between calls to
// DoWork().
const int64 kMaxTimerDelay = 1000 / 30;  // 30fps

client::MainMessageLoopExternalPump* g_external_message_pump = NULL;

} // namespace

MainMessageLoopExternalPump::MainMessageLoopExternalPump()
  : is_active_(false),
    reentrancy_detected_(false) {
  DCHECK(!g_external_message_pump);
  g_external_message_pump = this;
}

MainMessageLoopExternalPump::~MainMessageLoopExternalPump() {
  g_external_message_pump = NULL;
}

MainMessageLoopExternalPump* MainMessageLoopExternalPump::Get() {
  return g_external_message_pump;
}

void MainMessageLoopExternalPump::OnScheduleWork(int64 delay_ms) {
  REQUIRE_MAIN_THREAD();

  if (delay_ms == kTimerDelayPlaceholder && IsTimerPending()) {
    // Don't set the maximum timer requested from DoWork() if a timer event is
    // currently pending.
    return;
  }

  KillTimer();

  if (delay_ms <= 0) {
    // Execute the work immediately.
    DoWork();
  } else {
    // Never wait longer than the maximum allowed time.
    if (delay_ms > kMaxTimerDelay)
      delay_ms = kMaxTimerDelay;

    // Results in call to OnTimerTimeout() after the specified delay.
    SetTimer(delay_ms);
  }
}

void MainMessageLoopExternalPump::OnTimerTimeout() {
  REQUIRE_MAIN_THREAD();

  KillTimer();
  DoWork();
}

void MainMessageLoopExternalPump::DoWork() {
  const bool was_reentrant = PerformMessageLoopWork();
  if (was_reentrant) {
    // Execute the remaining work as soon as possible.
    OnScheduleMessagePumpWork(0);
  } else if (!IsTimerPending()) {
    // Schedule a timer event at the maximum allowed time. This may be dropped
    // in OnScheduleWork() if another timer event is already in-flight.
    OnScheduleMessagePumpWork(kTimerDelayPlaceholder);
  }
}

bool MainMessageLoopExternalPump::PerformMessageLoopWork() {
  if (is_active_) {
    // When CefDoMessageLoopWork() is called there may be various callbacks
    // (such as paint and IPC messages) that result in additional calls to this
    // method. If re-entrancy is detected we must repost a request again to the
    // owner thread to ensure that the discarded call is executed in the future.
    reentrancy_detected_ = true;
    return false;
  }

  reentrancy_detected_ = false;

  is_active_ = true;
  CefDoMessageLoopWork();
  is_active_ = false;

  // |reentrancy_detected_| may have changed due to re-entrant calls to this
  // method.
  return reentrancy_detected_;
}

}  // namespace client
