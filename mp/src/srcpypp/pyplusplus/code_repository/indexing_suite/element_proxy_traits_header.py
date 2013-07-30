# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/element_proxy_traits.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file element_proxy_traits.hpp
//
// Note: element_proxy.hpp must be included before this header
//
// This is a separate header so that element_proxy.hpp is not
// dependant on register_ptr_to_python.hpp. This avoids a problem with
// two-phase name lookup, where register_ptr_to_python must be
// included *after* the element_proxy overload of boost::get_pointer
// is declared.
//
// History
// =======
// 2003/10/23   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: element_proxy_traits.hpp,v 1.1.2.5 2003/12/05 17:36:14 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_ELEMENT_PROXY_TRAITS_HPP
#define BOOST_PYTHON_INDEXING_ELEMENT_PROXY_TRAITS_HPP

#include <indexing_suite/element_proxy.hpp>
#include <indexing_suite/value_traits.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/implicit.hpp>

namespace boost { namespace python { namespace indexing {
  template<typename ContainerProxy>
  struct element_proxy_traits
    : public value_traits<
          BOOST_DEDUCED_TYPENAME ContainerProxy::raw_value_type>
  {
    typedef element_proxy<ContainerProxy> element_proxy_type;
    typedef typename ContainerProxy::raw_value_type raw_value_type;
    typedef value_traits<raw_value_type> base_type;

    // Wrap the base class versions of the comparisons using
    // indirection
    struct less
      : std::binary_function<element_proxy_type, element_proxy_type, bool>
    {
      typename base_type::less m_base_compare;

      bool operator()(
          element_proxy_type const &p1, element_proxy_type const &p2) const
      {
        return m_base_compare (*p1, *p2);
      }
    };

    struct equal_to
      : std::binary_function<raw_value_type, element_proxy_type, bool>
    {
      // First param is raw_value_type to interface smoothly with the
      // bind1st used in default_algorithms::find

      typename base_type::equal_to m_base_compare;

      bool operator()(
          raw_value_type const &v, element_proxy_type const &p) const
      {
        return m_base_compare (v, *p);
      }
    };

    template<typename PythonClass, typename Policy>
    static void visit_container_class (PythonClass &, Policy const &)
    {
      register_ptr_to_python<element_proxy_type>();
      implicitly_convertible<raw_value_type, element_proxy_type>();
    }
  };

#if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  // value_traits partial specialization for element_proxy instances
  template<typename ContainerProxy>
  struct value_traits<element_proxy<ContainerProxy> >
    : element_proxy_traits<ContainerProxy>
  {
  };
#endif
} } }

#endif // BOOST_PYTHON_INDEXING_ELEMENT_PROXY_TRAITS_HPP


"""
