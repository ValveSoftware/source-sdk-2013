// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#include "cefclient/browser/osr_renderer.h"

#if defined(OS_WIN)
#include <gl/gl.h>
#elif defined(OS_MACOSX)
#include <OpenGL/gl.h>
#elif defined(OS_LINUX)
#include <GL/gl.h>
#else
#error Platform is not supported.
#endif

#include "include/base/cef_logging.h"
#include "include/wrapper/cef_helpers.h"

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

// DCHECK on gl errors.
#if DCHECK_IS_ON()
#define VERIFY_NO_ERROR { \
    int _gl_error = glGetError(); \
    DCHECK(_gl_error == GL_NO_ERROR) << \
    "glGetError returned " << _gl_error; \
  }
#else
#define VERIFY_NO_ERROR
#endif

namespace client {

OsrRenderer::Settings::Settings()
    : transparent(false),
      show_update_rect(false),
      background_color(CefColorSetARGB(255, 255, 255, 255)) {
}

OsrRenderer::OsrRenderer(const Settings& settings)
    : settings_(settings),
      initialized_(false),
      texture_id_(0),
      view_width_(0),
      view_height_(0),
      spin_x_(0),
      spin_y_(0) {
}

OsrRenderer::~OsrRenderer() {
  Cleanup();
}

void OsrRenderer::Initialize() {
  if (initialized_)
    return;

  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST); VERIFY_NO_ERROR;

  glClearColor(float(CefColorGetR(settings_.background_color)) / 255.0f,
               float(CefColorGetG(settings_.background_color)) / 255.0f,
               float(CefColorGetB(settings_.background_color)) / 255.0f,
               1.0f); VERIFY_NO_ERROR;

  // Necessary for non-power-of-2 textures to render correctly.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); VERIFY_NO_ERROR;

  // Create the texture.
  glGenTextures(1, &texture_id_); VERIFY_NO_ERROR;
  DCHECK_NE(texture_id_, 0U); VERIFY_NO_ERROR;

  glBindTexture(GL_TEXTURE_2D, texture_id_); VERIFY_NO_ERROR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST); VERIFY_NO_ERROR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_NEAREST); VERIFY_NO_ERROR;
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); VERIFY_NO_ERROR;

  initialized_ = true;
}

void OsrRenderer::Cleanup() {
  if (texture_id_ != 0)
    glDeleteTextures(1, &texture_id_);
}

void OsrRenderer::Render() {
  if (view_width_ == 0 || view_height_ == 0)
    return;

  DCHECK(initialized_);

  struct {
    float tu, tv;
    float x, y, z;
  } static vertices[] = {
    {0.0f, 1.0f, -1.0f, -1.0f, 0.0f},
    {1.0f, 1.0f,  1.0f, -1.0f, 0.0f},
    {1.0f, 0.0f,  1.0f,  1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f,  1.0f, 0.0f}
  };

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); VERIFY_NO_ERROR;

  glMatrixMode(GL_MODELVIEW); VERIFY_NO_ERROR;
  glLoadIdentity(); VERIFY_NO_ERROR;

  // Match GL units to screen coordinates.
  glViewport(0, 0, view_width_, view_height_); VERIFY_NO_ERROR;
  glMatrixMode(GL_PROJECTION); VERIFY_NO_ERROR;
  glLoadIdentity(); VERIFY_NO_ERROR;

  // Draw the background gradient.
  glPushAttrib(GL_ALL_ATTRIB_BITS); VERIFY_NO_ERROR;
  // Don't check for errors until glEnd().
  glBegin(GL_QUADS);
  glColor4f(1.0, 0.0, 0.0, 1.0);  // red
  glVertex2f(-1.0, -1.0);
  glVertex2f(1.0, -1.0);
  glColor4f(0.0, 0.0, 1.0, 1.0);  // blue
  glVertex2f(1.0, 1.0);
  glVertex2f(-1.0, 1.0);
  glEnd(); VERIFY_NO_ERROR;
  glPopAttrib(); VERIFY_NO_ERROR;

  // Rotate the view based on the mouse spin.
  if (spin_x_ != 0) {
    glRotatef(-spin_x_, 1.0f, 0.0f, 0.0f); VERIFY_NO_ERROR;
  }
  if (spin_y_ != 0) {
    glRotatef(-spin_y_, 0.0f, 1.0f, 0.0f); VERIFY_NO_ERROR;
  }

  if (settings_.transparent) {
    // Alpha blending style. Texture values have premultiplied alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); VERIFY_NO_ERROR;

    // Enable alpha blending.
    glEnable(GL_BLEND); VERIFY_NO_ERROR;
  }

  // Enable 2D textures.
  glEnable(GL_TEXTURE_2D); VERIFY_NO_ERROR;

  // Draw the facets with the texture.
  DCHECK_NE(texture_id_, 0U); VERIFY_NO_ERROR;
  glBindTexture(GL_TEXTURE_2D, texture_id_); VERIFY_NO_ERROR;
  glInterleavedArrays(GL_T2F_V3F, 0, vertices); VERIFY_NO_ERROR;
  glDrawArrays(GL_QUADS, 0, 4); VERIFY_NO_ERROR;

  // Disable 2D textures.
  glDisable(GL_TEXTURE_2D); VERIFY_NO_ERROR;

  if (settings_.transparent) {
    // Disable alpha blending.
    glDisable(GL_BLEND); VERIFY_NO_ERROR;
  }

  // Draw a rectangle around the update region.
  if (settings_.show_update_rect && !update_rect_.IsEmpty()) {
    int left = update_rect_.x;
    int right = update_rect_.x + update_rect_.width;
    int top = update_rect_.y;
    int bottom = update_rect_.y + update_rect_.height;

#if defined(OS_LINUX)
    // Shrink the box so that top & right sides are drawn.
    top += 1;
    right -= 1;
#else
    // Shrink the box so that left & bottom sides are drawn.
    left += 1;
    bottom -= 1;
#endif

    glPushAttrib(GL_ALL_ATTRIB_BITS); VERIFY_NO_ERROR
    glMatrixMode(GL_PROJECTION); VERIFY_NO_ERROR;
    glPushMatrix(); VERIFY_NO_ERROR;
    glLoadIdentity(); VERIFY_NO_ERROR;
    glOrtho(0, view_width_, view_height_, 0, 0, 1); VERIFY_NO_ERROR;

    glLineWidth(1); VERIFY_NO_ERROR;
    glColor3f(1.0f, 0.0f, 0.0f); VERIFY_NO_ERROR;
    // Don't check for errors until glEnd().
    glBegin(GL_LINE_STRIP);
     glVertex2i(left, top);
    glVertex2i(right, top);
    glVertex2i(right, bottom);
    glVertex2i(left, bottom);
    glVertex2i(left, top);
    glEnd(); VERIFY_NO_ERROR;

    glPopMatrix(); VERIFY_NO_ERROR;
    glPopAttrib(); VERIFY_NO_ERROR;
  }
}

void OsrRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser,
                              bool show) {
  if (!show) {
    // Clear the popup rectangle.
    ClearPopupRects();
  }
}

void OsrRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser,
                              const CefRect& rect) {
  if (rect.width <= 0 || rect.height <= 0)
    return;
  original_popup_rect_ = rect;
  popup_rect_ = GetPopupRectInWebView(original_popup_rect_);
}

CefRect OsrRenderer::GetPopupRectInWebView(const CefRect& original_rect) {
  CefRect rc(original_rect);
  // if x or y are negative, move them to 0.
  if (rc.x < 0)
    rc.x = 0;
  if (rc.y < 0)
    rc.y = 0;
  // if popup goes outside the view, try to reposition origin
  if (rc.x + rc.width > view_width_)
    rc.x = view_width_ - rc.width;
  if (rc.y + rc.height > view_height_)
    rc.y = view_height_ - rc.height;
  // if x or y became negative, move them to 0 again.
  if (rc.x < 0)
    rc.x = 0;
  if (rc.y < 0)
    rc.y = 0;
  return rc;
}

void OsrRenderer::ClearPopupRects() {
  popup_rect_.Set(0, 0, 0, 0);
  original_popup_rect_.Set(0, 0, 0, 0);
}

void OsrRenderer::OnPaint(CefRefPtr<CefBrowser> browser,
                          CefRenderHandler::PaintElementType type,
                          const CefRenderHandler::RectList& dirtyRects,
                          const void* buffer, int width, int height) {
  if (!initialized_)
    Initialize();

  if (settings_.transparent) {
    // Enable alpha blending.
    glEnable(GL_BLEND); VERIFY_NO_ERROR;
  }

  // Enable 2D textures.
  glEnable(GL_TEXTURE_2D); VERIFY_NO_ERROR;

  DCHECK_NE(texture_id_, 0U);
  glBindTexture(GL_TEXTURE_2D, texture_id_); VERIFY_NO_ERROR;

  if (type == PET_VIEW) {
    int old_width = view_width_;
    int old_height = view_height_;

    view_width_ = width;
    view_height_ = height;

    if (settings_.show_update_rect)
      update_rect_ = dirtyRects[0];

    glPixelStorei(GL_UNPACK_ROW_LENGTH, view_width_); VERIFY_NO_ERROR;

    if (old_width != view_width_ || old_height != view_height_ ||
        (dirtyRects.size() == 1 &&
         dirtyRects[0] == CefRect(0, 0, view_width_, view_height_))) {
      // Update/resize the whole texture.
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); VERIFY_NO_ERROR;
      glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); VERIFY_NO_ERROR;
      glTexImage2D(
          GL_TEXTURE_2D, 0, GL_RGBA, view_width_, view_height_, 0,
          GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer); VERIFY_NO_ERROR;
    } else {
      // Update just the dirty rectangles.
      CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
      for (; i != dirtyRects.end(); ++i) {
        const CefRect& rect = *i;
        DCHECK(rect.x + rect.width <= view_width_);
        DCHECK(rect.y + rect.height <= view_height_);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect.x); VERIFY_NO_ERROR;
        glPixelStorei(GL_UNPACK_SKIP_ROWS, rect.y); VERIFY_NO_ERROR;
        glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width,
                        rect.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                        buffer); VERIFY_NO_ERROR;
      }
    }
  } else if (type == PET_POPUP && popup_rect_.width > 0 &&
             popup_rect_.height > 0) {
    int skip_pixels = 0, x = popup_rect_.x;
    int skip_rows = 0, y = popup_rect_.y;
    int w = width;
    int h = height;

    // Adjust the popup to fit inside the view.
    if (x < 0) {
      skip_pixels = -x;
      x = 0;
    }
    if (y < 0) {
      skip_rows = -y;
      y = 0;
    }
    if (x + w > view_width_)
      w -= x + w - view_width_;
    if (y + h > view_height_)
      h -= y + h - view_height_;

    // Update the popup rectangle.
    glPixelStorei(GL_UNPACK_ROW_LENGTH, width); VERIFY_NO_ERROR;
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels); VERIFY_NO_ERROR;
    glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows); VERIFY_NO_ERROR;
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_BGRA,
                    GL_UNSIGNED_INT_8_8_8_8_REV, buffer); VERIFY_NO_ERROR;
  }

  // Disable 2D textures.
  glDisable(GL_TEXTURE_2D); VERIFY_NO_ERROR;

  if (settings_.transparent) {
    // Disable alpha blending.
    glDisable(GL_BLEND); VERIFY_NO_ERROR;
  }
}

void OsrRenderer::SetSpin(float spinX, float spinY) {
  spin_x_ = spinX;
  spin_y_ = spinY;
}

void OsrRenderer::IncrementSpin(float spinDX, float spinDY) {
  spin_x_ -= spinDX;
  spin_y_ -= spinDY;
}

}  // namespace client
