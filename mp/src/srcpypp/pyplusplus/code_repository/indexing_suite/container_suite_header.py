# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/container_suite.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file container_suite.hpp
//
// Top-level interface to the container suite.
//
// History
// =======
// 2003/ 8/23   rmg     File creation
// 2003/ 9/ 8   rmg     Extracted trait facilities into container_traits.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: container_suite.hpp,v 1.1.2.7 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_CONTAINER_SUITE_HPP
#define BOOST_PYTHON_INDEXING_CONTAINER_SUITE_HPP

#include <indexing_suite/methods.hpp>
#include <indexing_suite/algorithms.hpp>
#include <indexing_suite/visitor.hpp>

#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>

namespace boost { namespace python { namespace indexing {
  typedef boost::python::return_value_policy<boost::python::return_by_value>
  default_container_policies;

  template<
      class Container,
      method_set_type MethodMask = all_methods,  // All supported by algorithms
      class Algorithms
          = algorithms<Container>
  >
  struct container_suite
    : public visitor<Algorithms, default_container_policies, MethodMask>
  {
    typedef Algorithms algorithms;

    template<typename Policy>
    static visitor<Algorithms, Policy, MethodMask>
    with_policies (Policy const &policy)
    {
      return visitor <Algorithms, Policy, MethodMask> (policy);
    }
  };
} } }

#endif // BOOST_PYTHON_INDEXING_CONTAINER_SUITE_HPP


"""
