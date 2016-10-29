// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/client_handler_std.h"

namespace client {

ClientHandlerStd::ClientHandlerStd(Delegate* delegate,
                                   const std::string& startup_url)
    : ClientHandler(delegate, false, startup_url) {
}

}  // namespace client
