// Copyright (c) 2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_LIBCEF_DLL_CTOCPP_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_CTOCPP_H_
#pragma once

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/cef_base.h"
#include "include/capi/cef_base_capi.h"
#include "libcef_dll/wrapper_types.h"

// Wrap a C structure with a C++ class. This is used when the implementation
// exists on the other side of the DLL boundary but will have methods called on
// this side of the DLL boundary.
template <class ClassName, class BaseName, class StructName>
class CefCToCpp : public BaseName {
 public:
  // Create a new wrapper instance for a structure reference received from the
  // other side.
  static CefRefPtr<BaseName> Wrap(StructName* s);

  // Retrieve the underlying structure reference from a wrapper instance for
  // return back to the other side.
  static StructName* Unwrap(CefRefPtr<BaseName> c);

  // If returning the structure across the DLL boundary you should call
  // UnderlyingAddRef() on this wrapping CefCToCpp object.  On the other side of
  // the DLL boundary, call Release() on the CefCppToC object.
  StructName* GetStruct() const {
    WrapperStruct* wrapperStruct = GetWrapperStruct(this);
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);
    return wrapperStruct->struct_;
  }

  // CefBase methods increment/decrement reference counts on both this object
  // and the underlying wrapped structure.
  void AddRef() const {
    UnderlyingAddRef();
    ref_count_.AddRef();
  }
  bool Release() const;
  bool HasOneRef() const { return UnderlyingHasOneRef(); }

#if DCHECK_IS_ON()
  // Simple tracking of allocated objects.
  static base::AtomicRefCount DebugObjCt;  // NOLINT(runtime/int)
#endif

 protected:
  CefCToCpp() {
#if DCHECK_IS_ON()
    base::AtomicRefCountInc(&DebugObjCt);
#endif
  }

  virtual ~CefCToCpp() {
#if DCHECK_IS_ON()
    base::AtomicRefCountDec(&DebugObjCt);
#endif
  }

 private:
  // Used to associate this wrapper object and the structure reference received
  // from the other side.
  struct WrapperStruct;

  static WrapperStruct* GetWrapperStruct(const BaseName* obj);

  // Unwrap as the derived type.
  static StructName* UnwrapDerived(CefWrapperType type, BaseName* c);

  // Increment/decrement reference counts on only the underlying class.
  void UnderlyingAddRef() const {
    cef_base_t* base = reinterpret_cast<cef_base_t*>(GetStruct());
    if (base->add_ref)
      base->add_ref(base);
  }

  bool UnderlyingRelease() const {
    cef_base_t* base = reinterpret_cast<cef_base_t*>(GetStruct());
    if (!base->release)
      return false;
    return base->release(base) ? true : false;
  }

  bool UnderlyingHasOneRef() const {
    cef_base_t* base = reinterpret_cast<cef_base_t*>(GetStruct());
    if (!base->has_one_ref)
      return false;
    return base->has_one_ref(base) ? true : false;
  }

  CefRefCount ref_count_;

  static CefWrapperType kWrapperType;

  DISALLOW_COPY_AND_ASSIGN(CefCToCpp);
};

template <class ClassName, class BaseName, class StructName>
struct CefCToCpp<ClassName,BaseName,StructName>::WrapperStruct {
  CefWrapperType type_;
  StructName* struct_;
  ClassName wrapper_;
};

template <class ClassName, class BaseName, class StructName>
CefRefPtr<BaseName>
    CefCToCpp<ClassName, BaseName, StructName>::Wrap(StructName* s) {
  if (!s)
    return NULL;

  // Wrap their structure with the CefCToCpp object.
  WrapperStruct* wrapperStruct = new WrapperStruct;
  wrapperStruct->type_ = kWrapperType;
  wrapperStruct->struct_ = s;

  // Put the wrapper object in a smart pointer.
  CefRefPtr<BaseName> wrapperPtr(&wrapperStruct->wrapper_);
  // Release the reference that was added to the CefCppToC wrapper object on
  // the other side before their structure was passed to us.
  wrapperStruct->wrapper_.UnderlyingRelease();
  // Return the smart pointer.
  return wrapperPtr;
}

template <class ClassName, class BaseName, class StructName>
StructName*
    CefCToCpp<ClassName, BaseName, StructName>::Unwrap(CefRefPtr<BaseName> c) {
  if (!c.get())
    return NULL;

  WrapperStruct* wrapperStruct = GetWrapperStruct(c.get());

  // If the type does not match this object then we need to unwrap as the
  // derived type.
  if (wrapperStruct->type_ != kWrapperType)
    return UnwrapDerived(wrapperStruct->type_, c.get());

  // Add a reference to the CefCppToC wrapper object on the other side that
  // will be released once the structure is received.
  wrapperStruct->wrapper_.UnderlyingAddRef();
  // Return their original structure.
  return wrapperStruct->struct_;
}

template <class ClassName, class BaseName, class StructName>
bool CefCToCpp<ClassName, BaseName, StructName>::Release() const {
  UnderlyingRelease();
  if (ref_count_.Release()) {
    WrapperStruct* wrapperStruct = GetWrapperStruct(this);
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);
    delete wrapperStruct;
    return true;
  }
  return false;
}

template <class ClassName, class BaseName, class StructName>
typename CefCToCpp<ClassName, BaseName, StructName>::WrapperStruct*
    CefCToCpp<ClassName, BaseName, StructName>::GetWrapperStruct(
        const BaseName* obj) {
  // Offset using the WrapperStruct size instead of individual member sizes to
  // avoid problems due to platform/compiler differences in structure padding.
  return reinterpret_cast<WrapperStruct*>(
      reinterpret_cast<char*>(const_cast<BaseName*>(obj)) -
      (sizeof(WrapperStruct) - sizeof(ClassName)));
}

#endif  // CEF_LIBCEF_DLL_CTOCPP_CTOCPP_H_
