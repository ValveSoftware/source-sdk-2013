// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_RENDERER_PERFORMANCE_TEST_SETUP_H_
#define CEF_TESTS_CEFCLIENT_RENDERER_PERFORMANCE_TEST_SETUP_H_
#pragma once

#include "include/base/cef_macros.h"
#include "include/base/cef_logging.h"

namespace client {
namespace performance_test {

// Default number of iterations.
extern const int kDefaultIterations;

// Test name.
#define PERF_TEST_NAME(name) PerfTest##name

// Entry in test array.
#define PERF_TEST_ENTRY_EX(name, iterations) \
    { #name, PERF_TEST_NAME(name), iterations }
#define PERF_TEST_ENTRY(name) PERF_TEST_ENTRY_EX(name, kDefaultIterations)

// Test function declaration.
#define PERF_TEST_RESULT int64
#define PERF_TEST_PARAM_ITERATIONS iterations
#define PERF_TEST_PARAMS int PERF_TEST_PARAM_ITERATIONS
#define PERF_TEST_FUNC(name) \
    PERF_TEST_RESULT PERF_TEST_NAME(name)(PERF_TEST_PARAMS)

// Typedef for test pointers.
typedef PERF_TEST_RESULT(PerfTest(PERF_TEST_PARAMS));

class CefTimer {
 public:
  CefTimer() : running_(false) {
  }

  bool IsRunning() { return running_; }

  void Start() {
    DCHECK(!running_);
    running_ = true;
    start_.Now();
  }

  void Stop() {
    stop_.Now();
    DCHECK(running_);
    running_ = false;
  }

  int64 Delta() {
    DCHECK(!running_);
    return start_.Delta(stop_);
  }

 private:
  bool running_;
  CefTime start_;
  CefTime stop_;

  DISALLOW_COPY_AND_ASSIGN(CefTimer);
};

// Peform test iterations using a user-provided timing result variable.
#define PERF_ITERATIONS_START_EX() \
    { \
      CefTimer _timer; \
      _timer.Start(); \
      for (int _i = 0; _i < PERF_TEST_PARAM_ITERATIONS; ++_i) {

#define PERF_ITERATIONS_END_EX(result) \
      } \
      _timer.Stop(); \
      result = _timer.Delta(); \
    }

// Perform test iterations and return the timing result.
#define PERF_ITERATIONS_START() \
    int64 _result = 0; \
    PERF_ITERATIONS_START_EX()

#define PERF_ITERATIONS_END() \
    PERF_ITERATIONS_END_EX(_result) \
    return _result;

// Perf test entry structure.
struct PerfTestEntry {
  const char* name;
  PerfTest* test;
  int iterations;
};

// Array of perf tests.
extern const PerfTestEntry kPerfTests[];
extern const int kPerfTestsCount;

}  // namespace performance_test
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_RENDERER_PERFORMANCE_TEST_H_
