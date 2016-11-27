// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_APP_BROWSER_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_APP_BROWSER_H_
#pragma once

#include <set>

#include "cefclient/common/client_app.h"

namespace client {

// Client app implementation for the browser process.
class ClientAppBrowser : public ClientApp,
                         public CefBrowserProcessHandler {
 public:
  // Interface for browser delegates. All Delegates must be returned via
  // CreateDelegates. Do not perform work in the Delegate
  // constructor. See CefBrowserProcessHandler for documentation.
  class Delegate : public virtual CefBase {
   public:
    virtual void OnBeforeCommandLineProcessing(
        CefRefPtr<ClientAppBrowser> app,
        CefRefPtr<CefCommandLine> command_line) {}

    virtual void OnContextInitialized(CefRefPtr<ClientAppBrowser> app) {}

    virtual void OnBeforeChildProcessLaunch(
        CefRefPtr<ClientAppBrowser> app,
        CefRefPtr<CefCommandLine> command_line) {}

    virtual void OnRenderProcessThreadCreated(
        CefRefPtr<ClientAppBrowser> app,
        CefRefPtr<CefListValue> extra_info) {}
  };

  typedef std::set<CefRefPtr<Delegate> > DelegateSet;

  ClientAppBrowser();

 private:
  // Creates all of the Delegate objects. Implemented by cefclient in
  // client_app_delegates_browser.cc
  static void CreateDelegates(DelegateSet& delegates);

  // Create the Linux print handler. Implemented by cefclient in
  // client_app_delegates_browser.cc
  static CefRefPtr<CefPrintHandler> CreatePrintHandler();

  // CefApp methods.
  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) OVERRIDE;
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE {
    return this;
  }

  // CefBrowserProcessHandler methods.
  void OnContextInitialized() OVERRIDE;
  void OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) OVERRIDE;
  void OnRenderProcessThreadCreated(
      CefRefPtr<CefListValue> extra_info) OVERRIDE;
  CefRefPtr<CefPrintHandler> GetPrintHandler() OVERRIDE {
    return print_handler_;
  }
  void OnScheduleMessagePumpWork(int64 delay) OVERRIDE;

  // Set of supported Delegates.
  DelegateSet delegates_;

  CefRefPtr<CefPrintHandler> print_handler_;

  IMPLEMENT_REFCOUNTING(ClientAppBrowser);
  DISALLOW_COPY_AND_ASSIGN(ClientAppBrowser);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_APP_BROWSER_H_
