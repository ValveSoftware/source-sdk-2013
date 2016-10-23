// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_VIEWS_WINDOW_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_VIEWS_WINDOW_H_
#pragma once

#include <string>

#include "include/cef_menu_model_delegate.h"
#include "include/views/cef_button_delegate.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_browser_view_delegate.h"
#include "include/views/cef_label_button.h"
#include "include/views/cef_menu_button.h"
#include "include/views/cef_menu_button_delegate.h"
#include "include/views/cef_textfield.h"
#include "include/views/cef_textfield_delegate.h"
#include "include/views/cef_window.h"
#include "include/views/cef_window_delegate.h"

namespace client {

// Implements a CefWindow that hosts a single CefBrowserView and optional
// Views-based controls. All methods must be called on the browser process UI
// thread.
class ViewsWindow : public CefBrowserViewDelegate,
                    public CefMenuButtonDelegate,
                    public CefMenuModelDelegate,
                    public CefTextfieldDelegate,
                    public CefWindowDelegate {
 public:
  // Delegate methods will be called on the browser process UI thread.
  class Delegate {
   public:
    // Return true if the window should show controls.
    virtual bool WithControls() =0;

    // Return the initial window bounds.
    virtual CefRect GetWindowBounds() =0;

    // Called when the ViewsWindow is created.
    virtual void OnViewsWindowCreated(CefRefPtr<ViewsWindow> window) =0;

    // Called when the ViewsWindow is destroyed. All references to |window|
    // should be released in this callback.
    virtual void OnViewsWindowDestroyed(CefRefPtr<ViewsWindow> window) =0;

    // Return the Delegate for the popup window controlled by |client|.
    virtual Delegate* GetDelegateForPopup(CefRefPtr<CefClient> client) =0;

    // Called to execute a test. See resource.h for |test_id| values.
    virtual void OnTest(int test_id) = 0;

    // Called to exit the application.
    virtual void OnExit() = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Create a new top-level ViewsWindow hosting a browser with the specified
  // configuration.
  static CefRefPtr<ViewsWindow> Create(
      Delegate* delegate,
      CefRefPtr<CefClient> client,
      const CefString& url,
      const CefBrowserSettings& settings,
      CefRefPtr<CefRequestContext> request_context);

  void Show();
  void Hide();
  void Minimize();
  void Maximize();
  void SetBounds(const CefRect& bounds);
  void Close(bool force);
  void SetAddress(const std::string& url);
  void SetTitle(const std::string& title);
  void SetFavicon(CefRefPtr<CefImage> image);
  void SetFullscreen(bool fullscreen);
  void SetLoadingState(bool isLoading,
                       bool canGoBack,
                       bool canGoForward);
  void SetDraggableRegions(const std::vector<CefDraggableRegion>& regions);

  // CefBrowserViewDelegate methods:
  bool OnPopupBrowserViewCreated(
      CefRefPtr<CefBrowserView> browser_view,
      CefRefPtr<CefBrowserView> popup_browser_view,
      bool is_devtools) OVERRIDE;

  // CefButtonDelegate methods:
  void OnButtonPressed(CefRefPtr<CefButton> button) OVERRIDE;

  // CefMenuButtonDelegate methods:
  void OnMenuButtonPressed(CefRefPtr<CefMenuButton> menu_button,
                           const CefPoint& screen_point) OVERRIDE;

  // CefMenuModelDelegate methods:
  void ExecuteCommand(CefRefPtr<CefMenuModel> menu_model,
                      int command_id,
                      cef_event_flags_t event_flags) OVERRIDE;

  // CefTextfieldDelegate methods:
  bool OnKeyEvent(CefRefPtr<CefTextfield> textfield,
                  const CefKeyEvent& event) OVERRIDE;

  // CefWindowDelegate methods:
  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE;
  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE;
  bool IsFrameless(CefRefPtr<CefWindow> window) OVERRIDE;
  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE;

  // CefViewDelegate methods:
  CefSize GetMinimumSize(CefRefPtr<CefView> view) override;

 private:
  // |delegate| is guaranteed to outlive this object.
  // |browser_view| may be NULL, in which case SetBrowserView() will be called.
  ViewsWindow(Delegate* delegate,
              CefRefPtr<CefBrowserView> browser_view);

  void SetBrowserView(CefRefPtr<CefBrowserView> browser_view);

  // Create controls.
  void CreateMenuModel();
  CefRefPtr<CefLabelButton> CreateBrowseButton(const std::string& label,
                                               int id);

  // Add controls to the Window.
  void AddControls();

  // Enable or disable a view by |id|.
  void EnableView(int id, bool enable);

  // Show/hide top controls on the Window.
  void ShowTopControls(bool show);

  Delegate* delegate_;  // Not owned by this object.
  CefRefPtr<CefBrowserView> browser_view_;
  bool frameless_;
  bool with_controls_;
  CefRefPtr<CefWindow> window_;

  CefRefPtr<CefMenuModel> menu_model_;

  CefSize minimum_window_size_;

  IMPLEMENT_REFCOUNTING(ViewsWindow);
  DISALLOW_COPY_AND_ASSIGN(ViewsWindow);
};

}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_BROWSER_VIEWS_WINDOW_H_
