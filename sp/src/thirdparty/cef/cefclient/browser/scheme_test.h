// Copyright (c) 2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_SCHEME_TEST_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_SCHEME_TEST_H_
#pragma once

namespace client {
namespace scheme_test {

// Create and register the custom scheme handler. See
// common/scheme_handler_common.h for registration of the custom scheme
// name/type which must occur in all processes. Called from test_runner.cc.
void RegisterSchemeHandlers();

}  // namespace scheme_test
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_SCHEME_TEST_H_
