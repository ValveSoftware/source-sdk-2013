// Copyright (c) 2014 Marshall A. Greenblatt. Portions copyright (c) 2013
// Google Inc. All rights reserved.
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

#ifndef CEF_INCLUDE_BASE_CEF_CALLBACK_LIST_H_
#define CEF_INCLUDE_BASE_CEF_CALLBACK_LIST_H_
#pragma once

#if defined(BASE_CALLBACK_LIST_H_)
// Do nothing if the Chromium header has already been included.
// This can happen in cases where Chromium code is used directly by the
// client application. When using Chromium code directly always include
// the Chromium header first to avoid type conflicts.
#elif defined(USING_CHROMIUM_INCLUDES)
// When building CEF include the Chromium header directly.
#include "base/callback_list.h"
#else  // !USING_CHROMIUM_INCLUDES
// The following is substantially similar to the Chromium implementation.
// If the Chromium implementation diverges the below implementation should be
// updated to match.

#include <list>

#include "include/base/cef_basictypes.h"
#include "include/base/cef_callback.h"
#include "include/base/internal/cef_callback_internal.h"
#include "include/base/cef_build.h"
#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/base/cef_scoped_ptr.h"

// OVERVIEW:
//
// A container for a list of callbacks.  Unlike a normal STL vector or list,
// this container can be modified during iteration without invalidating the
// iterator. It safely handles the case of a callback removing itself
// or another callback from the list while callbacks are being run.
//
// TYPICAL USAGE:
//
// class MyWidget {
//  public:
//   ...
//
//   typedef base::Callback<void(const Foo&)> OnFooCallback;
//
//   scoped_ptr<base::CallbackList<void(const Foo&)>::Subscription>
//   RegisterCallback(const OnFooCallback& cb) {
//     return callback_list_.Add(cb);
//   }
//
//  private:
//   void NotifyFoo(const Foo& foo) {
//      callback_list_.Notify(foo);
//   }
//
//   base::CallbackList<void(const Foo&)> callback_list_;
//
//   DISALLOW_COPY_AND_ASSIGN(MyWidget);
// };
//
//
// class MyWidgetListener {
//  public:
//   MyWidgetListener::MyWidgetListener() {
//     foo_subscription_ = MyWidget::GetCurrent()->RegisterCallback(
//             base::Bind(&MyWidgetListener::OnFoo, this)));
//   }
//
//   MyWidgetListener::~MyWidgetListener() {
//      // Subscription gets deleted automatically and will deregister
//      // the callback in the process.
//   }
//
//  private:
//   void OnFoo(const Foo& foo) {
//     // Do something.
//   }
//
//   scoped_ptr<base::CallbackList<void(const Foo&)>::Subscription>
//       foo_subscription_;
//
//   DISALLOW_COPY_AND_ASSIGN(MyWidgetListener);
// };

namespace base {

namespace cef_internal {

template <typename CallbackType>
class CallbackListBase {
 public:
  class Subscription {
   public:
    Subscription(CallbackListBase<CallbackType>* list,
                 typename std::list<CallbackType>::iterator iter)
        : list_(list),
          iter_(iter) {
    }

    ~Subscription() {
      if (list_->active_iterator_count_) {
        iter_->Reset();
      } else {
        list_->callbacks_.erase(iter_);
        if (!list_->removal_callback_.is_null())
          list_->removal_callback_.Run();
      }
    }

   private:
    CallbackListBase<CallbackType>* list_;
    typename std::list<CallbackType>::iterator iter_;

    DISALLOW_COPY_AND_ASSIGN(Subscription);
  };

  // Add a callback to the list. The callback will remain registered until the
  // returned Subscription is destroyed, which must occur before the
  // CallbackList is destroyed.
  scoped_ptr<Subscription> Add(const CallbackType& cb) WARN_UNUSED_RESULT {
    DCHECK(!cb.is_null());
    return scoped_ptr<Subscription>(
        new Subscription(this, callbacks_.insert(callbacks_.end(), cb)));
  }

  // Sets a callback which will be run when a subscription list is changed.
  void set_removal_callback(const Closure& callback) {
    removal_callback_ = callback;
  }

  // Returns true if there are no subscriptions. This is only valid to call when
  // not looping through the list.
  bool empty() {
    DCHECK_EQ(0, active_iterator_count_);
    return callbacks_.empty();
  }

 protected:
  // An iterator class that can be used to access the list of callbacks.
  class Iterator {
   public:
    explicit Iterator(CallbackListBase<CallbackType>* list)
        : list_(list),
          list_iter_(list_->callbacks_.begin()) {
      ++list_->active_iterator_count_;
    }

    Iterator(const Iterator& iter)
        : list_(iter.list_),
          list_iter_(iter.list_iter_) {
      ++list_->active_iterator_count_;
    }

    ~Iterator() {
      if (list_ && --list_->active_iterator_count_ == 0) {
        list_->Compact();
      }
    }

    CallbackType* GetNext() {
      while ((list_iter_ != list_->callbacks_.end()) && list_iter_->is_null())
        ++list_iter_;

      CallbackType* cb = NULL;
      if (list_iter_ != list_->callbacks_.end()) {
        cb = &(*list_iter_);
        ++list_iter_;
      }
      return cb;
    }

   private:
    CallbackListBase<CallbackType>* list_;
    typename std::list<CallbackType>::iterator list_iter_;
  };

  CallbackListBase() : active_iterator_count_(0) {}

  ~CallbackListBase() {
    DCHECK_EQ(0, active_iterator_count_);
    DCHECK_EQ(0U, callbacks_.size());
  }

  // Returns an instance of a CallbackListBase::Iterator which can be used
  // to run callbacks.
  Iterator GetIterator() {
    return Iterator(this);
  }

  // Compact the list: remove any entries which were NULLed out during
  // iteration.
  void Compact() {
    typename std::list<CallbackType>::iterator it = callbacks_.begin();
    bool updated = false;
    while (it != callbacks_.end()) {
      if ((*it).is_null()) {
        updated = true;
        it = callbacks_.erase(it);
      } else {
        ++it;
      }

      if (updated && !removal_callback_.is_null())
        removal_callback_.Run();
    }
  }

 private:
  std::list<CallbackType> callbacks_;
  int active_iterator_count_;
  Closure removal_callback_;

  DISALLOW_COPY_AND_ASSIGN(CallbackListBase);
};

}  // namespace cef_internal

template <typename Sig> class CallbackList;

template <>
class CallbackList<void(void)>
    : public cef_internal::CallbackListBase<Callback<void(void)> > {
 public:
  typedef Callback<void(void)> CallbackType;

  CallbackList() {}

  void Notify() {
    cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run();
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1>
class CallbackList<void(A1)>
    : public cef_internal::CallbackListBase<Callback<void(A1)> > {
 public:
  typedef Callback<void(A1)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2>
class CallbackList<void(A1, A2)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2)> > {
 public:
  typedef Callback<void(A1, A2)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2, typename A3>
class CallbackList<void(A1, A2, A3)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2, A3)> > {
 public:
  typedef Callback<void(A1, A2, A3)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2,
              typename cef_internal::CallbackParamTraits<A3>::ForwardType a3) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2, typename A3, typename A4>
class CallbackList<void(A1, A2, A3, A4)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2, A3, A4)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2,
              typename cef_internal::CallbackParamTraits<A3>::ForwardType a3,
              typename cef_internal::CallbackParamTraits<A4>::ForwardType a4) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
class CallbackList<void(A1, A2, A3, A4, A5)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2, A3, A4, A5)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2,
              typename cef_internal::CallbackParamTraits<A3>::ForwardType a3,
              typename cef_internal::CallbackParamTraits<A4>::ForwardType a4,
              typename cef_internal::CallbackParamTraits<A5>::ForwardType a5) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6>
class CallbackList<void(A1, A2, A3, A4, A5, A6)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2, A3, A4, A5,
        A6)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5, A6)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2,
              typename cef_internal::CallbackParamTraits<A3>::ForwardType a3,
              typename cef_internal::CallbackParamTraits<A4>::ForwardType a4,
              typename cef_internal::CallbackParamTraits<A5>::ForwardType a5,
              typename cef_internal::CallbackParamTraits<A6>::ForwardType a6) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5, a6);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7>
class CallbackList<void(A1, A2, A3, A4, A5, A6, A7)>
    : public cef_internal::CallbackListBase<Callback<void(A1, A2, A3, A4, A5, A6,
        A7)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5, A6, A7)> CallbackType;

  CallbackList() {}

  void Notify(typename cef_internal::CallbackParamTraits<A1>::ForwardType a1,
              typename cef_internal::CallbackParamTraits<A2>::ForwardType a2,
              typename cef_internal::CallbackParamTraits<A3>::ForwardType a3,
              typename cef_internal::CallbackParamTraits<A4>::ForwardType a4,
              typename cef_internal::CallbackParamTraits<A5>::ForwardType a5,
              typename cef_internal::CallbackParamTraits<A6>::ForwardType a6,
              typename cef_internal::CallbackParamTraits<A7>::ForwardType a7) {
    typename cef_internal::CallbackListBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while ((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5, a6, a7);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackList);
};

}  // namespace base

#endif  // !USING_CHROMIUM_INCLUDES

#endif  // CEF_INCLUDE_BASE_CEF_CALLBACK_LIST_H_
