// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_OTHER_H_
#define CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_OTHER_H_
#pragma once

#include "cefclient/common/client_app.h"

namespace client {

// Client app implementation for other process types.
class ClientAppOther : public ClientApp {
 public:
  ClientAppOther();

 private:
  IMPLEMENT_REFCOUNTING(ClientAppOther);
  DISALLOW_COPY_AND_ASSIGN(ClientAppOther);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_COMMON_CLIENT_APP_OTHER_H_
