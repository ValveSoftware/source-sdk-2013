// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_BINDING_TEST_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_BINDING_TEST_H_
#pragma once

#include "cefclient/browser/test_runner.h"

namespace client {
namespace binding_test {

// Create message handlers. Called from test_runner.cc.
void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers);

}  // namespace binding_test
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_BINDING_TEST_H_
