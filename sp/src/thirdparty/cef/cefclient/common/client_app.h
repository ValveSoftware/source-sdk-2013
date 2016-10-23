// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_H_
#define CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_H_
#pragma once

#include <vector>

#include "include/cef_app.h"

namespace client {

// Base class for customizing process-type-based behavior.
class ClientApp : public CefApp {
 public:
  ClientApp();

  enum ProcessType {
    BrowserProcess,
    RendererProcess,
    ZygoteProcess,
    OtherProcess,
  };

  // Determine the process type based on command-line arguments.
  static ProcessType GetProcessType(CefRefPtr<CefCommandLine> command_line);

 protected:
  // Schemes that will be registered with the global cookie manager.
  std::vector<CefString> cookieable_schemes_;

 private:
  // Registers custom schemes. Implemented by cefclient in
  // client_app_delegates_common.cc
  static void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar,
                                    std::vector<CefString>& cookiable_schemes);

  // CefApp methods.
  void OnRegisterCustomSchemes(
      CefRefPtr<CefSchemeRegistrar> registrar) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(ClientApp);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_H_
