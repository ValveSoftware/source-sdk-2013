# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/element_proxy.hpp"

code = """// Header file element_proxy.hpp
//
// Proxy objects for invidivual elements in a container wrapped by
// container_proxy
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 9/15   rmg     File creation from container_proxy.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: element_proxy.hpp,v 1.1.2.10 2003/12/03 12:18:51 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_ELEMENT_PROXY_HPP
#define BOOST_PYTHON_INDEXING_ELEMENT_PROXY_HPP

#include <indexing_suite/shared_proxy_impl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/get_pointer.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/detail/workaround.hpp>

namespace boost { namespace python { namespace indexing {
  template<typename ContainerProxy>
  class element_proxy
  {
  public:
    typedef typename ContainerProxy::shared_proxy proxy_type;
    typedef typename ContainerProxy::raw_value_type raw_value_type;
    typedef typename ContainerProxy::size_type size_type;
    typedef boost::shared_ptr<proxy_type> proxy_pointer;

    typedef typename proxy_type::value_type value_type;
    typedef typename proxy_type::reference reference;
    typedef typename proxy_type::pointer pointer;
    typedef typename proxy_type::iterator_category iterator_category;
    typedef typename proxy_type::difference_type difference_type;

    typedef value_type element_type;      // Alias for register_ptr_to_python

    element_proxy () : m_ptr () { }
    explicit element_proxy (proxy_type *ptr) : m_ptr (ptr) { }
    explicit element_proxy (proxy_pointer const &ptr) : m_ptr (ptr) { }

    explicit element_proxy (raw_value_type const &val)
      : m_ptr (new proxy_type(val))
    {
      // Create new standalone value (i.e. detached)
    }

    reference operator* () const { return m_ptr->operator*(); }
    pointer operator-> () const { return (*m_ptr).operator->(); }
    pointer get () const { return operator->(); }  // Alias for pointer_holder

    // Implicit conversion to raw_value_type
    operator reference () const { return operator*(); }

#if BOOST_WORKAROUND (BOOST_MSVC, <= 1200)
    // The implicit conversion doesn't work on MSVC6, so help it along
    // a little.
    bool operator== (value_type const &val) const { return (**this) == val; }
    bool operator!= (value_type const &val) const { return (**this) != val; }
    bool operator< (value_type const &val) const { return (**this) < val; }
    bool operator> (value_type const &val) const { return (**this) > val; }
#endif

    element_proxy &operator= (value_type const &copy)
    {
      proxy_type &proxy = *m_ptr;
      ContainerProxy *container = proxy.owner();
      size_type index = proxy.index();

      if (container)
        {
          container->replace (index, copy);
          // Proxy was attached before, but is now detached. Make sure
          // we now refer to the new element, instead of the detached
          // copy of the old element
          m_ptr = container->at (index).m_ptr;

          // Note: in the special case that this we and the container
          // proxy itself have the only references to the
          // shared_proxy_impl, it is not necessary to first detach
          // the proxy. Maybe best to implement when changing to
          // intrusive_ptr instead of shared_ptr.
        }

      else
        {
          *proxy = copy;
        }

      return *this;
    }

    element_proxy &operator= (element_proxy const &copy)
    {
      // Required to make sorting a container_proxy work
      return (*this) = *copy;
    }

    size_t use_count() const { return m_ptr.use_count(); } // For debugging

  private:
    proxy_pointer m_ptr;
  };

  template<typename ContainerProxy>
  class const_element_proxy
  {
  public:
    typedef element_proxy<ContainerProxy> base_type;
    typedef typename base_type::proxy_type proxy_type;
    typedef typename base_type::proxy_pointer proxy_pointer;
    typedef typename base_type::raw_value_type raw_value_type;

    typedef typename proxy_type::value_type const value_type;
    typedef value_type &reference;
    typedef value_type *pointer;
    typedef typename base_type::iterator_category iterator_category;
    typedef typename base_type::difference_type difference_type;

    const_element_proxy () : m_base () { }
    explicit const_element_proxy (proxy_type *ptr) : m_base (ptr) { }

    explicit const_element_proxy (proxy_pointer const &ptr) : m_base (ptr) {}

    const_element_proxy (const_element_proxy<ContainerProxy> const &copy)
      : m_base (copy.m_base)
    {
    }

    const_element_proxy (element_proxy<ContainerProxy> const &copy)
      : m_base (copy)
    {
    }

    explicit const_element_proxy (raw_value_type const &val)
      : m_base (new proxy_type(val))
    {
    }

    reference operator* () const { return *m_base; }
    pointer operator-> () const { return m_base.operator->(); }

    // Implicit conversion to raw_value_type const &
    operator reference () const { return *m_base; }

#if BOOST_WORKAROUND (BOOST_MSVC, <= 1200)
    // The implicit conversion doesn't work on MSVC6, so help it along
    // a little.
    bool operator== (value_type const &val) const { return m_base == val; }
    bool operator!= (value_type const &val) const { return m_base != val; }
    bool operator< (value_type const &val) const { return m_base < val; }
    bool operator> (value_type const &val) const { return m_base > val; }
#endif

    size_t use_count() const { return m_base.use_count(); } // For debugging

  private:
    base_type m_base;
  };

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
}} // namespace python::indexing
#endif

  template<typename ContainerProxy>
  typename ContainerProxy::raw_value_type *
  get_pointer (python::indexing::element_proxy<ContainerProxy> const &proxy)
  {
    return ::boost::addressof (*proxy);
  }

#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
    // Don't hide these other get_pointer overloads
    using boost::get_pointer;
}} // namespace python::indexing
#endif
}

#endif // BOOST_PYTHON_INDEXING_ELEMENT_PROXY_HPP


"""
