// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/root_window_manager.h"

#include <sstream>

#include "include/base/cef_bind.h"
#include "include/base/cef_logging.h"
#include "include/wrapper/cef_helpers.h"
#include "cefclient/browser/main_context.h"
#include "cefclient/browser/resource_util.h"
#include "cefclient/browser/test_runner.h"
#include "cefclient/common/client_switches.h"

namespace client {

namespace {

class ClientRequestContextHandler : public CefRequestContextHandler {
 public:
  ClientRequestContextHandler() {}

  bool OnBeforePluginLoad(const CefString& mime_type,
                          const CefString& plugin_url,
                          const CefString& top_origin_url,
                          CefRefPtr<CefWebPluginInfo> plugin_info,
                          PluginPolicy* plugin_policy) OVERRIDE {
    // Always allow the PDF plugin to load.
    if (*plugin_policy != PLUGIN_POLICY_ALLOW &&
        mime_type == "application/pdf") {
      *plugin_policy = PLUGIN_POLICY_ALLOW;
      return true;
    }

    return false;
  }

 private:
  IMPLEMENT_REFCOUNTING(ClientRequestContextHandler);
};

}  // namespace

RootWindowManager::RootWindowManager(bool terminate_when_all_windows_closed)
    : terminate_when_all_windows_closed_(terminate_when_all_windows_closed) {
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  DCHECK(command_line.get());
  request_context_per_browser_ =
      command_line->HasSwitch(switches::kRequestContextPerBrowser);
  request_context_shared_cache_ =
      command_line->HasSwitch(switches::kRequestContextSharedCache);
}

RootWindowManager::~RootWindowManager() {
  // All root windows should already have been destroyed.
  DCHECK(root_windows_.empty());
}

scoped_refptr<RootWindow> RootWindowManager::CreateRootWindow(
    bool with_controls,
    bool with_osr,
    const CefRect& bounds,
    const std::string& url) {
  CefBrowserSettings settings;
  MainContext::Get()->PopulateBrowserSettings(&settings);

  scoped_refptr<RootWindow> root_window =
      RootWindow::Create(MainContext::Get()->UseViews());
  root_window->Init(this, with_controls, with_osr, bounds, settings,
                    url.empty() ? MainContext::Get()->GetMainURL() : url);

  // Store a reference to the root window on the main thread.
  OnRootWindowCreated(root_window);

  return root_window;
}

scoped_refptr<RootWindow> RootWindowManager::CreateRootWindowAsPopup(
    bool with_controls,
    bool with_osr,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings) {
  MainContext::Get()->PopulateBrowserSettings(&settings);

  scoped_refptr<RootWindow> root_window =
      RootWindow::Create(MainContext::Get()->UseViews());
  root_window->InitAsPopup(this, with_controls, with_osr,
                           popupFeatures, windowInfo, client, settings);

  // Store a reference to the root window on the main thread.
  OnRootWindowCreated(root_window);

  return root_window;
}

scoped_refptr<RootWindow> RootWindowManager::GetWindowForBrowser(
    int browser_id) {
  REQUIRE_MAIN_THREAD();

  RootWindowSet::const_iterator it = root_windows_.begin();
  for (; it != root_windows_.end(); ++it) {
    CefRefPtr<CefBrowser> browser = (*it)->GetBrowser();
    if (browser.get() && browser->GetIdentifier() == browser_id)
      return *it;
  }
  return NULL;
}

void RootWindowManager::CloseAllWindows(bool force) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&RootWindowManager::CloseAllWindows, base::Unretained(this),
                  force));
    return;
  }

  if (root_windows_.empty())
    return;

  RootWindowSet::const_iterator it = root_windows_.begin();
  for (; it != root_windows_.end(); ++it)
    (*it)->Close(force);
}

void RootWindowManager::OnRootWindowCreated(
    scoped_refptr<RootWindow> root_window) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&RootWindowManager::OnRootWindowCreated,
                   base::Unretained(this), root_window));
    return;
  }

  root_windows_.insert(root_window);
}

CefRefPtr<CefRequestContext> RootWindowManager::GetRequestContext(
    RootWindow* root_window) {
  REQUIRE_MAIN_THREAD();

  if (request_context_per_browser_) {
    // Create a new request context for each browser.
    CefRequestContextSettings settings;

    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::GetGlobalCommandLine();
    if (command_line->HasSwitch(switches::kCachePath)) {
      if (request_context_shared_cache_) {
        // Give each browser the same cache path. The resulting context objects
        // will share the same storage internally.
        CefString(&settings.cache_path) =
            command_line->GetSwitchValue(switches::kCachePath);
      } else {
        // Give each browser a unique cache path. This will create completely
        // isolated context objects.
        std::stringstream ss;
        ss << command_line->GetSwitchValue(switches::kCachePath).ToString() <<
            time(NULL);
        CefString(&settings.cache_path) = ss.str();
      }
    }

    return CefRequestContext::CreateContext(settings,
                                            new ClientRequestContextHandler);
  }

  // All browsers will share the global request context.
  if (!shared_request_context_.get()) {
    shared_request_context_ =
        CefRequestContext::CreateContext(CefRequestContext::GetGlobalContext(),
                                         new ClientRequestContextHandler);
  }
  return shared_request_context_;
}

CefRefPtr<CefImage> RootWindowManager::GetDefaultWindowIcon() {
  REQUIRE_MAIN_THREAD();

  if (!default_window_icon_) {
    // Create the Image and load resources at different scale factors.
    default_window_icon_ = LoadImageIcon("window_icon");
  }
  return default_window_icon_;
}

void RootWindowManager::OnTest(RootWindow* root_window, int test_id) {
  REQUIRE_MAIN_THREAD();

  test_runner::RunTest(root_window->GetBrowser(), test_id);
}

void RootWindowManager::OnExit(RootWindow* root_window) {
  REQUIRE_MAIN_THREAD();

  CloseAllWindows(false);
}

void RootWindowManager::OnRootWindowDestroyed(RootWindow* root_window) {
  REQUIRE_MAIN_THREAD();

  RootWindowSet::iterator it = root_windows_.find(root_window);
  DCHECK(it != root_windows_.end());
  if (it != root_windows_.end())
    root_windows_.erase(it);

  if (terminate_when_all_windows_closed_ && root_windows_.empty()) {
    // Quit the main message loop after all windows have closed.
    MainMessageLoop::Get()->Quit();
  }
}

}  // namespace client
