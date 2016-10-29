// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_H_
#pragma once

#include <set>
#include <string>

#include "include/cef_client.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_message_router.h"
#include "include/wrapper/cef_resource_manager.h"
#include "cefclient/browser/client_types.h"

#if defined(OS_LINUX)
#include "cefclient/browser/dialog_handler_gtk.h"
#endif

namespace client {

class ClientDownloadImageCallback;

// Client handler abstract base class. Provides common functionality shared by
// all concrete client handler implementations.
class ClientHandler : public CefClient,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
                      public CefDragHandler,
                      public CefGeolocationHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRequestHandler {
 public:
  // Implement this interface to receive notification of ClientHandler
  // events. The methods of this class will be called on the main thread.
  class Delegate {
   public:
    // Called when the browser is created.
    virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser) = 0;

    // Called when the browser is closing.
    virtual void OnBrowserClosing(CefRefPtr<CefBrowser> browser) = 0;

    // Called when the browser has been closed.
    virtual void OnBrowserClosed(CefRefPtr<CefBrowser> browser) = 0;

    // Set the window URL address.
    virtual void OnSetAddress(const std::string& url) = 0;

    // Set the window title.
    virtual void OnSetTitle(const std::string& title) = 0;

    // Set the Favicon image.
    virtual void OnSetFavicon(CefRefPtr<CefImage> image) {};

    // Set fullscreen mode.
    virtual void OnSetFullscreen(bool fullscreen) = 0;

    // Set the loading state.
    virtual void OnSetLoadingState(bool isLoading,
                                   bool canGoBack,
                                   bool canGoForward) = 0;

    // Set the draggable regions.
    virtual void OnSetDraggableRegions(
        const std::vector<CefDraggableRegion>& regions) = 0;

   protected:
    virtual ~Delegate() {}
  };

  typedef std::set<CefMessageRouterBrowserSide::Handler*> MessageHandlerSet;

  // Constructor may be called on any thread.
  // |delegate| must outlive this object or DetachDelegate() must be called.
  ClientHandler(Delegate* delegate,
                bool is_osr,
                const std::string& startup_url);

  // This object may outlive the Delegate object so it's necessary for the
  // Delegate to detach itself before destruction.
  void DetachDelegate();

  // CefClient methods
  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefDragHandler> GetDragHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefGeolocationHandler> GetGeolocationHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE {
    return this;
  }
  CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE {
    return this;
  }
  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) OVERRIDE;

#if defined(OS_LINUX)
  CefRefPtr<CefDialogHandler> GetDialogHandler() OVERRIDE {
    return dialog_handler_;
  }
  CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() OVERRIDE {
    return dialog_handler_;
  }
#endif

  // CefContextMenuHandler methods
  void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefContextMenuParams> params,
                           CefRefPtr<CefMenuModel> model) OVERRIDE;
  bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            CefRefPtr<CefContextMenuParams> params,
                            int command_id,
                            EventFlags event_flags) OVERRIDE;

  // CefDisplayHandler methods
  void OnAddressChange(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       const CefString& url) OVERRIDE;
  void OnTitleChange(CefRefPtr<CefBrowser> browser,
                     const CefString& title) OVERRIDE;
  void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                          const std::vector<CefString>& icon_urls) OVERRIDE;
  void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser,
                              bool fullscreen) OVERRIDE;
  bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                        const CefString& message,
                        const CefString& source,
                        int line) OVERRIDE;

  // CefDownloadHandler methods
  void OnBeforeDownload(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefDownloadItem> download_item,
      const CefString& suggested_name,
      CefRefPtr<CefBeforeDownloadCallback> callback) OVERRIDE;
  void OnDownloadUpdated(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefDownloadItem> download_item,
      CefRefPtr<CefDownloadItemCallback> callback) OVERRIDE;

  // CefDragHandler methods
  bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefDragData> dragData,
                   CefDragHandler::DragOperationsMask mask) OVERRIDE;

  void OnDraggableRegionsChanged(
      CefRefPtr<CefBrowser> browser,
      const std::vector<CefDraggableRegion>& regions) OVERRIDE;

  // CefGeolocationHandler methods
  bool OnRequestGeolocationPermission(
      CefRefPtr<CefBrowser> browser,
      const CefString& requesting_url,
      int request_id,
      CefRefPtr<CefGeolocationCallback> callback) OVERRIDE;

  // CefKeyboardHandler methods
  bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                     const CefKeyEvent& event,
                     CefEventHandle os_event,
                     bool* is_keyboard_shortcut) OVERRIDE;

  // CefLifeSpanHandler methods
  bool OnBeforePopup(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& target_url,
      const CefString& target_frame_name,
      CefLifeSpanHandler::WindowOpenDisposition target_disposition,
      bool user_gesture,
      const CefPopupFeatures& popupFeatures,
      CefWindowInfo& windowInfo,
      CefRefPtr<CefClient>& client,
      CefBrowserSettings& settings,
      bool* no_javascript_access) OVERRIDE;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

  // CefLoadHandler methods
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                            bool isLoading,
                            bool canGoBack,
                            bool canGoForward) OVERRIDE;
  void OnLoadError(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   ErrorCode errorCode,
                   const CefString& errorText,
                   const CefString& failedUrl) OVERRIDE;

  // CefRequestHandler methods
  bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame,
                      CefRefPtr<CefRequest> request,
                      bool is_redirect) OVERRIDE;
  bool OnOpenURLFromTab(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& target_url,
      CefRequestHandler::WindowOpenDisposition target_disposition,
      bool user_gesture) OVERRIDE;
  cef_return_value_t OnBeforeResourceLoad(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefRequestCallback> callback) OVERRIDE;
  CefRefPtr<CefResourceHandler> GetResourceHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) OVERRIDE;
  CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefResponse> response) OVERRIDE;
  bool OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                      const CefString& origin_url,
                      int64 new_size,
                      CefRefPtr<CefRequestCallback> callback) OVERRIDE;
  void OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                           const CefString& url,
                           bool& allow_os_execution) OVERRIDE;
  bool OnCertificateError(
      CefRefPtr<CefBrowser> browser,
      ErrorCode cert_error,
      const CefString& request_url,
      CefRefPtr<CefSSLInfo> ssl_info,
      CefRefPtr<CefRequestCallback> callback) OVERRIDE;
  void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                 TerminationStatus status) OVERRIDE;

  // Returns the number of browsers currently using this handler. Can only be
  // called on the CEF UI thread.
  int GetBrowserCount() const;

  // Show a new DevTools popup window.
  void ShowDevTools(CefRefPtr<CefBrowser> browser,
                    const CefPoint& inspect_element_at);

  // Close the existing DevTools popup window, if any.
  void CloseDevTools(CefRefPtr<CefBrowser> browser);

  // Test if the current site has SSL information available.
  bool HasSSLInformation(CefRefPtr<CefBrowser> browser);

  // Show SSL information for the current site.
  void ShowSSLInformation(CefRefPtr<CefBrowser> browser);

  // Returns the Delegate.
  Delegate* delegate() const { return delegate_; }

  // Returns the startup URL.
  std::string startup_url() const { return startup_url_; }

  // Returns true if this handler uses off-screen rendering.
  bool is_osr() const { return is_osr_; }

  // Set/get whether the client should download favicon images. Only safe to
  // call immediately after client creation or on the browser process UI thread.
  bool download_favicon_images() const { return download_favicon_images_; }
  void set_download_favicon_images(bool allow) {
    download_favicon_images_ = allow;
  }

 private:
  friend class ClientDownloadImageCallback;

  // Create a new popup window using the specified information. |is_devtools|
  // will be true if the window will be used for DevTools. Return true to
  // proceed with popup browser creation or false to cancel the popup browser.
  // May be called on any thead.
  bool CreatePopupWindow(
      CefRefPtr<CefBrowser> browser,
      bool is_devtools,
      const CefPopupFeatures& popupFeatures,
      CefWindowInfo& windowInfo,
      CefRefPtr<CefClient>& client,
      CefBrowserSettings& settings);

  // Execute Delegate notifications on the main thread.
  void NotifyBrowserCreated(CefRefPtr<CefBrowser> browser);
  void NotifyBrowserClosing(CefRefPtr<CefBrowser> browser);
  void NotifyBrowserClosed(CefRefPtr<CefBrowser> browser);
  void NotifyAddress(const CefString& url);
  void NotifyTitle(const CefString& title);
  void NotifyFavicon(CefRefPtr<CefImage> image);
  void NotifyFullscreen(bool fullscreen);
  void NotifyLoadingState(bool isLoading,
                          bool canGoBack,
                          bool canGoForward);
  void NotifyDraggableRegions(
      const std::vector<CefDraggableRegion>& regions);

  // Test context menu creation.
  void BuildTestMenu(CefRefPtr<CefMenuModel> model);
  bool ExecuteTestMenu(int command_id);

  // THREAD SAFE MEMBERS
  // The following members may be accessed from any thread.

  // True if this handler uses off-screen rendering.
  const bool is_osr_;

  // The startup URL.
  const std::string startup_url_;

  // True if mouse cursor change is disabled.
  bool mouse_cursor_change_disabled_;

  // True if Favicon images should be downloaded.
  bool download_favicon_images_;

#if defined(OS_LINUX)
  // Custom dialog handler for GTK.
  CefRefPtr<ClientDialogHandlerGtk> dialog_handler_;
#endif

  // Handles the browser side of query routing. The renderer side is handled
  // in client_renderer.cc.
  CefRefPtr<CefMessageRouterBrowserSide> message_router_;

  // Manages the registration and delivery of resources.
  CefRefPtr<CefResourceManager> resource_manager_;

  // MAIN THREAD MEMBERS
  // The following members will only be accessed on the main thread. This will
  // be the same as the CEF UI thread except when using multi-threaded message
  // loop mode on Windows.

  Delegate* delegate_;

  // UI THREAD MEMBERS
  // The following members will only be accessed on the CEF UI thread.

  // Track state information for the text context menu.
  struct TestMenuState {
    TestMenuState() : check_item(true), radio_item(0) {}
    bool check_item;
    int radio_item;
  } test_menu_state_;

  // The current number of browsers using this handler.
  int browser_count_;

  // Console logging state.
  const std::string console_log_file_;
  bool first_console_message_;

  // True if an editable field currently has focus.
  bool focus_on_editable_field_;

  // Set of Handlers registered with the message router.
  MessageHandlerSet message_handler_set_;

  DISALLOW_COPY_AND_ASSIGN(ClientHandler);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_CLIENT_HANDLER_H_
