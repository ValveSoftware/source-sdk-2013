// Copyright (c) 2013 Marshall A. Greenblatt. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------
//
// The contents of this file must follow a specific format in order to
// support the CEF translator tool. See the translator.README.txt file in the
// tools directory for more information.
//

#ifndef CEF_INCLUDE_CEF_REQUEST_CONTEXT_HANDLER_H_
#define CEF_INCLUDE_CEF_REQUEST_CONTEXT_HANDLER_H_
#pragma once

#include "include/cef_base.h"
#include "include/cef_cookie.h"
#include "include/cef_web_plugin.h"

///
// Implement this interface to provide handler implementations. The handler
// instance will not be released until all objects related to the context have
// been destroyed.
///
/*--cef(source=client,no_debugct_check)--*/
class CefRequestContextHandler : public virtual CefBase {
 public:
  typedef cef_plugin_policy_t PluginPolicy;

  ///
  // Called on the browser process IO thread to retrieve the cookie manager. If
  // this method returns NULL the default cookie manager retrievable via
  // CefRequestContext::GetDefaultCookieManager() will be used.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefCookieManager> GetCookieManager() { return NULL; }

  ///
  // Called on multiple browser process threads before a plugin instance is
  // loaded. |mime_type| is the mime type of the plugin that will be loaded.
  // |plugin_url| is the content URL that the plugin will load and may be empty.
  // |top_origin_url| is the URL for the top-level frame that contains the
  // plugin when loading a specific plugin instance or empty when building the
  // initial list of enabled plugins for 'navigator.plugins' JavaScript state.
  // |plugin_info| includes additional information about the plugin that will be
  // loaded. |plugin_policy| is the recommended policy. Modify |plugin_policy|
  // and return true to change the policy. Return false to use the recommended
  // policy. The default plugin policy can be set at runtime using the
  // `--plugin-policy=[allow|detect|block]` command-line flag. Decisions to mark
  // a plugin as disabled by setting |plugin_policy| to PLUGIN_POLICY_DISABLED
  // may be cached when |top_origin_url| is empty. To purge the plugin list
  // cache and potentially trigger new calls to this method call
  // CefRequestContext::PurgePluginListCache.
  ///
  /*--cef(optional_param=plugin_url,optional_param=top_origin_url)--*/
  virtual bool OnBeforePluginLoad(const CefString& mime_type,
                                  const CefString& plugin_url,
                                  const CefString& top_origin_url,
                                  CefRefPtr<CefWebPluginInfo> plugin_info,
                                  PluginPolicy* plugin_policy) {
    return false;
  }
};

#endif  // CEF_INCLUDE_CEF_REQUEST_CONTEXT_HANDLER_H_
