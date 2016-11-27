// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/resource_util.h"

#include <algorithm>
#include <string>

#include "include/base/cef_logging.h"

namespace client {

CefRefPtr<CefImage> LoadImageIcon(const char* icon_name) {
  CefRefPtr<CefImage> icon = CefImage::CreateImage();
  std::string image, resource;

  resource = std::string(icon_name) + ".1x.png";
  if (LoadBinaryResource(resource.c_str(), image))
    icon->AddPNG(1.0f, image.c_str(), image.size());

  resource = std::string(icon_name) + ".2x.png";
  if (LoadBinaryResource(resource.c_str(), image))
    icon->AddPNG(2.0f, image.c_str(), image.size());

  // Icons must be 16 in DIP.
  DCHECK_EQ(16U, std::max(icon->GetWidth(), icon->GetHeight()));

  return icon;
}

}  // namespace client
