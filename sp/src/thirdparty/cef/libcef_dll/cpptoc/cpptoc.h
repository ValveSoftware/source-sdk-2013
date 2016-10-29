// Copyright (c) 2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_LIBCEF_DLL_CPPTOC_CPPTOC_H_
#define CEF_LIBCEF_DLL_CPPTOC_CPPTOC_H_
#pragma once

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/cef_base.h"
#include "include/capi/cef_base_capi.h"
#include "libcef_dll/wrapper_types.h"

// Wrap a C++ class with a C structure.  This is used when the class
// implementation exists on this side of the DLL boundary but will have methods
// called from the other side of the DLL boundary.
template <class ClassName, class BaseName, class StructName>
class CefCppToC : public CefBase {
 public:
  // Create a new wrapper instance and associated structure reference for
  // passing an object instance the other side.
  static StructName* Wrap(CefRefPtr<BaseName> c) {
    if (!c.get())
      return NULL;

    // Wrap our object with the CefCppToC class.
    ClassName* wrapper = new ClassName();
    wrapper->wrapper_struct_.object_ = c.get();
    // Add a reference to our wrapper object that will be released once our
    // structure arrives on the other side.
    wrapper->AddRef();
    // Return the structure pointer that can now be passed to the other side.
    return wrapper->GetStruct();
  }

  // Retrieve the underlying object instance for a structure reference passed
  // back from the other side.
  static CefRefPtr<BaseName> Unwrap(StructName* s) {
    if (!s)
      return NULL;

    // Cast our structure to the wrapper structure type.
    WrapperStruct* wrapperStruct = GetWrapperStruct(s);

    // If the type does not match this object then we need to unwrap as the
    // derived type.
    if (wrapperStruct->type_ != kWrapperType)
      return UnwrapDerived(wrapperStruct->type_, s);

    // Add the underlying object instance to a smart pointer.
    CefRefPtr<BaseName> objectPtr(wrapperStruct->object_);
    // Release the reference to our wrapper object that was added before the
    // structure was passed back to us.
    wrapperStruct->wrapper_->Release();
    // Return the underlying object instance.
    return objectPtr;
  }

  // Retrieve the underlying object instance from our own structure reference
  // when the reference is passed as the required first parameter of a C API
  // function call. No explicit reference counting is done in this case.
  static CefRefPtr<BaseName> Get(StructName* s) {
    DCHECK(s);
    WrapperStruct* wrapperStruct = GetWrapperStruct(s);
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);
    return wrapperStruct->object_;
  }

  // If returning the structure across the DLL boundary you should call
  // AddRef() on this CefCppToC object.  On the other side of the DLL boundary,
  // call UnderlyingRelease() on the wrapping CefCToCpp object.
  StructName* GetStruct() { return &wrapper_struct_.struct_; }

  // CefBase methods increment/decrement reference counts on both this object
  // and the underlying wrapper class.
  void AddRef() const {
    UnderlyingAddRef();
    ref_count_.AddRef();
  }
  bool Release() const {
    UnderlyingRelease();
    if (ref_count_.Release()) {
      delete this;
      return true;
    }
    return false;
  }
  bool HasOneRef() const { return UnderlyingHasOneRef(); }

#if DCHECK_IS_ON()
  // Simple tracking of allocated objects.
  static base::AtomicRefCount DebugObjCt;  // NOLINT(runtime/int)
#endif

 protected:
  CefCppToC() {
    wrapper_struct_.type_ = kWrapperType;
    wrapper_struct_.wrapper_ = this;
    memset(GetStruct(), 0, sizeof(StructName));

    cef_base_t* base = reinterpret_cast<cef_base_t*>(GetStruct());
    base->size = sizeof(StructName);
    base->add_ref = struct_add_ref;
    base->release = struct_release;
    base->has_one_ref = struct_has_one_ref;

#if DCHECK_IS_ON()
    base::AtomicRefCountInc(&DebugObjCt);
#endif
  }

  virtual ~CefCppToC() {
#if DCHECK_IS_ON()
    base::AtomicRefCountDec(&DebugObjCt);
#endif
  }

 private:
  // Used to associate this wrapper object, the underlying object instance and
  // the structure that will be passed to the other side.
  struct WrapperStruct {
    CefWrapperType type_;
    BaseName* object_;
    CefCppToC<ClassName, BaseName, StructName>* wrapper_;
    StructName struct_;
  };

  static WrapperStruct* GetWrapperStruct(StructName* s) {
    // Offset using the WrapperStruct size instead of individual member sizes
    // to avoid problems due to platform/compiler differences in structure
    // padding.
    return reinterpret_cast<WrapperStruct*>(
        reinterpret_cast<char*>(s) -
        (sizeof(WrapperStruct) - sizeof(StructName)));
  }

  // Unwrap as the derived type.
  static CefRefPtr<BaseName> UnwrapDerived(CefWrapperType type, StructName* s);

  // Increment/decrement reference counts on only the underlying class.
  void UnderlyingAddRef() const {
    wrapper_struct_.object_->AddRef();
  }
  bool UnderlyingRelease() const {
    return wrapper_struct_.object_->Release();
  }
  bool UnderlyingHasOneRef() const {
    return wrapper_struct_.object_->HasOneRef();
  }

  static void CEF_CALLBACK struct_add_ref(cef_base_t* base) {
    DCHECK(base);
    if (!base)
      return;

    WrapperStruct* wrapperStruct =
        GetWrapperStruct(reinterpret_cast<StructName*>(base));
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);

    wrapperStruct->wrapper_->AddRef();
  }

  static int CEF_CALLBACK struct_release(cef_base_t* base) {
    DCHECK(base);
    if (!base)
      return 0;

    WrapperStruct* wrapperStruct =
        GetWrapperStruct(reinterpret_cast<StructName*>(base));
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);

    return wrapperStruct->wrapper_->Release();
  }

  static int CEF_CALLBACK struct_has_one_ref(cef_base_t* base) {
    DCHECK(base);
    if (!base)
      return 0;

    WrapperStruct* wrapperStruct =
        GetWrapperStruct(reinterpret_cast<StructName*>(base));
    // Verify that the wrapper offset was calculated correctly.
    DCHECK_EQ(kWrapperType, wrapperStruct->type_);

    return wrapperStruct->wrapper_->HasOneRef();
  }

  WrapperStruct wrapper_struct_;
  CefRefCount ref_count_;

  static CefWrapperType kWrapperType;

  DISALLOW_COPY_AND_ASSIGN(CefCppToC);
};

#endif  // CEF_LIBCEF_DLL_CPPTOC_CPPTOC_H_
