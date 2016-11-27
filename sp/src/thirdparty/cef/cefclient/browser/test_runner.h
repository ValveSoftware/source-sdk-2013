// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_TEST_RUNNER_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_TEST_RUNNER_H_
#pragma once

#include <set>
#include <string>

#include "include/cef_browser.h"
#include "include/cef_request.h"
#include "include/wrapper/cef_message_router.h"
#include "include/wrapper/cef_resource_manager.h"

namespace client {
namespace test_runner {

// Run a test.
void RunTest(CefRefPtr<CefBrowser> browser, int id);

// Returns the contents of the CefRequest as a string.
std::string DumpRequestContents(CefRefPtr<CefRequest> request);

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string& data,
                       const std::string& mime_type);

// Returns the string representation of the specified error code.
std::string GetErrorString(cef_errorcode_t code);

// Set up the resource manager for tests.
void SetupResourceManager(CefRefPtr<CefResourceManager> resource_manager);

// Show a JS alert message.
void Alert(CefRefPtr<CefBrowser> browser, const std::string& message);

// Create all CefMessageRouterBrowserSide::Handler objects. They will be
// deleted when the ClientHandler is destroyed.
typedef std::set<CefMessageRouterBrowserSide::Handler*> MessageHandlerSet;
void CreateMessageHandlers(MessageHandlerSet& handlers);

// Register scheme handlers for tests.
void RegisterSchemeHandlers();

// Create a resource response filter for tests.
CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response);

}  // namespace test_runner
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_TEST_RUNNER_H_
