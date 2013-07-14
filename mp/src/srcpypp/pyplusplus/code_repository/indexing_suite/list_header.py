# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/list.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file list.hpp
//
// Indexing algorithms support for std::list instances
//
// History
// =======
// 2003/10/28   rmg     File creation from algo_selector.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: list.hpp,v 1.1.2.7 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_LIST_HPP
#define BOOST_PYTHON_INDEXING_LIST_HPP

#include <indexing_suite/container_traits.hpp>
#include <indexing_suite/container_suite.hpp>
#include <indexing_suite/algorithms.hpp>
#include <list>

#if BOOST_WORKAROUND (BOOST_MSVC, == 1200)
# include <boost/static_assert.hpp>
# include <boost/type_traits.hpp>
#endif

namespace boost { namespace python { namespace indexing {
  /////////////////////////////////////////////////////////////////////////
  // ContainerTraits implementation for std::list instances
  /////////////////////////////////////////////////////////////////////////

  template<typename Container, typename ValueTraits = detail::no_override>
  class list_traits
    : public base_container_traits<Container, ValueTraits>
  {
    typedef base_container_traits<Container, ValueTraits> base_class;

  public:
    typedef typename base_class::value_traits_type value_traits_type;

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
              method_len
            | method_iter

            | detail::method_set_if<
                  value_traits_type::equality_comparable,
                    method_contains
                  | method_count
              >::value

            | detail::method_set_if<
                  base_class::is_mutable,
                  method_reverse
                  | method_append
              >::value

            | detail::method_set_if<
                  type_traits::ice_and<
                      base_class::is_mutable,
                      value_traits_type::less_than_comparable
                  >::value,
                  method_sort
              >::value

        ));
  };

  /////////////////////////////////////////////////////////////////////////
  // Algorithms implementation for std::list instances
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr = detail::no_override>
  class list_algorithms
    : public default_algorithms
        <ContainerTraits,
        typename detail::maybe_override
            <list_algorithms<ContainerTraits, Ovr>, Ovr>
          ::type>
  {
    typedef list_algorithms<ContainerTraits, Ovr> self_type;
    typedef typename detail::maybe_override<self_type, Ovr>::type most_derived;
    typedef default_algorithms<ContainerTraits, most_derived> Parent;

  public:
    typedef typename Parent::container container;

    // Use member functions for the following (hiding base class versions)
    static void      reverse    (container &);
    static void      sort       (container &);
    //    static void      sort       (container &, PyObject *);
  };

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  namespace detail {
    ///////////////////////////////////////////////////////////////////////
    // algorithms support for std::list instances
    ///////////////////////////////////////////////////////////////////////

    template <class T, class Allocator>
    class algorithms_selector<std::list<T, Allocator> >
    {
      typedef std::list<T, Allocator> Container;

      typedef list_traits<Container>       mutable_traits;
      typedef list_traits<Container const> const_traits;

    public:
      typedef list_algorithms<mutable_traits> mutable_algorithms;
      typedef list_algorithms<const_traits>   const_algorithms;
    };
  }
#endif

  template<
    class Container,
    method_set_type MethodMask = all_methods,
    class Traits = list_traits<Container>
  >
  struct list_suite
    : container_suite<Container, MethodMask, list_algorithms<Traits> >
  {
  };

  /////////////////////////////////////////////////////////////////////////
  // Reverse the contents of a list (using member function)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void list_algorithms<ContainerTraits, Ovr>::reverse (container &c)
  {
    c.reverse();
  }

  /////////////////////////////////////////////////////////////////////////
  // Sort the contents of a list (using member function)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void list_algorithms<ContainerTraits, Ovr>::sort (container &c)
  {
    typedef typename self_type::container_traits::value_traits_type
      vtraits;

    typedef typename vtraits::less comparison;
#if BOOST_WORKAROUND (BOOST_MSVC, == 1200)
    // MSVC6 doesn't have a templated sort member in list, so we just
    // use the parameterless version. This gives the correct behaviour
    // provided that value_traits_type::less is exactly
    // std::less<value_type>. It would be possible to support
    // std::greater<T> (the only other overload of list::sort in
    // MSVC6) with some additional work.
    BOOST_STATIC_ASSERT(
        (::boost::is_same<comparison, std::less<value_type> >::value));
    c.sort ();
#else
    c.sort (comparison());
#endif
  }
} } }

#endif // BOOST_PYTHON_INDEXING_LIST_HPP


"""
