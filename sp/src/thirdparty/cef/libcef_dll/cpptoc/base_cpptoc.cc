// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "libcef_dll/cpptoc/base_cpptoc.h"

CefBaseCppToC::CefBaseCppToC() {
}

template<> CefRefPtr<CefBase> CefCppToC<CefBaseCppToC, CefBase, cef_base_t>::
    UnwrapDerived(CefWrapperType type, cef_base_t* s) {
  NOTREACHED();
  return NULL;
}

#if DCHECK_IS_ON()
template<> base::AtomicRefCount CefCppToC<CefBaseCppToC, CefBase,
    cef_base_t>::DebugObjCt = 0;
#endif

template<> CefWrapperType CefCppToC<CefBaseCppToC, CefBase,cef_base_t>::
    kWrapperType = WT_BASE;
