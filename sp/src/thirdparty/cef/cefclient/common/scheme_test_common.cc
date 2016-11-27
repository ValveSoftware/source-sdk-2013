// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/common/scheme_test_common.h"

#include "include/cef_scheme.h"

namespace client {
namespace scheme_test {

void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar,
                           std::vector<CefString>& cookiable_schemes) {
  registrar->AddCustomScheme("client", true, false, false);
}

}  // namespace scheme_test
}  // namespace client
