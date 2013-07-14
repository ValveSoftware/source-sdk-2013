# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/set.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file set.hpp
//
// Indexing algorithms support for std::set instances
//
// History
// =======
// 2003/10/28   rmg     File creation from algo_selector.hpp
// 2008/12/08   Roman   Change indexing suite layout
// 2010/04/29   Roman   Adding "__len__" method
//
// $Id: set.hpp,v 1.1.2.6 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_SET_HPP
#define BOOST_PYTHON_INDEXING_SET_HPP

#include <indexing_suite/container_traits.hpp>
#include <indexing_suite/container_suite.hpp>
#include <indexing_suite/algorithms.hpp>
#include <boost/type_traits/is_const.hpp>
#include <set>

namespace boost { namespace python { namespace indexing {
  /////////////////////////////////////////////////////////////////////////
  // ContainerTraits implementation for std::set instances
  /////////////////////////////////////////////////////////////////////////

  template<typename Container>
  class set_traits : public base_container_traits<Container>
  {
    typedef base_container_traits<Container> base_class;

  public:
    typedef typename Container::key_type value_type; // probably unused
    typedef typename Container::key_type index_type; // operator[]
    typedef typename Container::key_type key_type;   // find, count, ...

    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS <value_type>::param_type
        value_param;
    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS <key_type>::param_type
        key_param;
    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS <index_type>::param_type
        index_param;

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
              method_iter
            | method_getitem
            | method_contains
            | method_count
            | method_has_key
            | method_len

            | detail::method_set_if<
                  base_class::is_mutable,
                    method_delitem
                  | method_insert
              >::value
        ));
  };

  /////////////////////////////////////////////////////////////////////////
  // Algorithms implementation for std::set instances
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr = detail::no_override>
  class set_algorithms
    : public assoc_algorithms
        <ContainerTraits,
        typename detail::maybe_override
            <set_algorithms<ContainerTraits, Ovr>, Ovr>
          ::type>
  {
    typedef set_algorithms<ContainerTraits, Ovr> self_type;
    typedef typename detail::maybe_override<self_type, Ovr>::type most_derived;
    typedef assoc_algorithms<ContainerTraits, most_derived> Parent;

  public:
    typedef typename Parent::container container;
    typedef typename Parent::value_param value_param;
    typedef typename Parent::index_param index_param;

    static void      insert     (container &, index_param);

    template<typename PythonClass, typename Policy>
    static void visit_container_class( PythonClass &pyClass, Policy const &policy)
    {
      ContainerTraits::visit_container_class (pyClass, policy);
      pyClass.def( "add", &self_type::insert );
    }

  };

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  namespace detail {
    ///////////////////////////////////////////////////////////////////////
    // algorithms support for std::set instances
    ///////////////////////////////////////////////////////////////////////

    template <class Key, class Compare, class Allocator>
    class algorithms_selector<std::set<Key, Compare, Allocator> >
    {
      typedef std::set<Key, Compare, Allocator> Container;

      typedef set_traits<Container>       mutable_traits;
      typedef set_traits<Container const> const_traits;

    public:
      typedef set_algorithms<mutable_traits> mutable_algorithms;
      typedef set_algorithms<const_traits>   const_algorithms;
    };

    ///////////////////////////////////////////////////////////////////////
    // algorithms support for std::multiset instances
    ///////////////////////////////////////////////////////////////////////

    template <class Key, class Compare, class Allocator>
    class algorithms_selector<std::multiset<Key, Compare, Allocator> >
    {
      typedef std::multiset<Key, Compare, Allocator> Container;

      typedef set_traits<Container>       mutable_traits;
      typedef set_traits<Container const> const_traits;

    public:
      typedef set_algorithms<mutable_traits> mutable_algorithms;
      typedef set_algorithms<const_traits>   const_algorithms;
    };
  }
#endif

  template<
    class Container,
    method_set_type MethodMask = all_methods,
    class Traits = set_traits<Container>
  >
  struct set_suite
    : container_suite<Container, MethodMask, set_algorithms<Traits> >
  {
  };

  /////////////////////////////////////////////////////////////////////////
  // Insert an element into a set
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  set_algorithms<ContainerTraits, Ovr>::insert(
      container &c, index_param ix)
  {
    c.insert (ix);
    //~ Python set does not raise exception in this situation
    //~ if (!c.insert (ix).second)
      //~ {
        //~ PyErr_SetString(
            //~ PyExc_ValueError, "Set already holds value for insertion");

        //~ boost::python::throw_error_already_set ();
      //~ }
  }
} } }

#endif // BOOST_PYTHON_INDEXING_SET_HPP


"""
