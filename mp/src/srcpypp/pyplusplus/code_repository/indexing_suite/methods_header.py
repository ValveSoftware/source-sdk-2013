# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/methods.hpp"

code = """// Copyright (c) 2004 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file methods.hpp
//
// Methods (and sets of methods) that containers can provide.
//
// History
// =======
// 2004/ 1/11   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: methods.hpp,v 1.1.2.1 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_METHODS_HPP
#define BOOST_PYTHON_INDEXING_METHODS_HPP

#include <boost/config.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace python { namespace indexing {

  typedef unsigned long method_set_type;

  enum methods_enum {
    method_len            = 1UL <<  0,
    method_iter           = 1UL <<  1,
    method_getitem        = 1UL <<  2,
    method_getitem_slice  = 1UL <<  3,
    method_index          = 1UL <<  4,
    method_contains       = 1UL <<  5,
    method_count          = 1UL <<  6,
    method_has_key        = 1UL <<  7,
    method_setitem        = 1UL <<  8,
    method_setitem_slice  = 1UL <<  9,
    method_delitem        = 1UL << 10,
    method_delitem_slice  = 1UL << 11,
    method_reverse        = 1UL << 12,
    method_append         = 1UL << 13,
    method_insert         = 1UL << 14,
    method_extend         = 1UL << 15,
    method_sort           = 1UL << 16
  };

  // Some sets of methods that could be useful for disabling expensive
  // features. e.g. something & ~(slice_methods | search_methods)

  enum {
    slice_methods
      = method_getitem_slice | method_setitem_slice | method_delitem_slice
  };

  enum {
    search_methods
      = method_index | method_contains | method_count | method_has_key
  };

  enum {
    reorder_methods
      = method_sort | method_reverse
  };

  enum {
    insert_methods
      = method_append | method_insert | method_extend
  };

  enum {
    all_methods = ~0UL
  };

  namespace detail {
    // Compile-time constant selection:
    //
    // method_set_if<c, t, f>::value == (c ? t : f)
    //
    // where c is convertible to bool, and t and f are convertible to
    // method_set_type. This gives a compile-time constant reliably on
    // all supported compilers.

    template<
        bool Cond, method_set_type TrueValue, method_set_type FalseValue = 0>

    struct method_set_if {
      struct true_type {
        BOOST_STATIC_CONSTANT(method_set_type, value = TrueValue);
      };

      struct false_type {
        BOOST_STATIC_CONSTANT(method_set_type, value = FalseValue);
      };

      typedef typename mpl::if_c<Cond, true_type, false_type>::type
          result_type;

      BOOST_STATIC_CONSTANT(method_set_type, value = result_type::value);
    };

    // Compile-time set membership test:
    // is_member<set, mem>::value == (bool) set & mem
    template<method_set_type Set, method_set_type Member>
    struct is_member {
      // Use a cast to prevent MSVC truncation warning C4305
      BOOST_STATIC_CONSTANT (bool, value = (bool) (Set & Member));
    };
  }

} } } // boost::python::indexing

#endif // BOOST_PYTHON_INDEXING_METHODS_HPP


"""
