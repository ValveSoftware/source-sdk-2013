// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/osr_window_win.h"

#include <windowsx.h>

#include "include/base/cef_build.h"
#include "cefclient/browser/geometry_util.h"
#include "cefclient/browser/main_message_loop.h"
#include "cefclient/browser/resource.h"
#include "cefclient/browser/util_win.h"

namespace client {

namespace {

const wchar_t kWndClass[] = L"Client_OsrWindow";

// Render at 30fps during rotation.
const int kRenderDelay = 1000 / 30;

// Helper that calls wglMakeCurrent.
class ScopedGLContext {
 public:
  ScopedGLContext(HDC hdc, HGLRC hglrc, bool swap_buffers)
    : hdc_(hdc),
      swap_buffers_(swap_buffers) {
    BOOL result = wglMakeCurrent(hdc, hglrc);
    ALLOW_UNUSED_LOCAL(result);
    DCHECK(result);
  }
  ~ScopedGLContext() {
    BOOL result = wglMakeCurrent(NULL, NULL);
    DCHECK(result);
    if (swap_buffers_) {
      result = SwapBuffers(hdc_);
      DCHECK(result);
    }
  }

 private:
  const HDC hdc_;
  const bool swap_buffers_;
};

}  // namespace


OsrWindowWin::OsrWindowWin(Delegate* delegate,
                           const OsrRenderer::Settings& settings)
    : delegate_(delegate),
      renderer_(settings),
      hwnd_(NULL),
      hdc_(NULL),
      hrc_(NULL),
      client_rect_(),
      device_scale_factor_(client::GetDeviceScaleFactor()),
      painting_popup_(false),
      render_task_pending_(false),
      hidden_(false),
      last_mouse_pos_(),
      current_mouse_pos_(),
      mouse_rotation_(false),
      mouse_tracking_(false),
      last_click_x_(0),
      last_click_y_(0),
      last_click_button_(MBT_LEFT),
      last_click_count_(0),
      last_click_time_(0),
      last_mouse_down_on_view_(false) {
  DCHECK(delegate_);
}

OsrWindowWin::~OsrWindowWin() {
  CEF_REQUIRE_UI_THREAD();
  // The native window should have already been destroyed.
  DCHECK(!hwnd_);
}

void OsrWindowWin::CreateBrowser(HWND parent_hwnd,
                                 const RECT& rect,
                                 CefRefPtr<CefClient> handler,
                                 const CefBrowserSettings& settings,
                                 CefRefPtr<CefRequestContext> request_context,
                                 const std::string& startup_url) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::CreateBrowser, this,
                                   parent_hwnd, rect, handler, settings,
                                   request_context, startup_url));
    return;
  }

  // Create the native window.
  Create(parent_hwnd, rect);

  CefWindowInfo window_info;
  window_info.SetAsWindowless(hwnd_, renderer_.IsTransparent());

  // Create the browser asynchronously.
  CefBrowserHost::CreateBrowser(window_info, handler, startup_url, settings,
                                request_context);
}

void OsrWindowWin::ShowPopup(HWND parent_hwnd,
                             int x, int y, size_t width, size_t height) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::ShowPopup, this,
                                   parent_hwnd, x, y, width, height));
    return;
  }

  DCHECK(browser_.get());

  // Create the native window.
  const RECT rect = {x, y,
                     x + static_cast<int>(width),
                     y + static_cast<int>(height)};
  Create(parent_hwnd, rect);

  // Send resize notification so the compositor is assigned the correct
  // viewport size and begins rendering.
  browser_->GetHost()->WasResized();

  Show();
}

void OsrWindowWin::Show() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::Show, this));
    return;
  }
  
  if (!browser_)
    return;

  // Show the native window if not currently visible.
  if (hwnd_ && !::IsWindowVisible(hwnd_))
    ShowWindow(hwnd_, SW_SHOW);

  if (hidden_) {
    // Set the browser as visible.
    browser_->GetHost()->WasHidden(false);
    hidden_ = false;
  }

  // Give focus to the browser.
  browser_->GetHost()->SendFocusEvent(true);
}

void OsrWindowWin::Hide() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::Hide, this));
    return;
  }

  if (!browser_)
    return;

  // Remove focus from the browser.
  browser_->GetHost()->SendFocusEvent(false);

  if (!hidden_) {
    // Set the browser as hidden.
    browser_->GetHost()->WasHidden(true);
    hidden_ = true;
  }
}

void OsrWindowWin::SetBounds(int x, int y, size_t width, size_t height) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetBounds, this, x, y, width,
                                   height));
    return;
  }

  if (hwnd_) {
    // Set the browser window bounds.
    ::SetWindowPos(hwnd_, NULL, x, y,
                   static_cast<int>(width), static_cast<int>(height),
                   SWP_NOZORDER);
  }
}

void OsrWindowWin::SetFocus() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetFocus, this));
    return;
  }

   if (hwnd_) {
    // Give focus to the native window.
    ::SetFocus(hwnd_);
  }
}

void OsrWindowWin::SetDeviceScaleFactor(float device_scale_factor) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute this method on the UI thread.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::SetDeviceScaleFactor, this,
                                   device_scale_factor));
    return;
  }

  if (device_scale_factor == device_scale_factor_)
    return;

  device_scale_factor_ = device_scale_factor;
  if (browser_) {
    browser_->GetHost()->NotifyScreenInfoChanged();
    browser_->GetHost()->WasResized();
  }
}

void OsrWindowWin::Create(HWND parent_hwnd, const RECT& rect) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK(!hwnd_ && !hdc_ && !hrc_);
  DCHECK(parent_hwnd);
  DCHECK(!::IsRectEmpty(&rect));

  HINSTANCE hInst = ::GetModuleHandle(NULL);

  const cef_color_t background_color = renderer_.GetBackgroundColor();
  const HBRUSH background_brush = CreateSolidBrush(
      RGB(CefColorGetR(background_color),
          CefColorGetG(background_color),
          CefColorGetB(background_color)));

  RegisterOsrClass(hInst, background_brush);

  // Create the native window with a border so it's easier to visually identify
  // OSR windows.
  hwnd_ = ::CreateWindow(kWndClass, 0,
      WS_BORDER | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
      rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
      parent_hwnd, 0, hInst, 0);
  CHECK(hwnd_);

  client_rect_ = rect;

  // Associate |this| with the window.
  SetUserDataPtr(hwnd_, this);

#if defined(CEF_USE_ATL)
  // Create/register the drag&drop handler.
  drop_target_ = DropTargetWin::Create(this, hwnd_);
  HRESULT register_res = RegisterDragDrop(hwnd_, drop_target_);
  DCHECK_EQ(register_res, S_OK);
#endif

  // Notify the window owner.
  NotifyNativeWindowCreated(hwnd_);
}

void OsrWindowWin::Destroy() {
  CEF_REQUIRE_UI_THREAD();
  DCHECK(hwnd_ != NULL);

#if defined(CEF_USE_ATL)
  // Revoke/delete the drag&drop handler.
  RevokeDragDrop(hwnd_);
  drop_target_ = NULL;
#endif

  DisableGL();

  // Destroy the native window.
  ::DestroyWindow(hwnd_);
  hwnd_ = NULL;
}

void OsrWindowWin::EnableGL() {
  CEF_REQUIRE_UI_THREAD();

  PIXELFORMATDESCRIPTOR pfd;
  int format;

  // Get the device context.
  hdc_ = GetDC(hwnd_);

  // Set the pixel format for the DC.
  ZeroMemory(&pfd, sizeof(pfd));
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 16;
  pfd.iLayerType = PFD_MAIN_PLANE;
  format = ChoosePixelFormat(hdc_, &pfd);
  SetPixelFormat(hdc_, format, &pfd);

  // Create and enable the render context.
  hrc_ = wglCreateContext(hdc_);

  ScopedGLContext scoped_gl_context(hdc_, hrc_, false);
  renderer_.Initialize();
}

void OsrWindowWin::DisableGL() {
  CEF_REQUIRE_UI_THREAD();

  if (!hdc_)
    return;

  {
    ScopedGLContext scoped_gl_context(hdc_, hrc_, false);
    renderer_.Cleanup();
  }

  if (IsWindow(hwnd_)) {
    // wglDeleteContext will make the context not current before deleting it.
    BOOL result = wglDeleteContext(hrc_);
    ALLOW_UNUSED_LOCAL(result);
    DCHECK(result);
    ReleaseDC(hwnd_, hdc_);
  }

  hdc_ = NULL;
  hrc_ = NULL;
}

void OsrWindowWin::Invalidate() {
  CEF_REQUIRE_UI_THREAD();

  // Don't post another task if the previous task is still pending.
  if (render_task_pending_)
    return;
  render_task_pending_ = true;

  CefPostDelayedTask(TID_UI, base::Bind(&OsrWindowWin::Render, this),
                     kRenderDelay);
}

void OsrWindowWin::Render() {
  CEF_REQUIRE_UI_THREAD();

  if (render_task_pending_)
    render_task_pending_ = false;

  if (!hdc_)
    EnableGL();

  ScopedGLContext scoped_gl_context(hdc_, hrc_, true);
  renderer_.Render();
}

void OsrWindowWin::NotifyNativeWindowCreated(HWND hwnd) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&OsrWindowWin::NotifyNativeWindowCreated, this, hwnd));
    return;
  }

  delegate_->OnOsrNativeWindowCreated(hwnd);
}

// static
void OsrWindowWin::RegisterOsrClass(HINSTANCE hInstance,
                                    HBRUSH background_brush) {
  // Only register the class one time.
  static bool class_registered = false;
  if (class_registered)
    return;
  class_registered = true;

  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style         = CS_OWNDC;
  wcex.lpfnWndProc   = OsrWndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = NULL;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = background_brush;
  wcex.lpszMenuName  = NULL;
  wcex.lpszClassName = kWndClass;
  wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  RegisterClassEx(&wcex);
}

// static
LRESULT CALLBACK OsrWindowWin::OsrWndProc(HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam) {
  CEF_REQUIRE_UI_THREAD();

  OsrWindowWin* self = GetUserDataPtr<OsrWindowWin*>(hWnd);
  if (!self)
    return DefWindowProc(hWnd, message, wParam, lParam);

  switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSELEAVE:
    case WM_MOUSEWHEEL:
      self->OnMouseEvent(message, wParam, lParam);
      break;

    case WM_SIZE:
      self->OnSize();
      break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      self->OnFocus(message == WM_SETFOCUS);
      break;

    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
      self->OnCaptureLost();
      break;

    case WM_SYSCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
      self->OnKeyEvent(message, wParam, lParam);
      break;

    case WM_PAINT:
      self->OnPaint();
      return 0;

    case WM_ERASEBKGND:
      if (self->OnEraseBkgnd())
        break;
      // Don't erase the background.
      return 0;

    case WM_NCDESTROY:
      // Clear the reference to |self|.
      SetUserDataPtr(hWnd, NULL);
      self->hwnd_ = NULL;
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void OsrWindowWin::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam) {
  CefRefPtr<CefBrowserHost> browser_host;
  if (browser_)
    browser_host = browser_->GetHost();

  LONG currentTime = 0;
  bool cancelPreviousClick = false;

  if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
      message == WM_MBUTTONDOWN || message == WM_MOUSEMOVE ||
      message == WM_MOUSELEAVE) {
    currentTime = GetMessageTime();
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    cancelPreviousClick =
        (abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2))
        || (abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2))
        || ((currentTime - last_click_time_) > GetDoubleClickTime());
    if (cancelPreviousClick &&
        (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE)) {
      last_click_count_ = 0;
      last_click_x_ = 0;
      last_click_y_ = 0;
      last_click_time_ = 0;
    }
  }

  switch(message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
      ::SetCapture(hwnd_);
      ::SetFocus(hwnd_);
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (wParam & MK_SHIFT) {
        // Start rotation effect.
        last_mouse_pos_.x = current_mouse_pos_.x = x;
        last_mouse_pos_.y = current_mouse_pos_.y = y;
        mouse_rotation_ = true;
      } else {
        CefBrowserHost::MouseButtonType btnType =
            (message == WM_LBUTTONDOWN ? MBT_LEFT : (
             message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
        if (!cancelPreviousClick && (btnType == last_click_button_)) {
          ++last_click_count_;
        } else {
          last_click_count_ = 1;
          last_click_x_ = x;
          last_click_y_ = y;
        }
        last_click_time_ = currentTime;
        last_click_button_ = btnType;

        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          last_mouse_down_on_view_ = !IsOverPopupWidget(x, y);
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseClickEvent(mouse_event, btnType, false,
                                            last_click_count_);
        }
      }
    } break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      if (GetCapture() == hwnd_)
        ReleaseCapture();
      if (mouse_rotation_) {
        // End rotation effect.
        mouse_rotation_ = false;
        renderer_.SetSpin(0, 0);
        Invalidate();
      } else {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        CefBrowserHost::MouseButtonType btnType =
            (message == WM_LBUTTONUP ? MBT_LEFT : (
             message == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));
        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          if (last_mouse_down_on_view_ &&
              IsOverPopupWidget(x, y) &&
              (GetPopupXOffset() || GetPopupYOffset())) {
            break;
          }
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseClickEvent(mouse_event, btnType, true,
                                            last_click_count_);
        }
      }
      break;

    case WM_MOUSEMOVE: {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (mouse_rotation_) {
        // Apply rotation effect.
        current_mouse_pos_.x = x;
        current_mouse_pos_.y = y;
        renderer_.IncrementSpin(
          current_mouse_pos_.x - last_mouse_pos_.x,
          current_mouse_pos_.y - last_mouse_pos_.y);
        last_mouse_pos_.x = current_mouse_pos_.x;
        last_mouse_pos_.y = current_mouse_pos_.y;
        Invalidate();
      } else {
        if (!mouse_tracking_) {
          // Start tracking mouse leave. Required for the WM_MOUSELEAVE event to
          // be generated.
          TRACKMOUSEEVENT tme;
          tme.cbSize = sizeof(TRACKMOUSEEVENT);
          tme.dwFlags = TME_LEAVE;
          tme.hwndTrack = hwnd_;
          TrackMouseEvent(&tme);
          mouse_tracking_ = true;
        }

        if (browser_host) {
          CefMouseEvent mouse_event;
          mouse_event.x = x;
          mouse_event.y = y;
          ApplyPopupOffset(mouse_event.x, mouse_event.y);
          DeviceToLogical(mouse_event, device_scale_factor_);
          mouse_event.modifiers = GetCefMouseModifiers(wParam);
          browser_host->SendMouseMoveEvent(mouse_event, false);
        }
      }
      break;
    }

    case WM_MOUSELEAVE: {
      if (mouse_tracking_) {
        // Stop tracking mouse leave.
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE & TME_CANCEL;
        tme.hwndTrack = hwnd_;
        TrackMouseEvent(&tme);
        mouse_tracking_ = false;
      }

      if (browser_host) {
        // Determine the cursor position in screen coordinates.
        POINT p;
        ::GetCursorPos(&p);
        ::ScreenToClient(hwnd_, &p);

        CefMouseEvent mouse_event;
        mouse_event.x = p.x;
        mouse_event.y = p.y;
        DeviceToLogical(mouse_event, device_scale_factor_);
        mouse_event.modifiers = GetCefMouseModifiers(wParam);
        browser_host->SendMouseMoveEvent(mouse_event, true);
      }
    } break;

    case WM_MOUSEWHEEL:
      if (browser_host) {
        POINT screen_point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        HWND scrolled_wnd = ::WindowFromPoint(screen_point);
        if (scrolled_wnd != hwnd_)
          break;

        ScreenToClient(hwnd_, &screen_point);
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);

        CefMouseEvent mouse_event;
        mouse_event.x = screen_point.x;
        mouse_event.y = screen_point.y;
        ApplyPopupOffset(mouse_event.x, mouse_event.y);
        DeviceToLogical(mouse_event, device_scale_factor_);
        mouse_event.modifiers = GetCefMouseModifiers(wParam);
        browser_host->SendMouseWheelEvent(mouse_event,
                                          IsKeyDown(VK_SHIFT) ? delta : 0,
                                          !IsKeyDown(VK_SHIFT) ? delta : 0);
      }
      break;
  }
}

void OsrWindowWin::OnSize() {
  // Keep |client_rect_| up to date.
  ::GetClientRect(hwnd_, &client_rect_);

  if (browser_)
    browser_->GetHost()->WasResized();
}

void OsrWindowWin::OnFocus(bool setFocus) {
  if (browser_)
    browser_->GetHost()->SendFocusEvent(setFocus);
}

void OsrWindowWin::OnCaptureLost() {
  if (mouse_rotation_)
    return;

  if (browser_)
    browser_->GetHost()->SendCaptureLostEvent();
}

void OsrWindowWin::OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {
  if (!browser_)
    return;

  CefKeyEvent event;
  event.windows_key_code = wParam;
  event.native_key_code = lParam;
  event.is_system_key = message == WM_SYSCHAR ||
                        message == WM_SYSKEYDOWN ||
                        message == WM_SYSKEYUP;

  if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
    event.type = KEYEVENT_RAWKEYDOWN;
  else if (message == WM_KEYUP || message == WM_SYSKEYUP)
    event.type = KEYEVENT_KEYUP;
  else
    event.type = KEYEVENT_CHAR;
  event.modifiers = GetCefKeyboardModifiers(wParam, lParam);

  browser_->GetHost()->SendKeyEvent(event);
}

void OsrWindowWin::OnPaint() {
  // Paint nothing here. Invalidate will cause OnPaint to be called for the
  // render handler.
  PAINTSTRUCT ps;
  BeginPaint(hwnd_, &ps);
  EndPaint(hwnd_, &ps);

  if (browser_)
    browser_->GetHost()->Invalidate(PET_VIEW);
}

bool OsrWindowWin::OnEraseBkgnd() {
  // Erase the background when the browser does not exist.
  return (browser_ == NULL);
}

bool OsrWindowWin::IsOverPopupWidget(int x, int y) const {
  CEF_REQUIRE_UI_THREAD();
  const CefRect& rc = renderer_.popup_rect();
  int popup_right = rc.x + rc.width;
  int popup_bottom = rc.y + rc.height;
  return (x >= rc.x) && (x < popup_right) &&
         (y >= rc.y) && (y < popup_bottom);
}

int OsrWindowWin::GetPopupXOffset() const {
  CEF_REQUIRE_UI_THREAD();
  return renderer_.original_popup_rect().x - renderer_.popup_rect().x;
}

int OsrWindowWin::GetPopupYOffset() const {
  CEF_REQUIRE_UI_THREAD();
  return renderer_.original_popup_rect().y - renderer_.popup_rect().y;
}

void OsrWindowWin::ApplyPopupOffset(int& x, int& y) const {
  if (IsOverPopupWidget(x, y)) {
    x += GetPopupXOffset();
    y += GetPopupYOffset();
  }
}

void OsrWindowWin::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK(!browser_);
  browser_ = browser;

  if (hwnd_) {
    // Show the browser window. Called asynchronously so that the browser has
    // time to create associated internal objects.
    CefPostTask(TID_UI, base::Bind(&OsrWindowWin::Show, this));
  }
}

void OsrWindowWin::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  // Detach |this| from the ClientHandlerOsr.
  static_cast<ClientHandlerOsr*>(browser_->GetHost()->GetClient().get())->
      DetachOsrDelegate();
  browser_ = NULL;
  Destroy();
}

bool OsrWindowWin::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                     CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();
  return false;
}

bool OsrWindowWin::GetViewRect(CefRefPtr<CefBrowser> browser,
                               CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  rect.x = rect.y = 0;
  rect.width = DeviceToLogical(client_rect_.right - client_rect_.left,
                               device_scale_factor_);
  rect.height = DeviceToLogical(client_rect_.bottom - client_rect_.top,
                                device_scale_factor_);
  return true;
}

bool OsrWindowWin::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                  int viewX,
                                  int viewY,
                                  int& screenX,
                                  int& screenY) {
  CEF_REQUIRE_UI_THREAD();

  if (!::IsWindow(hwnd_))
    return false;

  // Convert the point from view coordinates to actual screen coordinates.
  POINT screen_pt = {
      LogicalToDevice(viewX, device_scale_factor_),
      LogicalToDevice(viewY, device_scale_factor_)
  };
  ClientToScreen(hwnd_, &screen_pt);
  screenX = screen_pt.x;
  screenY = screen_pt.y;
  return true;
}

bool OsrWindowWin::GetScreenInfo(CefRefPtr<CefBrowser> browser,
                                 CefScreenInfo& screen_info) {
  CEF_REQUIRE_UI_THREAD();

  if (!::IsWindow(hwnd_))
    return false;

  CefRect view_rect;
  GetViewRect(browser, view_rect);

  screen_info.device_scale_factor = device_scale_factor_;

  // The screen info rectangles are used by the renderer to create and position
  // popups. Keep popups inside the view rectangle.
  screen_info.rect = view_rect;
  screen_info.available_rect = view_rect;
  return true;
}

void OsrWindowWin::OnPopupShow(CefRefPtr<CefBrowser> browser,
                               bool show) {
  CEF_REQUIRE_UI_THREAD();

  if (!show) {
    renderer_.ClearPopupRects();
    browser->GetHost()->Invalidate(PET_VIEW);
  }
  renderer_.OnPopupShow(browser, show);
}

void OsrWindowWin::OnPopupSize(CefRefPtr<CefBrowser> browser,
                               const CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  renderer_.OnPopupSize(browser, LogicalToDevice(rect, device_scale_factor_));
}

void OsrWindowWin::OnPaint(CefRefPtr<CefBrowser> browser,
                           CefRenderHandler::PaintElementType type,
                           const CefRenderHandler::RectList& dirtyRects,
                           const void* buffer,
                           int width,
                           int height) {
  CEF_REQUIRE_UI_THREAD();

  if (painting_popup_) {
    renderer_.OnPaint(browser, type, dirtyRects, buffer, width, height);
    return;
  }
  if (!hdc_)
    EnableGL();

  ScopedGLContext scoped_gl_context(hdc_, hrc_, true);
  renderer_.OnPaint(browser, type, dirtyRects, buffer, width, height);
  if (type == PET_VIEW && !renderer_.popup_rect().IsEmpty()) {
    painting_popup_ = true;
    browser->GetHost()->Invalidate(PET_POPUP);
    painting_popup_ = false;
  }
  renderer_.Render();
}

void OsrWindowWin::OnCursorChange(
    CefRefPtr<CefBrowser> browser,
    CefCursorHandle cursor,
    CefRenderHandler::CursorType type,
    const CefCursorInfo& custom_cursor_info) {
  CEF_REQUIRE_UI_THREAD();

  if (!::IsWindow(hwnd_))
    return;

  // Change the plugin window's cursor.
  SetClassLongPtr(hwnd_, GCLP_HCURSOR,
                  static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
  SetCursor(cursor);
}

bool OsrWindowWin::StartDragging(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDragData> drag_data,
    CefRenderHandler::DragOperationsMask allowed_ops,
    int x, int y) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  if (!drop_target_)
    return false;

  current_drag_op_ = DRAG_OPERATION_NONE;
  CefBrowserHost::DragOperationsMask result =
      drop_target_->StartDragging(browser, drag_data, allowed_ops, x, y);
  current_drag_op_ = DRAG_OPERATION_NONE;
  POINT pt = {};
  GetCursorPos(&pt);
  ScreenToClient(hwnd_, &pt);

  browser->GetHost()->DragSourceEndedAt(
      DeviceToLogical(pt.x, device_scale_factor_),
      DeviceToLogical(pt.y, device_scale_factor_),
      result);
  browser->GetHost()->DragSourceSystemDragEnded();
  return true;
#else
  // Cancel the drag. The dragging implementation requires ATL support.
  return false;
#endif
}

void OsrWindowWin::UpdateDragCursor(
    CefRefPtr<CefBrowser> browser,
    CefRenderHandler::DragOperation operation) {
  CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
  current_drag_op_ = operation;
#endif
}

#if defined(CEF_USE_ATL)

CefBrowserHost::DragOperationsMask
OsrWindowWin::OnDragEnter(CefRefPtr<CefDragData> drag_data,
                          CefMouseEvent ev,
                          CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragEnter(drag_data, ev, effect);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
  }
  return current_drag_op_;
}

CefBrowserHost::DragOperationsMask
OsrWindowWin::OnDragOver(CefMouseEvent ev,
                         CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
  }
  return current_drag_op_;
}

void OsrWindowWin::OnDragLeave() {
  if (browser_)
    browser_->GetHost()->DragTargetDragLeave();
}

CefBrowserHost::DragOperationsMask
OsrWindowWin::OnDrop(CefMouseEvent ev,
                     CefBrowserHost::DragOperationsMask effect) {
  if (browser_) {
    DeviceToLogical(ev, device_scale_factor_);
    browser_->GetHost()->DragTargetDragOver(ev, effect);
    browser_->GetHost()->DragTargetDrop(ev);
  }
  return current_drag_op_;
}

#endif  // defined(CEF_USE_ATL)

}  // namespace client
