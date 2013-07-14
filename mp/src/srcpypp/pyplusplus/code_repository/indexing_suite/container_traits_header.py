# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/container_traits.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file container_traits.hpp
//
// Traits information about entire containers for use in determining
// what Python methods to provide.
//
// History
// =======
// 2003/ 8/23   rmg     File creation as container_suite.hpp
// 2003/ 9/ 8   rmg     Renamed container_traits.hpp
// 2003/10/28   rmg     Split container-specific versions into separate headers
// 2004/ 1/28   rmg     Convert to bitset-based feature selection
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: container_traits.hpp,v 1.1.2.15 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_CONTAINER_TRAITS_HPP
#define BOOST_PYTHON_INDEXING_CONTAINER_TRAITS_HPP

#include <indexing_suite/suite_utils.hpp>
#include <indexing_suite/methods.hpp>
#include <indexing_suite/value_traits.hpp>

#include <boost/type_traits.hpp>
#include <boost/call_traits.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/ice.hpp>
#include <boost/iterator/iterator_traits.hpp>

namespace boost { namespace python { namespace indexing {
#if BOOST_WORKAROUND (BOOST_MSVC, <= 1200)
  // MSVC6 has problems with get_signature if parameter types have
  // top-level const qualification (e.g. int const). Unfortunately,
  // this is exactly what happens with boost::call_traits, so we
  // substitute a really dumb version of it instead.

  template<typename T> struct broken_call_traits {
    typedef T const & param_type;
  };
# define BOOST_PYTHON_INDEXING_CALL_TRAITS broken_call_traits
#else
# define BOOST_PYTHON_INDEXING_CALL_TRAITS ::boost::call_traits
#endif

  /////////////////////////////////////////////////////////////////////////
  // Lowest common denominator traits - applicable to real containers
  // and iterator pairs
  /////////////////////////////////////////////////////////////////////////

  template<typename Container, typename ValueTraits = detail::no_override>
  struct base_container_traits
  {
    typedef base_container_traits<Container, ValueTraits> self_type;

  protected:
    BOOST_STATIC_CONSTANT(
        bool, is_mutable = ! boost::is_const<Container>::value);

  public:
    typedef Container container;

    typedef BOOST_DEDUCED_TYPENAME container::value_type value_type;

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        is_const<container>,
        BOOST_DEDUCED_TYPENAME container::const_iterator,
        BOOST_DEDUCED_TYPENAME container::iterator
      >::type iterator;

    typedef typename ::boost::iterator_reference<iterator>::type reference;

    typedef value_type key_type; // Used for find, etc.
    typedef typename container::size_type size_type;
    typedef typename make_signed<size_type>::type index_type;
    // at(), operator[]. Signed to support Python -ve indexes

    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS<value_type>::param_type
        value_param;
    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS<key_type>::param_type
        key_param;
    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS<index_type>::param_type
        index_param;

    // Allow client code to replace the default value traits via our
    // second (optional) template parameter
    typedef value_traits<value_type> default_value_traits;
    typedef typename detail::maybe_override<
        default_value_traits, ValueTraits>::type value_traits_type;

    // Forward visit_container_class to value_traits_type
    template<typename PythonClass, typename Policy>
    static void visit_container_class(
        PythonClass &pyClass, Policy const &policy)
    {
      value_traits_type::visit_container_class (pyClass, policy);
    }
  };

  /////////////////////////////////////////////////////////////////////////
  // ContainerTraits for sequences with random access - std::vector,
  // std::deque and the like
  /////////////////////////////////////////////////////////////////////////

  template<typename Container, typename ValueTraits = detail::no_override>
  class random_access_sequence_traits
    : public base_container_traits<Container, ValueTraits>
  {
    typedef base_container_traits<Container, ValueTraits> base_class;

  public:
    typedef typename base_class::value_traits_type value_traits_type;

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
              method_len
            | method_getitem
            | method_getitem_slice

            | detail::method_set_if<
                  value_traits_type::equality_comparable,
                    method_index
                  | method_contains
                  | method_count
              >::value

            | detail::method_set_if<
                  base_class::is_mutable,
                    method_setitem
                  | method_setitem_slice
                  | method_delitem
                  | method_delitem_slice
                  | method_reverse
                  | method_append
                  | method_insert
                  | method_extend
              >::value

            | detail::method_set_if<
                  type_traits::ice_and<
                      base_class::is_mutable,
                      value_traits_type::less_than_comparable
                  >::value,
                  method_sort
              >::value

        ));

        // Not supported: method_iter, method_has_key
  };

} } }

#endif // BOOST_PYTHON_INDEXING_CONTAINER_SUITE_HPP


"""
