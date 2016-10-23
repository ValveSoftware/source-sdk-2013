// Copyright (c) 2012 Marshall A. Greenblatt. All rights reserved.
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

#ifndef CEF_INCLUDE_CEF_WEB_PLUGIN_H_
#define CEF_INCLUDE_CEF_WEB_PLUGIN_H_

#include "include/cef_base.h"

class CefBrowser;

///
// Information about a specific web plugin.
///
/*--cef(source=library)--*/
class CefWebPluginInfo : public virtual CefBase {
 public:
  ///
  // Returns the plugin name (i.e. Flash).
  ///
  /*--cef()--*/
  virtual CefString GetName() =0;

  ///
  // Returns the plugin file path (DLL/bundle/library).
  ///
  /*--cef()--*/
  virtual CefString GetPath() =0;

  ///
  // Returns the version of the plugin (may be OS-specific).
  ///
  /*--cef()--*/
  virtual CefString GetVersion() =0;

  ///
  // Returns a description of the plugin from the version information.
  ///
  /*--cef()--*/
  virtual CefString GetDescription() =0;
};

///
// Interface to implement for visiting web plugin information. The methods of
// this class will be called on the browser process UI thread.
///
/*--cef(source=client)--*/
class CefWebPluginInfoVisitor : public virtual CefBase {
 public:
  ///
  // Method that will be called once for each plugin. |count| is the 0-based
  // index for the current plugin. |total| is the total number of plugins.
  // Return false to stop visiting plugins. This method may never be called if
  // no plugins are found.
  ///
  /*--cef()--*/
  virtual bool Visit(CefRefPtr<CefWebPluginInfo> info, int count, int total) =0;
};

///
// Visit web plugin information. Can be called on any thread in the browser
// process.
///
/*--cef()--*/
void CefVisitWebPluginInfo(CefRefPtr<CefWebPluginInfoVisitor> visitor);

///
// Cause the plugin list to refresh the next time it is accessed regardless
// of whether it has already been loaded. Can be called on any thread in the
// browser process.
///
/*--cef()--*/
void CefRefreshWebPlugins();

///
// Unregister an internal plugin. This may be undone the next time
// CefRefreshWebPlugins() is called. Can be called on any thread in the browser
// process.
///
/*--cef()--*/
void CefUnregisterInternalWebPlugin(const CefString& path);

///
// Register a plugin crash. Can be called on any thread in the browser process
// but will be executed on the IO thread.
///
/*--cef()--*/
void CefRegisterWebPluginCrash(const CefString& path);

///
// Interface to implement for receiving unstable plugin information. The methods
// of this class will be called on the browser process IO thread.
///
/*--cef(source=client)--*/
class CefWebPluginUnstableCallback : public virtual CefBase {
 public:
  ///
  // Method that will be called for the requested plugin. |unstable| will be
  // true if the plugin has reached the crash count threshold of 3 times in 120
  // seconds.
  ///
  /*--cef()--*/
  virtual void IsUnstable(const CefString& path,
                          bool unstable) =0;
};

///
// Query if a plugin is unstable. Can be called on any thread in the browser
// process.
///
/*--cef()--*/
void CefIsWebPluginUnstable(const CefString& path,
                            CefRefPtr<CefWebPluginUnstableCallback> callback);

///
// Implement this interface to receive notification when CDM registration is
// complete. The methods of this class will be called on the browser process
// UI thread.
///
/*--cef(source=client)--*/
class CefRegisterCdmCallback : public virtual CefBase {
 public:
  ///
  // Method that will be called when CDM registration is complete. |result|
  // will be CEF_CDM_REGISTRATION_ERROR_NONE if registration completed
  // successfully. Otherwise, |result| and |error_message| will contain
  // additional information about why registration failed.
  ///
  /*--cef(optional_param=error_message)--*/
  virtual void OnCdmRegistrationComplete(cef_cdm_registration_error_t result,
                                         const CefString& error_message) =0;
};

///
// Register the Widevine CDM plugin.
//
// The client application is responsible for downloading an appropriate
// platform-specific CDM binary distribution from Google, extracting the
// contents, and building the required directory structure on the local machine.
// The CefBrowserHost::StartDownload method and CefZipArchive class can be used
// to implement this functionality in CEF. Contact Google via
// https://www.widevine.com/contact.html for details on CDM download.
//
// |path| is a directory that must contain the following files:
//   1. manifest.json file from the CDM binary distribution (see below).
//   2. widevinecdm file from the CDM binary distribution (e.g.
//      widevinecdm.dll on on Windows, libwidevinecdm.dylib on OS X,
//      libwidevinecdm.so on Linux).
//   3. widevidecdmadapter file from the CEF binary distribution (e.g.
//      widevinecdmadapter.dll on Windows, widevinecdmadapter.plugin on OS X,
//      libwidevinecdmadapter.so on Linux).
//
// If any of these files are missing or if the manifest file has incorrect
// contents the registration will fail and |callback| will receive a |result|
// value of CEF_CDM_REGISTRATION_ERROR_INCORRECT_CONTENTS.
//
// The manifest.json file must contain the following keys:
//   A. "os": Supported OS (e.g. "mac", "win" or "linux").
//   B. "arch": Supported architecture (e.g. "ia32" or "x64").
//   C. "x-cdm-module-versions": Module API version (e.g. "4").
//   D. "x-cdm-interface-versions": Interface API version (e.g. "8").
//   E. "x-cdm-host-versions": Host API version (e.g. "8").
//   F. "version": CDM version (e.g. "1.4.8.903").
//   G. "x-cdm-codecs": List of supported codecs (e.g. "vp8,vp9.0,avc1").
//
// A through E are used to verify compatibility with the current Chromium
// version. If the CDM is not compatible the registration will fail and
// |callback| will receive a |result| value of
// CEF_CDM_REGISTRATION_ERROR_INCOMPATIBLE.
//
// |callback| will be executed asynchronously once registration is complete.
//
// On Linux this function must be called before CefInitialize() and the
// registration cannot be changed during runtime. If registration is not
// supported at the time that CefRegisterWidevineCdm() is called then |callback|
// will receive a |result| value of CEF_CDM_REGISTRATION_ERROR_NOT_SUPPORTED.
///
/*--cef(optional_param=callback)--*/
void CefRegisterWidevineCdm(const CefString& path,
                            CefRefPtr<CefRegisterCdmCallback> callback);

#endif  // CEF_INCLUDE_CEF_WEB_PLUGIN_H_
