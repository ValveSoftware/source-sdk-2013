# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/value_traits.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file value_traits.hpp:
//
// Traits information for use in determining which Python methods to
// support for a container with elements of a given type.
//
// History
// =======
// 2003/ 9/12   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: value_traits.hpp,v 1.1.2.6 2003/12/05 17:36:12 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_VALUE_TRAITS_HPP
#define BOOST_PYTHON_INDEXING_VALUE_TRAITS_HPP

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>

namespace boost { namespace python { namespace indexing {
  // The default_value_traits template is used by all ContainerTraits
  // templates. It can be overridden by specialization or by supplying
  // the optional ValueTraits parameter to a container traits
  // template.
  template<typename T> struct value_traits;

  // Implementation for default use. Providing this in a separate
  // template allows specializations of value_traits to make use of
  // it.
  template<typename T>
  struct simple_value_traits {
    BOOST_STATIC_CONSTANT (bool, equality_comparable = true);
    typedef std::equal_to<T> equal_to;

    BOOST_STATIC_CONSTANT (bool, less_than_comparable = true);
    typedef std::less<T> less;

    // Default, do-nothing, version of visit_container_class
    template<typename PythonClass, typename Policy>
    static void visit_container_class (PythonClass &, Policy const &) { }
  };

  // Implementation using pointer indirection
  template <typename Ptr>
  struct indirect_value_traits : simple_value_traits<Ptr> {
    // Hide the base class versions of the comparisons, using these
    // indirect versions
    struct less : std::binary_function<Ptr, Ptr, bool> {
      bool operator() (Ptr const &p1, Ptr const &p2) const {
        return *p1 < *p2;
      }
    };

    struct equal_to : std::binary_function<Ptr, Ptr, bool> {
      bool operator() (Ptr const &p1, Ptr const &p2) const {
        return *p1 == *p2;
      }
    };
  };

  // Default implementation selection. It's basically just a typedef
  // for simple_value_traits
  template<typename T>
  struct value_traits : simple_value_traits<T>
  {
  };

#if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  // Partial specialization for instances of boost::shared_ptr
  template<typename T>
  struct value_traits< ::boost::shared_ptr<T> >
    : indirect_value_traits< ::boost::shared_ptr<T> >
  {
  };
#endif
} } }

#endif // BOOST_PYTHON_INDEXING_VALUE_TRAITS_HPP


"""
