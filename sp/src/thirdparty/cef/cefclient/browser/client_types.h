// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_TYPES_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_TYPES_H_
#pragma once

#include "include/cef_base.h"

#if defined(OS_LINUX)
#include <gtk/gtk.h>

// The Linux client uses GTK instead of the underlying platform type (X11).
#define ClientWindowHandle GtkWidget*
#else
#define ClientWindowHandle CefWindowHandle
#endif

#if defined(OS_MACOSX)
// Forward declaration of ObjC types used by cefclient and not provided by
// include/internal/cef_types_mac.h.
#ifdef __cplusplus
#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif
#endif
#endif  // defined OS_MACOSX

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_TYPES_H_

