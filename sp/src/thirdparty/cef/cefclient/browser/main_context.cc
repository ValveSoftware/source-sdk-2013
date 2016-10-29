// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/main_context.h"

#include "include/base/cef_logging.h"

namespace client {

namespace {

MainContext* g_main_context = NULL;

}  // namespace

// static
MainContext* MainContext::Get() {
  DCHECK(g_main_context);
  return g_main_context;
}

MainContext::MainContext() {
  DCHECK(!g_main_context);
  g_main_context = this;

}

MainContext::~MainContext() {
  g_main_context = NULL;
}

}  // namespace client
