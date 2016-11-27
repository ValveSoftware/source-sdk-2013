// Copyright (c) 2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_COMMON_SCHEME_TEST_COMMON_H_
#define CEF_TESTS_CEFCLIENT_COMMON_SCHEME_TEST_COMMON_H_
#pragma once

#include <vector>

#include "include/cef_scheme.h"

namespace client {
namespace scheme_test {

// Register the custom scheme name/type. This must be done in all processes.
// See browser/scheme_test.h for creation/registration of the custom scheme
// handler which only occurs in the browser process. Called from
// client_app_delegates_common.cc.
void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar,
                           std::vector<CefString>& cookiable_schemes);

}  // namespace scheme_test
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_COMMON_SCHEME_TEST_COMMON_H_
