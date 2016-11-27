// Copyright (c) 2014 Marshall A. Greenblatt. All rights reserved.
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


#ifndef CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
#define CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
#pragma once

#include "include/base/cef_basictypes.h"
#include "include/internal/cef_string.h"
#include "include/internal/cef_string_list.h"
#include "include/internal/cef_time.h"

// Bring in platform-specific definitions.
#if defined(OS_WIN)
#include "include/internal/cef_types_win.h"
#elif defined(OS_MACOSX)
#include "include/internal/cef_types_mac.h"
#elif defined(OS_LINUX)
#include "include/internal/cef_types_linux.h"
#endif

// 32-bit ARGB color value, not premultiplied. The color components are always
// in a known order. Equivalent to the SkColor type.
typedef uint32              cef_color_t;

// Return the alpha byte from a cef_color_t value.
#define CefColorGetA(color)      (((color) >> 24) & 0xFF)
// Return the red byte from a cef_color_t value.
#define CefColorGetR(color)      (((color) >> 16) & 0xFF)
// Return the green byte from a cef_color_t value.
#define CefColorGetG(color)      (((color) >>  8) & 0xFF)
// Return the blue byte from a cef_color_t value.
#define CefColorGetB(color)      (((color) >>  0) & 0xFF)

// Return an cef_color_t value with the specified byte component values.
#define CefColorSetARGB(a, r, g, b) \
    static_cast<cef_color_t>( \
        (static_cast<unsigned>(a) << 24) | \
        (static_cast<unsigned>(r) << 16) | \
        (static_cast<unsigned>(g) << 8) | \
        (static_cast<unsigned>(b) << 0))

// Return an int64 value with the specified low and high int32 component values.
#define CefInt64Set(int32_low, int32_high) \
    static_cast<int64>((static_cast<uint32>(int32_low)) | \
        (static_cast<int64>(static_cast<int32>(int32_high))) << 32)

// Return the low int32 value from an int64 value.
#define CefInt64GetLow(int64_val) static_cast<int32>(int64_val)
// Return the high int32 value from an int64 value.
#define CefInt64GetHigh(int64_val) \
    static_cast<int32>((static_cast<int64>(int64_val) >> 32) & 0xFFFFFFFFL)


#ifdef __cplusplus
extern "C" {
#endif

///
// Log severity levels.
///
typedef enum {
  ///
  // Default logging (currently INFO logging).
  ///
  LOGSEVERITY_DEFAULT,

  ///
  // Verbose logging.
  ///
  LOGSEVERITY_VERBOSE,

  ///
  // INFO logging.
  ///
  LOGSEVERITY_INFO,

  ///
  // WARNING logging.
  ///
  LOGSEVERITY_WARNING,

  ///
  // ERROR logging.
  ///
  LOGSEVERITY_ERROR,

  ///
  // Completely disable logging.
  ///
  LOGSEVERITY_DISABLE = 99
} cef_log_severity_t;

///
// Represents the state of a setting.
///
typedef enum {
  ///
  // Use the default state for the setting.
  ///
  STATE_DEFAULT = 0,

  ///
  // Enable or allow the setting.
  ///
  STATE_ENABLED,

  ///
  // Disable or disallow the setting.
  ///
  STATE_DISABLED,
} cef_state_t;

///
// Initialization settings. Specify NULL or 0 to get the recommended default
// values. Many of these and other settings can also configured using command-
// line switches.
///
typedef struct _cef_settings_t {
  ///
  // Size of this structure.
  ///
  size_t size;

  ///
  // Set to true (1) to use a single process for the browser and renderer. This
  // run mode is not officially supported by Chromium and is less stable than
  // the multi-process default. Also configurable using the "single-process"
  // command-line switch.
  ///
  int single_process;

  ///
  // Set to true (1) to disable the sandbox for sub-processes. See
  // cef_sandbox_win.h for requirements to enable the sandbox on Windows. Also
  // configurable using the "no-sandbox" command-line switch.
  ///
  int no_sandbox;

  ///
  // The path to a separate executable that will be launched for sub-processes.
  // By default the browser process executable is used. See the comments on
  // CefExecuteProcess() for details. Also configurable using the
  // "browser-subprocess-path" command-line switch.
  ///
  cef_string_t browser_subprocess_path;

  ///
  // Set to true (1) to have the browser process message loop run in a separate
  // thread. If false (0) than the CefDoMessageLoopWork() function must be
  // called from your application message loop. This option is only supported on
  // Windows.
  ///
  int multi_threaded_message_loop;

  ///
  // Set to true (1) to control browser process main (UI) thread message pump
  // scheduling via the CefBrowserProcessHandler::OnScheduleMessagePumpWork()
  // callback. This option is recommended for use in combination with the
  // CefDoMessageLoopWork() function in cases where the CEF message loop must be
  // integrated into an existing application message loop (see additional
  // comments and warnings on CefDoMessageLoopWork). Enabling this option is not
  // recommended for most users; leave this option disabled and use either the
  // CefRunMessageLoop() function or multi_threaded_message_loop if possible.
  ///
  int external_message_pump;

  ///
  // Set to true (1) to enable windowless (off-screen) rendering support. Do not
  // enable this value if the application does not use windowless rendering as
  // it may reduce rendering performance on some systems.
  ///
  int windowless_rendering_enabled;

  ///
  // Set to true (1) to disable configuration of browser process features using
  // standard CEF and Chromium command-line arguments. Configuration can still
  // be specified using CEF data structures or via the
  // CefApp::OnBeforeCommandLineProcessing() method.
  ///
  int command_line_args_disabled;

  ///
  // The location where cache data will be stored on disk. If empty then
  // browsers will be created in "incognito mode" where in-memory caches are
  // used for storage and no data is persisted to disk. HTML5 databases such as
  // localStorage will only persist across sessions if a cache path is
  // specified. Can be overridden for individual CefRequestContext instances via
  // the CefRequestContextSettings.cache_path value.
  ///
  cef_string_t cache_path;

  ///
  // The location where user data such as spell checking dictionary files will
  // be stored on disk. If empty then the default platform-specific user data
  // directory will be used ("~/.cef_user_data" directory on Linux,
  // "~/Library/Application Support/CEF/User Data" directory on Mac OS X,
  // "Local Settings\Application Data\CEF\User Data" directory under the user
  // profile directory on Windows).
  ///
  cef_string_t user_data_path;

  ///
  // To persist session cookies (cookies without an expiry date or validity
  // interval) by default when using the global cookie manager set this value to
  // true (1). Session cookies are generally intended to be transient and most
  // Web browsers do not persist them. A |cache_path| value must also be
  // specified to enable this feature. Also configurable using the
  // "persist-session-cookies" command-line switch. Can be overridden for
  // individual CefRequestContext instances via the
  // CefRequestContextSettings.persist_session_cookies value.
  ///
  int persist_session_cookies;

  ///
  // To persist user preferences as a JSON file in the cache path directory set
  // this value to true (1). A |cache_path| value must also be specified
  // to enable this feature. Also configurable using the
  // "persist-user-preferences" command-line switch. Can be overridden for
  // individual CefRequestContext instances via the
  // CefRequestContextSettings.persist_user_preferences value.
  ///
  int persist_user_preferences;

  ///
  // Value that will be returned as the User-Agent HTTP header. If empty the
  // default User-Agent string will be used. Also configurable using the
  // "user-agent" command-line switch.
  ///
  cef_string_t user_agent;

  ///
  // Value that will be inserted as the product portion of the default
  // User-Agent string. If empty the Chromium product version will be used. If
  // |userAgent| is specified this value will be ignored. Also configurable
  // using the "product-version" command-line switch.
  ///
  cef_string_t product_version;

  ///
  // The locale string that will be passed to WebKit. If empty the default
  // locale of "en-US" will be used. This value is ignored on Linux where locale
  // is determined using environment variable parsing with the precedence order:
  // LANGUAGE, LC_ALL, LC_MESSAGES and LANG. Also configurable using the "lang"
  // command-line switch.
  ///
  cef_string_t locale;

  ///
  // The directory and file name to use for the debug log. If empty a default
  // log file name and location will be used. On Windows and Linux a "debug.log"
  // file will be written in the main executable directory. On Mac OS X a
  // "~/Library/Logs/<app name>_debug.log" file will be written where <app name>
  // is the name of the main app executable. Also configurable using the
  // "log-file" command-line switch.
  ///
  cef_string_t log_file;

  ///
  // The log severity. Only messages of this severity level or higher will be
  // logged. Also configurable using the "log-severity" command-line switch with
  // a value of "verbose", "info", "warning", "error", "error-report" or
  // "disable".
  ///
  cef_log_severity_t log_severity;

  ///
  // Custom flags that will be used when initializing the V8 JavaScript engine.
  // The consequences of using custom flags may not be well tested. Also
  // configurable using the "js-flags" command-line switch.
  ///
  cef_string_t javascript_flags;

  ///
  // The fully qualified path for the resources directory. If this value is
  // empty the cef.pak and/or devtools_resources.pak files must be located in
  // the module directory on Windows/Linux or the app bundle Resources directory
  // on Mac OS X. Also configurable using the "resources-dir-path" command-line
  // switch.
  ///
  cef_string_t resources_dir_path;

  ///
  // The fully qualified path for the locales directory. If this value is empty
  // the locales directory must be located in the module directory. This value
  // is ignored on Mac OS X where pack files are always loaded from the app
  // bundle Resources directory. Also configurable using the "locales-dir-path"
  // command-line switch.
  ///
  cef_string_t locales_dir_path;

  ///
  // Set to true (1) to disable loading of pack files for resources and locales.
  // A resource bundle handler must be provided for the browser and render
  // processes via CefApp::GetResourceBundleHandler() if loading of pack files
  // is disabled. Also configurable using the "disable-pack-loading" command-
  // line switch.
  ///
  int pack_loading_disabled;

  ///
  // Set to a value between 1024 and 65535 to enable remote debugging on the
  // specified port. For example, if 8080 is specified the remote debugging URL
  // will be http://localhost:8080. CEF can be remotely debugged from any CEF or
  // Chrome browser window. Also configurable using the "remote-debugging-port"
  // command-line switch.
  ///
  int remote_debugging_port;

  ///
  // The number of stack trace frames to capture for uncaught exceptions.
  // Specify a positive value to enable the CefRenderProcessHandler::
  // OnUncaughtException() callback. Specify 0 (default value) and
  // OnUncaughtException() will not be called. Also configurable using the
  // "uncaught-exception-stack-size" command-line switch.
  ///
  int uncaught_exception_stack_size;

  ///
  // By default CEF V8 references will be invalidated (the IsValid() method will
  // return false) after the owning context has been released. This reduces the
  // need for external record keeping and avoids crashes due to the use of V8
  // references after the associated context has been released.
  //
  // CEF currently offers two context safety implementations with different
  // performance characteristics. The default implementation (value of 0) uses a
  // map of hash values and should provide better performance in situations with
  // a small number contexts. The alternate implementation (value of 1) uses a
  // hidden value attached to each context and should provide better performance
  // in situations with a large number of contexts.
  //
  // If you need better performance in the creation of V8 references and you
  // plan to manually track context lifespan you can disable context safety by
  // specifying a value of -1.
  //
  // Also configurable using the "context-safety-implementation" command-line
  // switch.
  ///
  int context_safety_implementation;

  ///
  // Set to true (1) to ignore errors related to invalid SSL certificates.
  // Enabling this setting can lead to potential security vulnerabilities like
  // "man in the middle" attacks. Applications that load content from the
  // internet should not enable this setting. Also configurable using the
  // "ignore-certificate-errors" command-line switch. Can be overridden for
  // individual CefRequestContext instances via the
  // CefRequestContextSettings.ignore_certificate_errors value.
  ///
  int ignore_certificate_errors;

  ///
  // Opaque background color used for accelerated content. By default the
  // background color will be white. Only the RGB compontents of the specified
  // value will be used. The alpha component must greater than 0 to enable use
  // of the background color but will be otherwise ignored.
  ///
  cef_color_t background_color;

  ///
  // Comma delimited ordered list of language codes without any whitespace that
  // will be used in the "Accept-Language" HTTP header. May be overridden on a
  // per-browser basis using the CefBrowserSettings.accept_language_list value.
  // If both values are empty then "en-US,en" will be used. Can be overridden
  // for individual CefRequestContext instances via the
  // CefRequestContextSettings.accept_language_list value.
  ///
  cef_string_t accept_language_list;
} cef_settings_t;

///
// Request context initialization settings. Specify NULL or 0 to get the
// recommended default values.
///
typedef struct _cef_request_context_settings_t {
  ///
  // Size of this structure.
  ///
  size_t size;

  ///
  // The location where cache data will be stored on disk. If empty then
  // browsers will be created in "incognito mode" where in-memory caches are
  // used for storage and no data is persisted to disk. HTML5 databases such as
  // localStorage will only persist across sessions if a cache path is
  // specified. To share the global browser cache and related configuration set
  // this value to match the CefSettings.cache_path value.
  ///
  cef_string_t cache_path;

  ///
  // To persist session cookies (cookies without an expiry date or validity
  // interval) by default when using the global cookie manager set this value to
  // true (1). Session cookies are generally intended to be transient and most
  // Web browsers do not persist them. Can be set globally using the
  // CefSettings.persist_session_cookies value. This value will be ignored if
  // |cache_path| is empty or if it matches the CefSettings.cache_path value.
  ///
  int persist_session_cookies;

  ///
  // To persist user preferences as a JSON file in the cache path directory set
  // this value to true (1). Can be set globally using the
  // CefSettings.persist_user_preferences value. This value will be ignored if
  // |cache_path| is empty or if it matches the CefSettings.cache_path value.
  ///
  int persist_user_preferences;

  ///
  // Set to true (1) to ignore errors related to invalid SSL certificates.
  // Enabling this setting can lead to potential security vulnerabilities like
  // "man in the middle" attacks. Applications that load content from the
  // internet should not enable this setting. Can be set globally using the
  // CefSettings.ignore_certificate_errors value. This value will be ignored if
  // |cache_path| matches the CefSettings.cache_path value.
  ///
  int ignore_certificate_errors;

  ///
  // Comma delimited ordered list of language codes without any whitespace that
  // will be used in the "Accept-Language" HTTP header. Can be set globally
  // using the CefSettings.accept_language_list value or overridden on a per-
  // browser basis using the CefBrowserSettings.accept_language_list value. If
  // all values are empty then "en-US,en" will be used. This value will be
  // ignored if |cache_path| matches the CefSettings.cache_path value.
  ///
  cef_string_t accept_language_list;
} cef_request_context_settings_t;

///
// Browser initialization settings. Specify NULL or 0 to get the recommended
// default values. The consequences of using custom values may not be well
// tested. Many of these and other settings can also configured using command-
// line switches.
///
typedef struct _cef_browser_settings_t {
  ///
  // Size of this structure.
  ///
  size_t size;

  ///
  // The maximum rate in frames per second (fps) that CefRenderHandler::OnPaint
  // will be called for a windowless browser. The actual fps may be lower if
  // the browser cannot generate frames at the requested rate. The minimum
  // value is 1 and the maximum value is 60 (default 30). This value can also be
  // changed dynamically via CefBrowserHost::SetWindowlessFrameRate.
  ///
  int windowless_frame_rate;

  // The below values map to WebPreferences settings.

  ///
  // Font settings.
  ///
  cef_string_t standard_font_family;
  cef_string_t fixed_font_family;
  cef_string_t serif_font_family;
  cef_string_t sans_serif_font_family;
  cef_string_t cursive_font_family;
  cef_string_t fantasy_font_family;
  int default_font_size;
  int default_fixed_font_size;
  int minimum_font_size;
  int minimum_logical_font_size;

  ///
  // Default encoding for Web content. If empty "ISO-8859-1" will be used. Also
  // configurable using the "default-encoding" command-line switch.
  ///
  cef_string_t default_encoding;

  ///
  // Controls the loading of fonts from remote sources. Also configurable using
  // the "disable-remote-fonts" command-line switch.
  ///
  cef_state_t remote_fonts;

  ///
  // Controls whether JavaScript can be executed. Also configurable using the
  // "disable-javascript" command-line switch.
  ///
  cef_state_t javascript;

  ///
  // Controls whether JavaScript can be used for opening windows. Also
  // configurable using the "disable-javascript-open-windows" command-line
  // switch.
  ///
  cef_state_t javascript_open_windows;

  ///
  // Controls whether JavaScript can be used to close windows that were not
  // opened via JavaScript. JavaScript can still be used to close windows that
  // were opened via JavaScript or that have no back/forward history. Also
  // configurable using the "disable-javascript-close-windows" command-line
  // switch.
  ///
  cef_state_t javascript_close_windows;

  ///
  // Controls whether JavaScript can access the clipboard. Also configurable
  // using the "disable-javascript-access-clipboard" command-line switch.
  ///
  cef_state_t javascript_access_clipboard;

  ///
  // Controls whether DOM pasting is supported in the editor via
  // execCommand("paste"). The |javascript_access_clipboard| setting must also
  // be enabled. Also configurable using the "disable-javascript-dom-paste"
  // command-line switch.
  ///
  cef_state_t javascript_dom_paste;

  ///
  // Controls whether the caret position will be drawn. Also configurable using
  // the "enable-caret-browsing" command-line switch.
  ///
  cef_state_t caret_browsing;

  ///
  // Controls whether any plugins will be loaded. Also configurable using the
  // "disable-plugins" command-line switch.
  ///
  cef_state_t plugins;

  ///
  // Controls whether file URLs will have access to all URLs. Also configurable
  // using the "allow-universal-access-from-files" command-line switch.
  ///
  cef_state_t universal_access_from_file_urls;

  ///
  // Controls whether file URLs will have access to other file URLs. Also
  // configurable using the "allow-access-from-files" command-line switch.
  ///
  cef_state_t file_access_from_file_urls;

  ///
  // Controls whether web security restrictions (same-origin policy) will be
  // enforced. Disabling this setting is not recommend as it will allow risky
  // security behavior such as cross-site scripting (XSS). Also configurable
  // using the "disable-web-security" command-line switch.
  ///
  cef_state_t web_security;

  ///
  // Controls whether image URLs will be loaded from the network. A cached image
  // will still be rendered if requested. Also configurable using the
  // "disable-image-loading" command-line switch.
  ///
  cef_state_t image_loading;

  ///
  // Controls whether standalone images will be shrunk to fit the page. Also
  // configurable using the "image-shrink-standalone-to-fit" command-line
  // switch.
  ///
  cef_state_t image_shrink_standalone_to_fit;

  ///
  // Controls whether text areas can be resized. Also configurable using the
  // "disable-text-area-resize" command-line switch.
  ///
  cef_state_t text_area_resize;

  ///
  // Controls whether the tab key can advance focus to links. Also configurable
  // using the "disable-tab-to-links" command-line switch.
  ///
  cef_state_t tab_to_links;

  ///
  // Controls whether local storage can be used. Also configurable using the
  // "disable-local-storage" command-line switch.
  ///
  cef_state_t local_storage;

  ///
  // Controls whether databases can be used. Also configurable using the
  // "disable-databases" command-line switch.
  ///
  cef_state_t databases;

  ///
  // Controls whether the application cache can be used. Also configurable using
  // the "disable-application-cache" command-line switch.
  ///
  cef_state_t application_cache;

  ///
  // Controls whether WebGL can be used. Note that WebGL requires hardware
  // support and may not work on all systems even when enabled. Also
  // configurable using the "disable-webgl" command-line switch.
  ///
  cef_state_t webgl;

  ///
  // Opaque background color used for the browser before a document is loaded
  // and when no document color is specified. By default the background color
  // will be the same as CefSettings.background_color. Only the RGB compontents
  // of the specified value will be used. The alpha component must greater than
  // 0 to enable use of the background color but will be otherwise ignored.
  ///
  cef_color_t background_color;

  ///
  // Comma delimited ordered list of language codes without any whitespace that
  // will be used in the "Accept-Language" HTTP header. May be set globally
  // using the CefBrowserSettings.accept_language_list value. If both values are
  // empty then "en-US,en" will be used.
  ///
  cef_string_t accept_language_list;
} cef_browser_settings_t;

///
// Return value types.
///
typedef enum {
  ///
  // Cancel immediately.
  ///
  RV_CANCEL = 0,

  ///
  // Continue immediately.
  ///
  RV_CONTINUE,

  ///
  // Continue asynchronously (usually via a callback).
  ///
  RV_CONTINUE_ASYNC,
} cef_return_value_t;

///
// URL component parts.
///
typedef struct _cef_urlparts_t {
  ///
  // The complete URL specification.
  ///
  cef_string_t spec;

  ///
  // Scheme component not including the colon (e.g., "http").
  ///
  cef_string_t scheme;

  ///
  // User name component.
  ///
  cef_string_t username;

  ///
  // Password component.
  ///
  cef_string_t password;

  ///
  // Host component. This may be a hostname, an IPv4 address or an IPv6 literal
  // surrounded by square brackets (e.g., "[2001:db8::1]").
  ///
  cef_string_t host;

  ///
  // Port number component.
  ///
  cef_string_t port;

  ///
  // Origin contains just the scheme, host, and port from a URL. Equivalent to
  // clearing any username and password, replacing the path with a slash, and
  // clearing everything after that. This value will be empty for non-standard
  // URLs.
  ///
  cef_string_t origin;

  ///
  // Path component including the first slash following the host.
  ///
  cef_string_t path;

  ///
  // Query string component (i.e., everything following the '?').
  ///
  cef_string_t query;
} cef_urlparts_t;

///
// Cookie information.
///
typedef struct _cef_cookie_t {
  ///
  // The cookie name.
  ///
  cef_string_t name;

  ///
  // The cookie value.
  ///
  cef_string_t value;

  ///
  // If |domain| is empty a host cookie will be created instead of a domain
  // cookie. Domain cookies are stored with a leading "." and are visible to
  // sub-domains whereas host cookies are not.
  ///
  cef_string_t domain;

  ///
  // If |path| is non-empty only URLs at or below the path will get the cookie
  // value.
  ///
  cef_string_t path;

  ///
  // If |secure| is true the cookie will only be sent for HTTPS requests.
  ///
  int secure;

  ///
  // If |httponly| is true the cookie will only be sent for HTTP requests.
  ///
  int httponly;

  ///
  // The cookie creation date. This is automatically populated by the system on
  // cookie creation.
  ///
  cef_time_t creation;

  ///
  // The cookie last access date. This is automatically populated by the system
  // on access.
  ///
  cef_time_t last_access;

  ///
  // The cookie expiration date is only valid if |has_expires| is true.
  ///
  int has_expires;
  cef_time_t expires;
} cef_cookie_t;

///
// Process termination status values.
///
typedef enum {
  ///
  // Non-zero exit status.
  ///
  TS_ABNORMAL_TERMINATION,

  ///
  // SIGKILL or task manager kill.
  ///
  TS_PROCESS_WAS_KILLED,

  ///
  // Segmentation fault.
  ///
  TS_PROCESS_CRASHED,
} cef_termination_status_t;

///
// Path key values.
///
typedef enum {
  ///
  // Current directory.
  ///
  PK_DIR_CURRENT,

  ///
  // Directory containing PK_FILE_EXE.
  ///
  PK_DIR_EXE,

  ///
  // Directory containing PK_FILE_MODULE.
  ///
  PK_DIR_MODULE,

  ///
  // Temporary directory.
  ///
  PK_DIR_TEMP,

  ///
  // Path and filename of the current executable.
  ///
  PK_FILE_EXE,

  ///
  // Path and filename of the module containing the CEF code (usually the libcef
  // module).
  ///
  PK_FILE_MODULE,

  ///
  // "Local Settings\Application Data" directory under the user profile
  // directory on Windows.
  ///
  PK_LOCAL_APP_DATA,

  ///
  // "Application Data" directory under the user profile directory on Windows
  // and "~/Library/Application Support" directory on Mac OS X.
  ///
  PK_USER_DATA,
} cef_path_key_t;

///
// Storage types.
///
typedef enum {
  ST_LOCALSTORAGE = 0,
  ST_SESSIONSTORAGE,
} cef_storage_type_t;

///
// Supported error code values. See net\base\net_error_list.h for complete
// descriptions of the error codes.
///
typedef enum {
  ERR_NONE = 0,
  ERR_FAILED = -2,
  ERR_ABORTED = -3,
  ERR_INVALID_ARGUMENT = -4,
  ERR_INVALID_HANDLE = -5,
  ERR_FILE_NOT_FOUND = -6,
  ERR_TIMED_OUT = -7,
  ERR_FILE_TOO_BIG = -8,
  ERR_UNEXPECTED = -9,
  ERR_ACCESS_DENIED = -10,
  ERR_NOT_IMPLEMENTED = -11,
  ERR_CONNECTION_CLOSED = -100,
  ERR_CONNECTION_RESET = -101,
  ERR_CONNECTION_REFUSED = -102,
  ERR_CONNECTION_ABORTED = -103,
  ERR_CONNECTION_FAILED = -104,
  ERR_NAME_NOT_RESOLVED = -105,
  ERR_INTERNET_DISCONNECTED = -106,
  ERR_SSL_PROTOCOL_ERROR = -107,
  ERR_ADDRESS_INVALID = -108,
  ERR_ADDRESS_UNREACHABLE = -109,
  ERR_SSL_CLIENT_AUTH_CERT_NEEDED = -110,
  ERR_TUNNEL_CONNECTION_FAILED = -111,
  ERR_NO_SSL_VERSIONS_ENABLED = -112,
  ERR_SSL_VERSION_OR_CIPHER_MISMATCH = -113,
  ERR_SSL_RENEGOTIATION_REQUESTED = -114,
  ERR_CERT_COMMON_NAME_INVALID = -200,
  ERR_CERT_BEGIN = ERR_CERT_COMMON_NAME_INVALID,
  ERR_CERT_DATE_INVALID = -201,
  ERR_CERT_AUTHORITY_INVALID = -202,
  ERR_CERT_CONTAINS_ERRORS = -203,
  ERR_CERT_NO_REVOCATION_MECHANISM = -204,
  ERR_CERT_UNABLE_TO_CHECK_REVOCATION = -205,
  ERR_CERT_REVOKED = -206,
  ERR_CERT_INVALID = -207,
  ERR_CERT_WEAK_SIGNATURE_ALGORITHM = -208,
  // -209 is available: was ERR_CERT_NOT_IN_DNS.
  ERR_CERT_NON_UNIQUE_NAME = -210,
  ERR_CERT_WEAK_KEY = -211,
  ERR_CERT_NAME_CONSTRAINT_VIOLATION = -212,
  ERR_CERT_VALIDITY_TOO_LONG = -213,
  ERR_CERT_END = ERR_CERT_VALIDITY_TOO_LONG,
  ERR_INVALID_URL = -300,
  ERR_DISALLOWED_URL_SCHEME = -301,
  ERR_UNKNOWN_URL_SCHEME = -302,
  ERR_TOO_MANY_REDIRECTS = -310,
  ERR_UNSAFE_REDIRECT = -311,
  ERR_UNSAFE_PORT = -312,
  ERR_INVALID_RESPONSE = -320,
  ERR_INVALID_CHUNKED_ENCODING = -321,
  ERR_METHOD_NOT_SUPPORTED = -322,
  ERR_UNEXPECTED_PROXY_AUTH = -323,
  ERR_EMPTY_RESPONSE = -324,
  ERR_RESPONSE_HEADERS_TOO_BIG = -325,
  ERR_CACHE_MISS = -400,
  ERR_INSECURE_RESPONSE = -501,
} cef_errorcode_t;

///
// Supported certificate status code values. See net\cert\cert_status_flags.h
// for more information. CERT_STATUS_NONE is new in CEF because we use an
// enum while cert_status_flags.h uses a typedef and static const variables.
///
typedef enum {
  CERT_STATUS_NONE = 0,
  CERT_STATUS_COMMON_NAME_INVALID = 1 << 0,
  CERT_STATUS_DATE_INVALID = 1 << 1,
  CERT_STATUS_AUTHORITY_INVALID = 1 << 2,
  // 1 << 3 is reserved for ERR_CERT_CONTAINS_ERRORS (not useful with WinHTTP).
  CERT_STATUS_NO_REVOCATION_MECHANISM = 1 << 4,
  CERT_STATUS_UNABLE_TO_CHECK_REVOCATION = 1 << 5,
  CERT_STATUS_REVOKED = 1 << 6,
  CERT_STATUS_INVALID = 1 << 7,
  CERT_STATUS_WEAK_SIGNATURE_ALGORITHM = 1 << 8,
  // 1 << 9 was used for CERT_STATUS_NOT_IN_DNS
  CERT_STATUS_NON_UNIQUE_NAME = 1 << 10,
  CERT_STATUS_WEAK_KEY = 1 << 11,
  // 1 << 12 was used for CERT_STATUS_WEAK_DH_KEY
  CERT_STATUS_PINNED_KEY_MISSING = 1 << 13,
  CERT_STATUS_NAME_CONSTRAINT_VIOLATION = 1 << 14,
  CERT_STATUS_VALIDITY_TOO_LONG = 1 << 15,

  // Bits 16 to 31 are for non-error statuses.
  CERT_STATUS_IS_EV = 1 << 16,
  CERT_STATUS_REV_CHECKING_ENABLED = 1 << 17,
  // Bit 18 was CERT_STATUS_IS_DNSSEC
  CERT_STATUS_SHA1_SIGNATURE_PRESENT = 1 << 19,
  CERT_STATUS_CT_COMPLIANCE_FAILED = 1 << 20,
} cef_cert_status_t;

///
// The manner in which a link click should be opened. These constants match
// their equivalents in Chromium's window_open_disposition.h and should not be
// renumbered.
///
typedef enum {
  WOD_UNKNOWN,
  WOD_CURRENT_TAB,
  WOD_SINGLETON_TAB,
  WOD_NEW_FOREGROUND_TAB,
  WOD_NEW_BACKGROUND_TAB,
  WOD_NEW_POPUP,
  WOD_NEW_WINDOW,
  WOD_SAVE_TO_DISK,
  WOD_OFF_THE_RECORD,
  WOD_IGNORE_ACTION
} cef_window_open_disposition_t;

///
// "Verb" of a drag-and-drop operation as negotiated between the source and
// destination. These constants match their equivalents in WebCore's
// DragActions.h and should not be renumbered.
///
typedef enum {
    DRAG_OPERATION_NONE    = 0,
    DRAG_OPERATION_COPY    = 1,
    DRAG_OPERATION_LINK    = 2,
    DRAG_OPERATION_GENERIC = 4,
    DRAG_OPERATION_PRIVATE = 8,
    DRAG_OPERATION_MOVE    = 16,
    DRAG_OPERATION_DELETE  = 32,
    DRAG_OPERATION_EVERY   = UINT_MAX
} cef_drag_operations_mask_t;

///
// V8 access control values.
///
typedef enum {
  V8_ACCESS_CONTROL_DEFAULT               = 0,
  V8_ACCESS_CONTROL_ALL_CAN_READ          = 1,
  V8_ACCESS_CONTROL_ALL_CAN_WRITE         = 1 << 1,
  V8_ACCESS_CONTROL_PROHIBITS_OVERWRITING = 1 << 2
} cef_v8_accesscontrol_t;

///
// V8 property attribute values.
///
typedef enum {
  V8_PROPERTY_ATTRIBUTE_NONE       = 0,       // Writeable, Enumerable,
                                              //   Configurable
  V8_PROPERTY_ATTRIBUTE_READONLY   = 1 << 0,  // Not writeable
  V8_PROPERTY_ATTRIBUTE_DONTENUM   = 1 << 1,  // Not enumerable
  V8_PROPERTY_ATTRIBUTE_DONTDELETE = 1 << 2   // Not configurable
} cef_v8_propertyattribute_t;

///
// Post data elements may represent either bytes or files.
///
typedef enum {
  PDE_TYPE_EMPTY  = 0,
  PDE_TYPE_BYTES,
  PDE_TYPE_FILE,
} cef_postdataelement_type_t;

///
// Resource type for a request.
///
typedef enum {
  ///
  // Top level page.
  ///
  RT_MAIN_FRAME = 0,

  ///
  // Frame or iframe.
  ///
  RT_SUB_FRAME,

  ///
  // CSS stylesheet.
  ///
  RT_STYLESHEET,

  ///
  // External script.
  ///
  RT_SCRIPT,

  ///
  // Image (jpg/gif/png/etc).
  ///
  RT_IMAGE,

  ///
  // Font.
  ///
  RT_FONT_RESOURCE,

  ///
  // Some other subresource. This is the default type if the actual type is
  // unknown.
  ///
  RT_SUB_RESOURCE,

  ///
  // Object (or embed) tag for a plugin, or a resource that a plugin requested.
  ///
  RT_OBJECT,

  ///
  // Media resource.
  ///
  RT_MEDIA,

  ///
  // Main resource of a dedicated worker.
  ///
  RT_WORKER,

  ///
  // Main resource of a shared worker.
  ///
  RT_SHARED_WORKER,

  ///
  // Explicitly requested prefetch.
  ///
  RT_PREFETCH,

  ///
  // Favicon.
  ///
  RT_FAVICON,

  ///
  // XMLHttpRequest.
  ///
  RT_XHR,

  ///
  // A request for a <ping>
  ///
  RT_PING,

  ///
  // Main resource of a service worker.
  ///
  RT_SERVICE_WORKER,

  ///
  // A report of Content Security Policy violations.
  ///
  RT_CSP_REPORT,

  ///
  // A resource that a plugin requested.
  ///
  RT_PLUGIN_RESOURCE,
} cef_resource_type_t;

///
// Transition type for a request. Made up of one source value and 0 or more
// qualifiers.
///
typedef enum {
  ///
  // Source is a link click or the JavaScript window.open function. This is
  // also the default value for requests like sub-resource loads that are not
  // navigations.
  ///
  TT_LINK = 0,

  ///
  // Source is some other "explicit" navigation action such as creating a new
  // browser or using the LoadURL function. This is also the default value
  // for navigations where the actual type is unknown.
  ///
  TT_EXPLICIT = 1,

  ///
  // Source is a subframe navigation. This is any content that is automatically
  // loaded in a non-toplevel frame. For example, if a page consists of several
  // frames containing ads, those ad URLs will have this transition type.
  // The user may not even realize the content in these pages is a separate
  // frame, so may not care about the URL.
  ///
  TT_AUTO_SUBFRAME = 3,

  ///
  // Source is a subframe navigation explicitly requested by the user that will
  // generate new navigation entries in the back/forward list. These are
  // probably more important than frames that were automatically loaded in
  // the background because the user probably cares about the fact that this
  // link was loaded.
  ///
  TT_MANUAL_SUBFRAME = 4,

  ///
  // Source is a form submission by the user. NOTE: In some situations
  // submitting a form does not result in this transition type. This can happen
  // if the form uses a script to submit the contents.
  ///
  TT_FORM_SUBMIT = 7,

  ///
  // Source is a "reload" of the page via the Reload function or by re-visiting
  // the same URL. NOTE: This is distinct from the concept of whether a
  // particular load uses "reload semantics" (i.e. bypasses cached data).
  ///
  TT_RELOAD = 8,

  ///
  // General mask defining the bits used for the source values.
  ///
  TT_SOURCE_MASK = 0xFF,

  // Qualifiers.
  // Any of the core values above can be augmented by one or more qualifiers.
  // These qualifiers further define the transition.

  ///
  // Attempted to visit a URL but was blocked.
  ///
  TT_BLOCKED_FLAG = 0x00800000,

  ///
  // Used the Forward or Back function to navigate among browsing history.
  ///
  TT_FORWARD_BACK_FLAG = 0x01000000,

  ///
  // The beginning of a navigation chain.
  ///
  TT_CHAIN_START_FLAG = 0x10000000,

  ///
  // The last transition in a redirect chain.
  ///
  TT_CHAIN_END_FLAG = 0x20000000,

  ///
  // Redirects caused by JavaScript or a meta refresh tag on the page.
  ///
  TT_CLIENT_REDIRECT_FLAG = 0x40000000,

  ///
  // Redirects sent from the server by HTTP headers.
  ///
  TT_SERVER_REDIRECT_FLAG = 0x80000000,

  ///
  // Used to test whether a transition involves a redirect.
  ///
  TT_IS_REDIRECT_MASK = 0xC0000000,

  ///
  // General mask defining the bits used for the qualifiers.
  ///
  TT_QUALIFIER_MASK = 0xFFFFFF00,
} cef_transition_type_t;

///
// Flags used to customize the behavior of CefURLRequest.
///
typedef enum {
  ///
  // Default behavior.
  ///
  UR_FLAG_NONE                      = 0,

  ///
  // If set the cache will be skipped when handling the request.
  ///
  UR_FLAG_SKIP_CACHE                = 1 << 0,

  ///
  // If set user name, password, and cookies may be sent with the request, and
  // cookies may be saved from the response.
  ///
  UR_FLAG_ALLOW_CACHED_CREDENTIALS  = 1 << 1,

  ///
  // If set upload progress events will be generated when a request has a body.
  ///
  UR_FLAG_REPORT_UPLOAD_PROGRESS    = 1 << 3,

  ///
  // If set the CefURLRequestClient::OnDownloadData method will not be called.
  ///
  UR_FLAG_NO_DOWNLOAD_DATA          = 1 << 6,

  ///
  // If set 5XX redirect errors will be propagated to the observer instead of
  // automatically re-tried. This currently only applies for requests
  // originated in the browser process.
  ///
  UR_FLAG_NO_RETRY_ON_5XX           = 1 << 7,
} cef_urlrequest_flags_t;

///
// Flags that represent CefURLRequest status.
///
typedef enum {
  ///
  // Unknown status.
  ///
  UR_UNKNOWN = 0,

  ///
  // Request succeeded.
  ///
  UR_SUCCESS,

  ///
  // An IO request is pending, and the caller will be informed when it is
  // completed.
  ///
  UR_IO_PENDING,

  ///
  // Request was canceled programatically.
  ///
  UR_CANCELED,

  ///
  // Request failed for some reason.
  ///
  UR_FAILED,
} cef_urlrequest_status_t;

///
// Structure representing a point.
///
typedef struct _cef_point_t {
  int x;
  int y;
} cef_point_t;

///
// Structure representing a rectangle.
///
typedef struct _cef_rect_t {
  int x;
  int y;
  int width;
  int height;
} cef_rect_t;

///
// Structure representing a size.
///
typedef struct _cef_size_t {
  int width;
  int height;
} cef_size_t;

///
// Structure representing a range.
///
typedef struct _cef_range_t {
  int from;
  int to;
} cef_range_t;

///
// Structure representing insets.
///
typedef struct _cef_insets_t {
  int top;
  int left;
  int bottom;
  int right;
} cef_insets_t;

///
// Structure representing a draggable region.
///
typedef struct _cef_draggable_region_t {
  ///
  // Bounds of the region.
  ///
  cef_rect_t bounds;

  ///
  // True (1) this this region is draggable and false (0) otherwise.
  ///
  int draggable;
} cef_draggable_region_t;

///
// Existing process IDs.
///
typedef enum {
  ///
  // Browser process.
  ///
  PID_BROWSER,
  ///
  // Renderer process.
  ///
  PID_RENDERER,
} cef_process_id_t;

///
// Existing thread IDs.
///
typedef enum {
// BROWSER PROCESS THREADS -- Only available in the browser process.

  ///
  // The main thread in the browser. This will be the same as the main
  // application thread if CefInitialize() is called with a
  // CefSettings.multi_threaded_message_loop value of false.
  ///
  TID_UI,

  ///
  // Used to interact with the database.
  ///
  TID_DB,

  ///
  // Used to interact with the file system.
  ///
  TID_FILE,

  ///
  // Used for file system operations that block user interactions.
  // Responsiveness of this thread affects users.
  ///
  TID_FILE_USER_BLOCKING,

  ///
  // Used to launch and terminate browser processes.
  ///
  TID_PROCESS_LAUNCHER,

  ///
  // Used to handle slow HTTP cache operations.
  ///
  TID_CACHE,

  ///
  // Used to process IPC and network messages.
  ///
  TID_IO,

// RENDER PROCESS THREADS -- Only available in the render process.

  ///
  // The main thread in the renderer. Used for all WebKit and V8 interaction.
  ///
  TID_RENDERER,
} cef_thread_id_t;

///
// Supported value types.
///
typedef enum {
  VTYPE_INVALID = 0,
  VTYPE_NULL,
  VTYPE_BOOL,
  VTYPE_INT,
  VTYPE_DOUBLE,
  VTYPE_STRING,
  VTYPE_BINARY,
  VTYPE_DICTIONARY,
  VTYPE_LIST,
} cef_value_type_t;

///
// Supported JavaScript dialog types.
///
typedef enum {
  JSDIALOGTYPE_ALERT = 0,
  JSDIALOGTYPE_CONFIRM,
  JSDIALOGTYPE_PROMPT,
} cef_jsdialog_type_t;

///
// Screen information used when window rendering is disabled. This structure is
// passed as a parameter to CefRenderHandler::GetScreenInfo and should be filled
// in by the client.
///
typedef struct _cef_screen_info_t {
  ///
  // Device scale factor. Specifies the ratio between physical and logical
  // pixels.
  ///
  float device_scale_factor;

  ///
  // The screen depth in bits per pixel.
  ///
  int depth;

  ///
  // The bits per color component. This assumes that the colors are balanced
  // equally.
  ///
  int depth_per_component;

  ///
  // This can be true for black and white printers.
  ///
  int is_monochrome;

  ///
  // This is set from the rcMonitor member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the display monitor rectangle,
  //   expressed in virtual-screen coordinates. Note that if the monitor
  //   is not the primary display monitor, some of the rectangle's
  //   coordinates may be negative values."
  //
  // The |rect| and |available_rect| properties are used to determine the
  // available surface for rendering popup views.
  ///
  cef_rect_t rect;

  ///
  // This is set from the rcWork member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the work area rectangle of the
  //   display monitor that can be used by applications, expressed in
  //   virtual-screen coordinates. Windows uses this rectangle to
  //   maximize an application on the monitor. The rest of the area in
  //   rcMonitor contains system windows such as the task bar and side
  //   bars. Note that if the monitor is not the primary display monitor,
  //   some of the rectangle's coordinates may be negative values".
  //
  // The |rect| and |available_rect| properties are used to determine the
  // available surface for rendering popup views.
  ///
  cef_rect_t available_rect;
} cef_screen_info_t;

///
// Supported menu IDs. Non-English translations can be provided for the
// IDS_MENU_* strings in CefResourceBundleHandler::GetLocalizedString().
///
typedef enum {
  // Navigation.
  MENU_ID_BACK                = 100,
  MENU_ID_FORWARD             = 101,
  MENU_ID_RELOAD              = 102,
  MENU_ID_RELOAD_NOCACHE      = 103,
  MENU_ID_STOPLOAD            = 104,

  // Editing.
  MENU_ID_UNDO                = 110,
  MENU_ID_REDO                = 111,
  MENU_ID_CUT                 = 112,
  MENU_ID_COPY                = 113,
  MENU_ID_PASTE               = 114,
  MENU_ID_DELETE              = 115,
  MENU_ID_SELECT_ALL          = 116,

  // Miscellaneous.
  MENU_ID_FIND                = 130,
  MENU_ID_PRINT               = 131,
  MENU_ID_VIEW_SOURCE         = 132,

  // Spell checking word correction suggestions.
  MENU_ID_SPELLCHECK_SUGGESTION_0        = 200,
  MENU_ID_SPELLCHECK_SUGGESTION_1        = 201,
  MENU_ID_SPELLCHECK_SUGGESTION_2        = 202,
  MENU_ID_SPELLCHECK_SUGGESTION_3        = 203,
  MENU_ID_SPELLCHECK_SUGGESTION_4        = 204,
  MENU_ID_SPELLCHECK_SUGGESTION_LAST     = 204,
  MENU_ID_NO_SPELLING_SUGGESTIONS        = 205,
  MENU_ID_ADD_TO_DICTIONARY              = 206,

  // Custom menu items originating from the renderer process. For example,
  // plugin placeholder menu items or Flash menu items.
  MENU_ID_CUSTOM_FIRST        = 220,
  MENU_ID_CUSTOM_LAST         = 250,

  // All user-defined menu IDs should come between MENU_ID_USER_FIRST and
  // MENU_ID_USER_LAST to avoid overlapping the Chromium and CEF ID ranges
  // defined in the tools/gritsettings/resource_ids file.
  MENU_ID_USER_FIRST          = 26500,
  MENU_ID_USER_LAST           = 28500,
} cef_menu_id_t;

///
// Mouse button types.
///
typedef enum {
  MBT_LEFT   = 0,
  MBT_MIDDLE,
  MBT_RIGHT,
} cef_mouse_button_type_t;

///
// Structure representing mouse event information.
///
typedef struct _cef_mouse_event_t {
  ///
  // X coordinate relative to the left side of the view.
  ///
  int x;

  ///
  // Y coordinate relative to the top side of the view.
  ///
  int y;

  ///
  // Bit flags describing any pressed modifier keys. See
  // cef_event_flags_t for values.
  ///
  uint32 modifiers;
} cef_mouse_event_t;

///
// Paint element types.
///
typedef enum {
  PET_VIEW  = 0,
  PET_POPUP,
} cef_paint_element_type_t;

///
// Supported event bit flags.
///
typedef enum {
  EVENTFLAG_NONE                = 0,
  EVENTFLAG_CAPS_LOCK_ON        = 1 << 0,
  EVENTFLAG_SHIFT_DOWN          = 1 << 1,
  EVENTFLAG_CONTROL_DOWN        = 1 << 2,
  EVENTFLAG_ALT_DOWN            = 1 << 3,
  EVENTFLAG_LEFT_MOUSE_BUTTON   = 1 << 4,
  EVENTFLAG_MIDDLE_MOUSE_BUTTON = 1 << 5,
  EVENTFLAG_RIGHT_MOUSE_BUTTON  = 1 << 6,
  // Mac OS-X command key.
  EVENTFLAG_COMMAND_DOWN        = 1 << 7,
  EVENTFLAG_NUM_LOCK_ON         = 1 << 8,
  EVENTFLAG_IS_KEY_PAD          = 1 << 9,
  EVENTFLAG_IS_LEFT             = 1 << 10,
  EVENTFLAG_IS_RIGHT            = 1 << 11,
} cef_event_flags_t;

///
// Supported menu item types.
///
typedef enum {
  MENUITEMTYPE_NONE,
  MENUITEMTYPE_COMMAND,
  MENUITEMTYPE_CHECK,
  MENUITEMTYPE_RADIO,
  MENUITEMTYPE_SEPARATOR,
  MENUITEMTYPE_SUBMENU,
} cef_menu_item_type_t;

///
// Supported context menu type flags.
///
typedef enum {
  ///
  // No node is selected.
  ///
  CM_TYPEFLAG_NONE        = 0,
  ///
  // The top page is selected.
  ///
  CM_TYPEFLAG_PAGE        = 1 << 0,
  ///
  // A subframe page is selected.
  ///
  CM_TYPEFLAG_FRAME       = 1 << 1,
  ///
  // A link is selected.
  ///
  CM_TYPEFLAG_LINK        = 1 << 2,
  ///
  // A media node is selected.
  ///
  CM_TYPEFLAG_MEDIA       = 1 << 3,
  ///
  // There is a textual or mixed selection that is selected.
  ///
  CM_TYPEFLAG_SELECTION   = 1 << 4,
  ///
  // An editable element is selected.
  ///
  CM_TYPEFLAG_EDITABLE    = 1 << 5,
} cef_context_menu_type_flags_t;

///
// Supported context menu media types.
///
typedef enum {
  ///
  // No special node is in context.
  ///
  CM_MEDIATYPE_NONE,
  ///
  // An image node is selected.
  ///
  CM_MEDIATYPE_IMAGE,
  ///
  // A video node is selected.
  ///
  CM_MEDIATYPE_VIDEO,
  ///
  // An audio node is selected.
  ///
  CM_MEDIATYPE_AUDIO,
  ///
  // A file node is selected.
  ///
  CM_MEDIATYPE_FILE,
  ///
  // A plugin node is selected.
  ///
  CM_MEDIATYPE_PLUGIN,
} cef_context_menu_media_type_t;

///
// Supported context menu media state bit flags.
///
typedef enum {
  CM_MEDIAFLAG_NONE                  = 0,
  CM_MEDIAFLAG_ERROR                 = 1 << 0,
  CM_MEDIAFLAG_PAUSED                = 1 << 1,
  CM_MEDIAFLAG_MUTED                 = 1 << 2,
  CM_MEDIAFLAG_LOOP                  = 1 << 3,
  CM_MEDIAFLAG_CAN_SAVE              = 1 << 4,
  CM_MEDIAFLAG_HAS_AUDIO             = 1 << 5,
  CM_MEDIAFLAG_HAS_VIDEO             = 1 << 6,
  CM_MEDIAFLAG_CONTROL_ROOT_ELEMENT  = 1 << 7,
  CM_MEDIAFLAG_CAN_PRINT             = 1 << 8,
  CM_MEDIAFLAG_CAN_ROTATE            = 1 << 9,
} cef_context_menu_media_state_flags_t;

///
// Supported context menu edit state bit flags.
///
typedef enum {
  CM_EDITFLAG_NONE            = 0,
  CM_EDITFLAG_CAN_UNDO        = 1 << 0,
  CM_EDITFLAG_CAN_REDO        = 1 << 1,
  CM_EDITFLAG_CAN_CUT         = 1 << 2,
  CM_EDITFLAG_CAN_COPY        = 1 << 3,
  CM_EDITFLAG_CAN_PASTE       = 1 << 4,
  CM_EDITFLAG_CAN_DELETE      = 1 << 5,
  CM_EDITFLAG_CAN_SELECT_ALL  = 1 << 6,
  CM_EDITFLAG_CAN_TRANSLATE   = 1 << 7,
} cef_context_menu_edit_state_flags_t;

///
// Key event types.
///
typedef enum {
  ///
  // Notification that a key transitioned from "up" to "down".
  ///
  KEYEVENT_RAWKEYDOWN = 0,

  ///
  // Notification that a key was pressed. This does not necessarily correspond
  // to a character depending on the key and language. Use KEYEVENT_CHAR for
  // character input.
  ///
  KEYEVENT_KEYDOWN,

  ///
  // Notification that a key was released.
  ///
  KEYEVENT_KEYUP,

  ///
  // Notification that a character was typed. Use this for text input. Key
  // down events may generate 0, 1, or more than one character event depending
  // on the key, locale, and operating system.
  ///
  KEYEVENT_CHAR
} cef_key_event_type_t;

///
// Structure representing keyboard event information.
///
typedef struct _cef_key_event_t {
  ///
  // The type of keyboard event.
  ///
  cef_key_event_type_t type;

  ///
  // Bit flags describing any pressed modifier keys. See
  // cef_event_flags_t for values.
  ///
  uint32 modifiers;

  ///
  // The Windows key code for the key event. This value is used by the DOM
  // specification. Sometimes it comes directly from the event (i.e. on
  // Windows) and sometimes it's determined using a mapping function. See
  // WebCore/platform/chromium/KeyboardCodes.h for the list of values.
  ///
  int windows_key_code;

  ///
  // The actual key code genenerated by the platform.
  ///
  int native_key_code;

  ///
  // Indicates whether the event is considered a "system key" event (see
  // http://msdn.microsoft.com/en-us/library/ms646286(VS.85).aspx for details).
  // This value will always be false on non-Windows platforms.
  ///
  int is_system_key;

  ///
  // The character generated by the keystroke.
  ///
  char16 character;

  ///
  // Same as |character| but unmodified by any concurrently-held modifiers
  // (except shift). This is useful for working out shortcut keys.
  ///
  char16 unmodified_character;

  ///
  // True if the focus is currently on an editable field on the page. This is
  // useful for determining if standard key events should be intercepted.
  ///
  int focus_on_editable_field;
} cef_key_event_t;

///
// Focus sources.
///
typedef enum {
  ///
  // The source is explicit navigation via the API (LoadURL(), etc).
  ///
  FOCUS_SOURCE_NAVIGATION = 0,
  ///
  // The source is a system-generated focus event.
  ///
  FOCUS_SOURCE_SYSTEM,
} cef_focus_source_t;

///
// Navigation types.
///
typedef enum {
  NAVIGATION_LINK_CLICKED = 0,
  NAVIGATION_FORM_SUBMITTED,
  NAVIGATION_BACK_FORWARD,
  NAVIGATION_RELOAD,
  NAVIGATION_FORM_RESUBMITTED,
  NAVIGATION_OTHER,
} cef_navigation_type_t;

///
// Supported XML encoding types. The parser supports ASCII, ISO-8859-1, and
// UTF16 (LE and BE) by default. All other types must be translated to UTF8
// before being passed to the parser. If a BOM is detected and the correct
// decoder is available then that decoder will be used automatically.
///
typedef enum {
  XML_ENCODING_NONE = 0,
  XML_ENCODING_UTF8,
  XML_ENCODING_UTF16LE,
  XML_ENCODING_UTF16BE,
  XML_ENCODING_ASCII,
} cef_xml_encoding_type_t;

///
// XML node types.
///
typedef enum {
  XML_NODE_UNSUPPORTED = 0,
  XML_NODE_PROCESSING_INSTRUCTION,
  XML_NODE_DOCUMENT_TYPE,
  XML_NODE_ELEMENT_START,
  XML_NODE_ELEMENT_END,
  XML_NODE_ATTRIBUTE,
  XML_NODE_TEXT,
  XML_NODE_CDATA,
  XML_NODE_ENTITY_REFERENCE,
  XML_NODE_WHITESPACE,
  XML_NODE_COMMENT,
} cef_xml_node_type_t;

///
// Popup window features.
///
typedef struct _cef_popup_features_t {
  int x;
  int xSet;
  int y;
  int ySet;
  int width;
  int widthSet;
  int height;
  int heightSet;

  int menuBarVisible;
  int statusBarVisible;
  int toolBarVisible;
  int locationBarVisible;
  int scrollbarsVisible;
  int resizable;

  int fullscreen;
  int dialog;
  cef_string_list_t additionalFeatures;
} cef_popup_features_t;

///
// DOM document types.
///
typedef enum {
  DOM_DOCUMENT_TYPE_UNKNOWN = 0,
  DOM_DOCUMENT_TYPE_HTML,
  DOM_DOCUMENT_TYPE_XHTML,
  DOM_DOCUMENT_TYPE_PLUGIN,
} cef_dom_document_type_t;

///
// DOM event category flags.
///
typedef enum {
  DOM_EVENT_CATEGORY_UNKNOWN = 0x0,
  DOM_EVENT_CATEGORY_UI = 0x1,
  DOM_EVENT_CATEGORY_MOUSE = 0x2,
  DOM_EVENT_CATEGORY_MUTATION = 0x4,
  DOM_EVENT_CATEGORY_KEYBOARD = 0x8,
  DOM_EVENT_CATEGORY_TEXT = 0x10,
  DOM_EVENT_CATEGORY_COMPOSITION = 0x20,
  DOM_EVENT_CATEGORY_DRAG = 0x40,
  DOM_EVENT_CATEGORY_CLIPBOARD = 0x80,
  DOM_EVENT_CATEGORY_MESSAGE = 0x100,
  DOM_EVENT_CATEGORY_WHEEL = 0x200,
  DOM_EVENT_CATEGORY_BEFORE_TEXT_INSERTED = 0x400,
  DOM_EVENT_CATEGORY_OVERFLOW = 0x800,
  DOM_EVENT_CATEGORY_PAGE_TRANSITION = 0x1000,
  DOM_EVENT_CATEGORY_POPSTATE = 0x2000,
  DOM_EVENT_CATEGORY_PROGRESS = 0x4000,
  DOM_EVENT_CATEGORY_XMLHTTPREQUEST_PROGRESS = 0x8000,
} cef_dom_event_category_t;

///
// DOM event processing phases.
///
typedef enum {
  DOM_EVENT_PHASE_UNKNOWN = 0,
  DOM_EVENT_PHASE_CAPTURING,
  DOM_EVENT_PHASE_AT_TARGET,
  DOM_EVENT_PHASE_BUBBLING,
} cef_dom_event_phase_t;

///
// DOM node types.
///
typedef enum {
  DOM_NODE_TYPE_UNSUPPORTED = 0,
  DOM_NODE_TYPE_ELEMENT,
  DOM_NODE_TYPE_ATTRIBUTE,
  DOM_NODE_TYPE_TEXT,
  DOM_NODE_TYPE_CDATA_SECTION,
  DOM_NODE_TYPE_PROCESSING_INSTRUCTIONS,
  DOM_NODE_TYPE_COMMENT,
  DOM_NODE_TYPE_DOCUMENT,
  DOM_NODE_TYPE_DOCUMENT_TYPE,
  DOM_NODE_TYPE_DOCUMENT_FRAGMENT,
} cef_dom_node_type_t;

///
// Supported file dialog modes.
///
typedef enum {
  ///
  // Requires that the file exists before allowing the user to pick it.
  ///
  FILE_DIALOG_OPEN = 0,

  ///
  // Like Open, but allows picking multiple files to open.
  ///
  FILE_DIALOG_OPEN_MULTIPLE,

  ///
  // Like Open, but selects a folder to open.
  ///
  FILE_DIALOG_OPEN_FOLDER,

  ///
  // Allows picking a nonexistent file, and prompts to overwrite if the file
  // already exists.
  ///
  FILE_DIALOG_SAVE,

  ///
  // General mask defining the bits used for the type values.
  ///
  FILE_DIALOG_TYPE_MASK = 0xFF,

  // Qualifiers.
  // Any of the type values above can be augmented by one or more qualifiers.
  // These qualifiers further define the dialog behavior.

  ///
  // Prompt to overwrite if the user selects an existing file with the Save
  // dialog.
  ///
  FILE_DIALOG_OVERWRITEPROMPT_FLAG = 0x01000000,

  ///
  // Do not display read-only files.
  ///
  FILE_DIALOG_HIDEREADONLY_FLAG = 0x02000000,
} cef_file_dialog_mode_t;

///
// Geoposition error codes.
///
typedef enum {
  GEOPOSITON_ERROR_NONE = 0,
  GEOPOSITON_ERROR_PERMISSION_DENIED,
  GEOPOSITON_ERROR_POSITION_UNAVAILABLE,
  GEOPOSITON_ERROR_TIMEOUT,
} cef_geoposition_error_code_t;

///
// Structure representing geoposition information. The properties of this
// structure correspond to those of the JavaScript Position object although
// their types may differ.
///
typedef struct _cef_geoposition_t {
  ///
  // Latitude in decimal degrees north (WGS84 coordinate frame).
  ///
  double latitude;

  ///
  // Longitude in decimal degrees west (WGS84 coordinate frame).
  ///
  double longitude;

  ///
  // Altitude in meters (above WGS84 datum).
  ///
  double altitude;

  ///
  // Accuracy of horizontal position in meters.
  ///
  double accuracy;

  ///
  // Accuracy of altitude in meters.
  ///
  double altitude_accuracy;

  ///
  // Heading in decimal degrees clockwise from true north.
  ///
  double heading;

  ///
  // Horizontal component of device velocity in meters per second.
  ///
  double speed;

  ///
  // Time of position measurement in milliseconds since Epoch in UTC time. This
  // is taken from the host computer's system clock.
  ///
  cef_time_t timestamp;

  ///
  // Error code, see enum above.
  ///
  cef_geoposition_error_code_t error_code;

  ///
  // Human-readable error message.
  ///
  cef_string_t error_message;
} cef_geoposition_t;

///
// Print job color mode values.
///
typedef enum {
  COLOR_MODEL_UNKNOWN,
  COLOR_MODEL_GRAY,
  COLOR_MODEL_COLOR,
  COLOR_MODEL_CMYK,
  COLOR_MODEL_CMY,
  COLOR_MODEL_KCMY,
  COLOR_MODEL_CMY_K,  // CMY_K represents CMY+K.
  COLOR_MODEL_BLACK,
  COLOR_MODEL_GRAYSCALE,
  COLOR_MODEL_RGB,
  COLOR_MODEL_RGB16,
  COLOR_MODEL_RGBA,
  COLOR_MODEL_COLORMODE_COLOR,  // Used in samsung printer ppds.
  COLOR_MODEL_COLORMODE_MONOCHROME,  // Used in samsung printer ppds.
  COLOR_MODEL_HP_COLOR_COLOR,  // Used in HP color printer ppds.
  COLOR_MODEL_HP_COLOR_BLACK,  // Used in HP color printer ppds.
  COLOR_MODEL_PRINTOUTMODE_NORMAL,  // Used in foomatic ppds.
  COLOR_MODEL_PRINTOUTMODE_NORMAL_GRAY,  // Used in foomatic ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_CMYK,  // Used in canon printer ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_GREYSCALE,  // Used in canon printer ppds.
  COLOR_MODEL_PROCESSCOLORMODEL_RGB,  // Used in canon printer ppds
} cef_color_model_t;

///
// Print job duplex mode values.
///
typedef enum {
  DUPLEX_MODE_UNKNOWN = -1,
  DUPLEX_MODE_SIMPLEX,
  DUPLEX_MODE_LONG_EDGE,
  DUPLEX_MODE_SHORT_EDGE,
} cef_duplex_mode_t;

///
// Cursor type values.
///
typedef enum {
  CT_POINTER = 0,
  CT_CROSS,
  CT_HAND,
  CT_IBEAM,
  CT_WAIT,
  CT_HELP,
  CT_EASTRESIZE,
  CT_NORTHRESIZE,
  CT_NORTHEASTRESIZE,
  CT_NORTHWESTRESIZE,
  CT_SOUTHRESIZE,
  CT_SOUTHEASTRESIZE,
  CT_SOUTHWESTRESIZE,
  CT_WESTRESIZE,
  CT_NORTHSOUTHRESIZE,
  CT_EASTWESTRESIZE,
  CT_NORTHEASTSOUTHWESTRESIZE,
  CT_NORTHWESTSOUTHEASTRESIZE,
  CT_COLUMNRESIZE,
  CT_ROWRESIZE,
  CT_MIDDLEPANNING,
  CT_EASTPANNING,
  CT_NORTHPANNING,
  CT_NORTHEASTPANNING,
  CT_NORTHWESTPANNING,
  CT_SOUTHPANNING,
  CT_SOUTHEASTPANNING,
  CT_SOUTHWESTPANNING,
  CT_WESTPANNING,
  CT_MOVE,
  CT_VERTICALTEXT,
  CT_CELL,
  CT_CONTEXTMENU,
  CT_ALIAS,
  CT_PROGRESS,
  CT_NODROP,
  CT_COPY,
  CT_NONE,
  CT_NOTALLOWED,
  CT_ZOOMIN,
  CT_ZOOMOUT,
  CT_GRAB,
  CT_GRABBING,
  CT_CUSTOM,
} cef_cursor_type_t;

///
// Structure representing cursor information. |buffer| will be
// |size.width|*|size.height|*4 bytes in size and represents a BGRA image with
// an upper-left origin.
///
typedef struct _cef_cursor_info_t {
  cef_point_t hotspot;
  float image_scale_factor;
  void* buffer;
  cef_size_t size;
} cef_cursor_info_t;

///
// URI unescape rules passed to CefURIDecode().
///
typedef enum {
  ///
  // Don't unescape anything at all.
  ///
  UU_NONE = 0,

  ///
  // Don't unescape anything special, but all normal unescaping will happen.
  // This is a placeholder and can't be combined with other flags (since it's
  // just the absence of them). All other unescape rules imply "normal" in
  // addition to their special meaning. Things like escaped letters, digits,
  // and most symbols will get unescaped with this mode.
  ///
  UU_NORMAL = 1 << 0,

  ///
  // Convert %20 to spaces. In some places where we're showing URLs, we may
  // want this. In places where the URL may be copied and pasted out, then
  // you wouldn't want this since it might not be interpreted in one piece
  // by other applications.
  ///
  UU_SPACES = 1 << 1,

  ///
  // Unescapes '/' and '\\'. If these characters were unescaped, the resulting
  // URL won't be the same as the source one. Moreover, they are dangerous to
  // unescape in strings that will be used as file paths or names. This value
  // should only be used when slashes don't have special meaning, like data
  // URLs.
  ///
  UU_PATH_SEPARATORS = 1 << 2,

  ///
  // Unescapes various characters that will change the meaning of URLs,
  // including '%', '+', '&', '#'. Does not unescape path separators.
  // If these characters were unescaped, the resulting URL won't be the same
  // as the source one. This flag is used when generating final output like
  // filenames for URLs where we won't be interpreting as a URL and want to do
  // as much unescaping as possible.
  ///
  UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS = 1 << 3,

  ///
  // Unescapes characters that can be used in spoofing attempts (such as LOCK)
  // and control characters (such as BiDi control characters and %01).  This
  // INCLUDES NULLs.  This is used for rare cases such as data: URL decoding
  // where the result is binary data.
  //
  // DO NOT use UU_SPOOFING_AND_CONTROL_CHARS if the URL is going to be
  // displayed in the UI for security reasons.
  ///
  UU_SPOOFING_AND_CONTROL_CHARS = 1 << 4,

  ///
  // URL queries use "+" for space. This flag controls that replacement.
  ///
  UU_REPLACE_PLUS_WITH_SPACE = 1 << 5,
} cef_uri_unescape_rule_t;

///
// Options that can be passed to CefParseJSON.
///
typedef enum {
  ///
  // Parses the input strictly according to RFC 4627. See comments in Chromium's
  // base/json/json_reader.h file for known limitations/deviations from the RFC.
  ///
  JSON_PARSER_RFC = 0,

  ///
  // Allows commas to exist after the last element in structures.
  ///
  JSON_PARSER_ALLOW_TRAILING_COMMAS = 1 << 0,
} cef_json_parser_options_t;

///
// Error codes that can be returned from CefParseJSONAndReturnError.
///
typedef enum {
  JSON_NO_ERROR = 0,
  JSON_INVALID_ESCAPE,
  JSON_SYNTAX_ERROR,
  JSON_UNEXPECTED_TOKEN,
  JSON_TRAILING_COMMA,
  JSON_TOO_MUCH_NESTING,
  JSON_UNEXPECTED_DATA_AFTER_ROOT,
  JSON_UNSUPPORTED_ENCODING,
  JSON_UNQUOTED_DICTIONARY_KEY,
  JSON_PARSE_ERROR_COUNT
} cef_json_parser_error_t;

///
// Options that can be passed to CefWriteJSON.
///
typedef enum {
  ///
  // Default behavior.
  ///
  JSON_WRITER_DEFAULT = 0,

  ///
  // This option instructs the writer that if a Binary value is encountered,
  // the value (and key if within a dictionary) will be omitted from the
  // output, and success will be returned. Otherwise, if a binary value is
  // encountered, failure will be returned.
  ///
  JSON_WRITER_OMIT_BINARY_VALUES = 1 << 0,

  ///
  // This option instructs the writer to write doubles that have no fractional
  // part as a normal integer (i.e., without using exponential notation
  // or appending a '.0') as long as the value is within the range of a
  // 64-bit int.
  ///
  JSON_WRITER_OMIT_DOUBLE_TYPE_PRESERVATION = 1 << 1,

  ///
  // Return a slightly nicer formatted json string (pads with whitespace to
  // help with readability).
  ///
  JSON_WRITER_PRETTY_PRINT = 1 << 2,
} cef_json_writer_options_t;

///
// Margin type for PDF printing.
///
typedef enum {
  ///
  // Default margins.
  ///
  PDF_PRINT_MARGIN_DEFAULT,

  ///
  // No margins.
  ///
  PDF_PRINT_MARGIN_NONE,

  ///
  // Minimum margins.
  ///
  PDF_PRINT_MARGIN_MINIMUM,

  ///
  // Custom margins using the |margin_*| values from cef_pdf_print_settings_t.
  ///
  PDF_PRINT_MARGIN_CUSTOM,
} cef_pdf_print_margin_type_t;

///
// Structure representing PDF print settings.
///
typedef struct _cef_pdf_print_settings_t {
  ///
  // Page title to display in the header. Only used if |header_footer_enabled|
  // is set to true (1).
  ///
  cef_string_t header_footer_title;

  ///
  // URL to display in the footer. Only used if |header_footer_enabled| is set
  // to true (1).
  ///
  cef_string_t header_footer_url;

  ///
  // Output page size in microns. If either of these values is less than or
  // equal to zero then the default paper size (A4) will be used.
  ///
  int page_width;
  int page_height;

  ///
  // Margins in millimeters. Only used if |margin_type| is set to
  // PDF_PRINT_MARGIN_CUSTOM.
  ///
  double margin_top;
  double margin_right;
  double margin_bottom;
  double margin_left;

  ///
  // Margin type.
  ///
  cef_pdf_print_margin_type_t margin_type;

  ///
  // Set to true (1) to print headers and footers or false (0) to not print
  // headers and footers.
  ///
  int header_footer_enabled;

  ///
  // Set to true (1) to print the selection only or false (0) to print all.
  ///
  int selection_only;

  ///
  // Set to true (1) for landscape mode or false (0) for portrait mode.
  ///
  int landscape;

  ///
  // Set to true (1) to print background graphics or false (0) to not print
  // background graphics.
  ///
  int backgrounds_enabled;

} cef_pdf_print_settings_t;

///
// Supported UI scale factors for the platform. SCALE_FACTOR_NONE is used for
// density independent resources such as string, html/js files or an image that
// can be used for any scale factors (such as wallpapers).
///
typedef enum {
  SCALE_FACTOR_NONE = 0,
  SCALE_FACTOR_100P,
  SCALE_FACTOR_125P,
  SCALE_FACTOR_133P,
  SCALE_FACTOR_140P,
  SCALE_FACTOR_150P,
  SCALE_FACTOR_180P,
  SCALE_FACTOR_200P,
  SCALE_FACTOR_250P,
  SCALE_FACTOR_300P,
} cef_scale_factor_t;

///
// Plugin policies supported by CefRequestContextHandler::OnBeforePluginLoad.
///
typedef enum {
  ///
  // Allow the content.
  ///
  PLUGIN_POLICY_ALLOW,

  ///
  // Allow important content and block unimportant content based on heuristics.
  // The user can manually load blocked content.
  ///
  PLUGIN_POLICY_DETECT_IMPORTANT,

  ///
  // Block the content. The user can manually load blocked content.
  ///
  PLUGIN_POLICY_BLOCK,

  ///
  // Disable the content. The user cannot load disabled content.
  ///
  PLUGIN_POLICY_DISABLE,
} cef_plugin_policy_t;

///
// Policy for how the Referrer HTTP header value will be sent during navigation.
// If the `--no-referrers` command-line flag is specified then the policy value
// will be ignored and the Referrer value will never be sent.
///
typedef enum {
  ///
  // Always send the complete Referrer value.
  ///
  REFERRER_POLICY_ALWAYS,

  ///
  // Use the default policy. This is REFERRER_POLICY_ORIGIN_WHEN_CROSS_ORIGIN
  // when the `--reduced-referrer-granularity` command-line flag is specified
  // and REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE otherwise.
  //
  ///
  REFERRER_POLICY_DEFAULT,

  ///
  // When navigating from HTTPS to HTTP do not send the Referrer value.
  // Otherwise, send the complete Referrer value.
  ///
  REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE,

  ///
  // Never send the Referrer value.
  ///
  REFERRER_POLICY_NEVER,

  ///
  // Only send the origin component of the Referrer value.
  ///
  REFERRER_POLICY_ORIGIN,

  ///
  // When navigating cross-origin only send the origin component of the Referrer
  // value. Otherwise, send the complete Referrer value.
  ///
  REFERRER_POLICY_ORIGIN_WHEN_CROSS_ORIGIN,
} cef_referrer_policy_t;

///
// Return values for CefResponseFilter::Filter().
///
typedef enum {
  ///
  // Some or all of the pre-filter data was read successfully but more data is
  // needed in order to continue filtering (filtered output is pending).
  ///
  RESPONSE_FILTER_NEED_MORE_DATA,

  ///
  // Some or all of the pre-filter data was read successfully and all available
  // filtered output has been written.
  ///
  RESPONSE_FILTER_DONE,

  ///
  // An error occurred during filtering.
  ///
  RESPONSE_FILTER_ERROR
} cef_response_filter_status_t;

///
// Describes how to interpret the components of a pixel.
///
typedef enum {
  ///
  // RGBA with 8 bits per pixel (32bits total).
  ///
  CEF_COLOR_TYPE_RGBA_8888,

  ///
  // BGRA with 8 bits per pixel (32bits total).
  ///
  CEF_COLOR_TYPE_BGRA_8888,
} cef_color_type_t;

///
// Describes how to interpret the alpha component of a pixel.
///
typedef enum {
  ///
  // No transparency. The alpha component is ignored.
  ///
  CEF_ALPHA_TYPE_OPAQUE,

  ///
  // Transparency with pre-multiplied alpha component.
  ///
  CEF_ALPHA_TYPE_PREMULTIPLIED,
  
  ///
  // Transparency with post-multiplied alpha component.
  ///
  CEF_ALPHA_TYPE_POSTMULTIPLIED,
} cef_alpha_type_t;

///
// Text style types. Should be kepy in sync with gfx::TextStyle.
///
typedef enum {
  CEF_TEXT_STYLE_BOLD,
  CEF_TEXT_STYLE_ITALIC,
  CEF_TEXT_STYLE_STRIKE,
  CEF_TEXT_STYLE_DIAGONAL_STRIKE,
  CEF_TEXT_STYLE_UNDERLINE,
} cef_text_style_t;

///
// Specifies where along the main axis the CefBoxLayout child views should be
// laid out.
///
typedef enum {
  ///
  // Child views will be left-aligned.
  ///
  CEF_MAIN_AXIS_ALIGNMENT_START,

  ///
  // Child views will be center-aligned.
  ///
  CEF_MAIN_AXIS_ALIGNMENT_CENTER,

  ///
  // Child views will be right-aligned.
  ///
  CEF_MAIN_AXIS_ALIGNMENT_END,
} cef_main_axis_alignment_t;

///
// Specifies where along the cross axis the CefBoxLayout child views should be
// laid out.
///
typedef enum {
  ///
  // Child views will be stretched to fit.
  ///
  CEF_CROSS_AXIS_ALIGNMENT_STRETCH,

  ///
  // Child views will be left-aligned.
  ///
  CEF_CROSS_AXIS_ALIGNMENT_START,

  ///
  // Child views will be center-aligned.
  ///
  CEF_CROSS_AXIS_ALIGNMENT_CENTER,

  ///
  // Child views will be right-aligned.
  ///
  CEF_CROSS_AXIS_ALIGNMENT_END,
} cef_cross_axis_alignment_t;

///
// Settings used when initializing a CefBoxLayout.
///
typedef struct _cef_box_layout_settings_t {
  ///
  // If true (1) the layout will be horizontal, otherwise the layout will be
  // vertical.
  ///
  int horizontal;

  ///
  // Adds additional horizontal space between the child view area and the host
  // view border.
  ///
  int inside_border_horizontal_spacing;

  ///
  // Adds additional vertical space between the child view area and the host
  // view border.
  ///
  int inside_border_vertical_spacing;

  ///
  // Adds additional space around the child view area.
  ///
  cef_insets_t inside_border_insets;

  ///
  // Adds additional space between child views.
  ///
  int between_child_spacing;

  ///
  // Specifies where along the main axis the child views should be laid out.
  ///
  cef_main_axis_alignment_t main_axis_alignment;

  ///
  // Specifies where along the cross axis the child views should be laid out.
  ///
  cef_cross_axis_alignment_t cross_axis_alignment;

  ///
  // Minimum cross axis size.
  ///
  int minimum_cross_axis_size;

  ///
  // Default flex for views when none is specified via CefBoxLayout methods.
  // Using the preferred size as the basis, free space along the main axis is
  // distributed to views in the ratio of their flex weights. Similarly, if the
  // views will overflow the parent, space is subtracted in these ratios. A flex
  // of 0 means this view is not resized. Flex values must not be negative.
  ///
  int default_flex;
} cef_box_layout_settings_t;

///
// Specifies the button display state.
///
typedef enum {
  CEF_BUTTON_STATE_NORMAL,
  CEF_BUTTON_STATE_HOVERED,
  CEF_BUTTON_STATE_PRESSED,
  CEF_BUTTON_STATE_DISABLED,
} cef_button_state_t;

///
// Specifies the horizontal text alignment mode.
///
typedef enum {
  ///
  // Align the text's left edge with that of its display area.
  ///
  CEF_HORIZONTAL_ALIGNMENT_LEFT,

  ///
  // Align the text's center with that of its display area.
  ///
  CEF_HORIZONTAL_ALIGNMENT_CENTER,

  ///
  // Align the text's right edge with that of its display area.
  ///
  CEF_HORIZONTAL_ALIGNMENT_RIGHT,
} cef_horizontal_alignment_t;

///
// Specifies how a menu will be anchored for non-RTL languages. The opposite
// position will be used for RTL languages.
///
typedef enum {
  CEF_MENU_ANCHOR_TOPLEFT,
  CEF_MENU_ANCHOR_TOPRIGHT,
  CEF_MENU_ANCHOR_BOTTOMCENTER,
} cef_menu_anchor_position_t;

// Supported SSL version values. See net/ssl/ssl_connection_status_flags.h
// for more information.
typedef enum {
  SSL_CONNECTION_VERSION_UNKNOWN = 0,  // Unknown SSL version.
  SSL_CONNECTION_VERSION_SSL2 = 1,
  SSL_CONNECTION_VERSION_SSL3 = 2,
  SSL_CONNECTION_VERSION_TLS1 = 3,
  SSL_CONNECTION_VERSION_TLS1_1 = 4,
  SSL_CONNECTION_VERSION_TLS1_2 = 5,
  // Reserve 6 for TLS 1.3.
  SSL_CONNECTION_VERSION_QUIC = 7,
} cef_ssl_version_t;

// Supported SSL content status flags. See content/public/common/ssl_status.h
// for more information.
typedef enum {
  SSL_CONTENT_NORMAL_CONTENT = 0,
  SSL_CONTENT_DISPLAYED_INSECURE_CONTENT = 1 << 0,
  SSL_CONTENT_RAN_INSECURE_CONTENT = 1 << 1,
} cef_ssl_content_status_t;

///
// Error codes for CDM registration. See cef_web_plugin.h for details.
///
typedef enum {
  ///
  // No error. Registration completed successfully.
  ///
  CEF_CDM_REGISTRATION_ERROR_NONE,

  ///
  // Required files or manifest contents are missing.
  ///
  CEF_CDM_REGISTRATION_ERROR_INCORRECT_CONTENTS,

  ///
  // The CDM is incompatible with the current Chromium version.
  ///
  CEF_CDM_REGISTRATION_ERROR_INCOMPATIBLE,

  ///
  // CDM registration is not supported at this time.
  ///
  CEF_CDM_REGISTRATION_ERROR_NOT_SUPPORTED,
} cef_cdm_registration_error_t;

#ifdef __cplusplus
}
#endif

#endif  // CEF_INCLUDE_INTERNAL_CEF_TYPES_H_
