# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/algorithms.hpp"

code = """// Header file algorithms.hpp
//
// Uniform interface layer for all containers.
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 9/11   rmg     File creation from suite_utils.hpp
// 2003/10/28   rmg     Split container-specific versions into separate headers
// 2006/10/25   Roman   Adding keys function to assoc_algorithms class
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: algorithms.hpp,v 1.1.2.15 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_ALGORITHMS_HPP
#define BOOST_PYTHON_INDEXING_ALGORITHMS_HPP

#include <indexing_suite/suite_utils.hpp>

#include <boost/type_traits.hpp>
#include <boost/python/errors.hpp>
#include <indexing_suite/int_slice_helper.hpp>
#include <indexing_suite/slice.hpp>
#include <boost/mpl/if.hpp>
#include <boost/limits.hpp>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <set>

namespace boost { namespace python { namespace indexing {
  template<typename ContainerTraits, typename Ovr = detail::no_override>
  class default_algorithms
  {
    typedef default_algorithms<ContainerTraits, Ovr> self_type;
    typedef typename detail::maybe_override<self_type, Ovr>
        ::type most_derived;

  public:
    typedef ContainerTraits container_traits;

    // Import typedefs from the container_traits for convenience
    typedef typename ContainerTraits::container   container;
    typedef typename ContainerTraits::iterator    iterator;
    typedef typename ContainerTraits::reference   reference;
    typedef typename ContainerTraits::size_type   size_type;
    typedef typename ContainerTraits::value_type  value_type;
    typedef typename ContainerTraits::value_param value_param;
    typedef typename ContainerTraits::index_param index_param;
    typedef typename ContainerTraits::key_param   key_param;

    // Defer selection of supported_methods to the ContainerTraits
    // template argument. This makes sense because default_algorithms
    // derives all of its other information from this argument, and
    // can't decide which of the static member functions will
    // instantiate successfully for the container. Obviously a
    // custom-written Algorithms implementation could choose to
    // provide the supported_methods directly.

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = ContainerTraits::supported_methods);

    static size_type size       (container &);
    static iterator  find       (container &, key_param);
    static size_type get_index  (container &, key_param);
    static size_type count      (container &, key_param);
    static bool      contains   (container &, key_param);
    static void      reverse    (container &);
    static reference get        (container &, index_param);
    static void      assign     (container &, index_param, value_param);
    static void      insert     (container &, index_param, value_param);
    static void      erase_one  (container &, index_param);
    static void      erase_range(container &, index_param, index_param);
    static void      push_back  (container &, value_param);
    static void      sort       (container &);
    //    static void      sort       (container &, PyObject *);

    static iterator  begin      (container &c) { return c.begin(); }
    static iterator  end        (container &c) { return c.end(); }

    // Reasonable defaults for slice handling
    typedef int_slice_helper<self_type, integer_slice> slice_helper;

    static slice_helper make_slice_helper (container &c, slice const &);

    // Default visit_container_class
    template<typename PythonClass, typename Policy>
    static void visit_container_class(
        PythonClass &pyClass, Policy const &policy)
    {
      container_traits::visit_container_class (pyClass, policy);
    }

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    // MSVC6 and 7.0 seem to complain about most_derived::bounds_check
    // for an instantiation of list_algorithms.
  public:
#else
  private:
#endif
    static size_type bounds_check(
        container &, index_param, char const *msg,
        bool one_past = false,
        bool truncate = false);
    // Throws std::out_of_range if necessary. If one_past is set, then
    // indexes up to container.size() *inclusive* are allowed. If
    // truncate is set, then out of bounds values are reset to the
    // nearest in-bound value (and if none exists, throws an
    // exception). If truncate is *not* set, then negative values index
    // from the upper bound backwards and are bounds-checked.
  };

  /////////////////////////////////////////////////////////////////////////
  // Base class for associative containers
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr = detail::no_override>
  class assoc_algorithms
    : public default_algorithms
        <ContainerTraits,
        BOOST_DEDUCED_TYPENAME detail::maybe_override
            <assoc_algorithms<ContainerTraits, Ovr>, Ovr>
          ::type>
  {
    typedef assoc_algorithms<ContainerTraits, Ovr> self_type;
    typedef typename detail::maybe_override<self_type, Ovr>
        ::type most_derived;
    typedef default_algorithms<ContainerTraits, most_derived> Parent;

  public:
    typedef typename Parent::iterator iterator;
    typedef typename Parent::size_type size_type;
    typedef typename Parent::container container;
    typedef typename Parent::reference reference;
    typedef typename Parent::key_param key_param;
    typedef typename Parent::value_param value_param;
    typedef typename Parent::index_param index_param;

    static reference get        (container &, index_param);

    // Use member functions for the following (hiding base class versions)
    static void      erase_one (container &, key_param);
    static iterator  find      (container &, key_param);
    static size_type count     (container &, key_param);
    static bool      contains  (container &, key_param);

    // Default visit_container_class
    template<typename PythonClass, typename Policy>
    static void visit_container_class( PythonClass &pyClass, Policy const &policy)
    {
      ContainerTraits::visit_container_class (pyClass, policy);
    }


  protected:
    static iterator  find_or_throw (container &, index_param);
  };

  /////////////////////////////////////////////////////////////////////////
  // Get the size of a container
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::size_type
  default_algorithms<ContainerTraits, Ovr>::size (container &c)
  {
    return c.size();
  }

  /////////////////////////////////////////////////////////////////////////
  // Range check an index and throw out_of_range if necessary
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::size_type
  default_algorithms<ContainerTraits, Ovr>::bounds_check(
      container &c,
      index_param ix,
      char const *msg,
      bool one_past,
      bool truncate)
  {
    size_type bound = most_derived::size(c) + (one_past ? 1 : 0);
    size_type result;

    if (truncate)
      {
        if (ix < 0)
          {
            result = 0;
          }

        else
          {
            result = ix;

            if ((result >= bound) && (bound > 0))
              {
                result = bound - 1;
              }
          }
      }

    else if (ix < 0)
      {
        if (size_type(-ix) > bound)
          {
            throw std::out_of_range (msg);
          }

        result = bound + ix;
      }

    else
      {
        result = ix;
      }

    if (result >= bound)
      {
        throw std::out_of_range (msg);
      }

    return result;
  }

  /////////////////////////////////////////////////////////////////////////
  // Find an element in a container (std algorithm version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::iterator
  default_algorithms<ContainerTraits, Ovr>::find(
      container &c, key_param key)
  {
    typedef typename container_traits::value_traits_type vtraits;
    typedef typename vtraits::equal_to comparison;

    return std::find_if(
        most_derived::begin(c),
        most_derived::end(c),
        std::bind1st (comparison(), key));
  }

  /////////////////////////////////////////////////////////////////////////
  // Find an element and return its index (std algorithm version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::size_type
  default_algorithms<ContainerTraits, Ovr>::get_index(
      container &c, key_param key)
  {
    iterator found (most_derived::find (c, key));

    if (found == most_derived::end(c))
      {
        PyErr_SetString(
            PyExc_ValueError, "get_index: element not found");

        boost::python::throw_error_already_set ();
      }

    iterator start (most_derived::begin (c));
    return std::distance (start, found);
  }

  /////////////////////////////////////////////////////////////////////////
  // Count occurances of an element in a container (std algorithm version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::size_type
  default_algorithms<ContainerTraits, Ovr>::count(
      container &c, key_param key)
  {
    typedef typename container_traits::value_traits_type vtraits;
    typedef typename vtraits::equal_to comparison;

    return std::count_if(
        most_derived::begin(c),
        most_derived::end(c),
        std::bind1st (comparison(), key));
  }

  /////////////////////////////////////////////////////////////////////////
  // Check whether a container contains the given element (std algo ver)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  bool
  default_algorithms<ContainerTraits, Ovr>::contains(
      container &c, key_param key)
  {
    return most_derived::find (c, key) != most_derived::end(c);
  }

  /////////////////////////////////////////////////////////////////////////
  // Index into a container (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::reference
  default_algorithms<ContainerTraits, Ovr>::get(
      container &c, index_param ix)
  {
    return c[most_derived::bounds_check (c, ix, "get")];
  }

  /////////////////////////////////////////////////////////////////////////
  // Assign a value at a particular index (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  default_algorithms<ContainerTraits, Ovr>::assign(
      container &c, index_param ix, value_param val)
  {
    c[most_derived::bounds_check (c, ix, "assign")] = val;
  }

  /////////////////////////////////////////////////////////////////////////
  // Insert at end of a container (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  default_algorithms<ContainerTraits, Ovr>::push_back(
      container &c, value_param v)
  {
    c.push_back (v);
  }

  /////////////////////////////////////////////////////////////////////////
  // Insert at an index in the container (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  default_algorithms<ContainerTraits, Ovr>::insert(
      container &c, index_param i, value_param v)
  {
    iterator insert_pos (most_derived::begin(c));

    // Index may range up to c.size() inclusive to allow inserting at end
    std::advance(
        insert_pos, most_derived::bounds_check (c, i, "insert", true, true));

    c.insert (insert_pos, v);
  }

  /////////////////////////////////////////////////////////////////////////
  // Erase between given indexes in the container (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  default_algorithms<ContainerTraits, Ovr>::erase_range(
      container &c, index_param from, index_param to)
  {
    iterator start (most_derived::begin(c));
    iterator finish (most_derived::begin(c));

    // Start index must be properly in bounds
    std::advance
      (start, most_derived::bounds_check (c, from, "erase_range (from)"));

    // End index is one-past-the-end, so may range up to c.size() inclusive
    std::advance
      (finish, most_derived::bounds_check (c, to, "erase_range (to)", true));

    c.erase (start, finish);
  }

  /////////////////////////////////////////////////////////////////////////
  // Erase one element at the given index in the container (generic version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  default_algorithms<ContainerTraits, Ovr>::erase_one(
      container &c, index_param ix)
  {
    iterator iter (most_derived::begin(c));
    std::advance (iter, most_derived::bounds_check (c, ix, "erase_one"));
    c.erase (iter);
  }

  /////////////////////////////////////////////////////////////////////////
  // Reverse the contents of a container (std algorithm version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void default_algorithms<ContainerTraits, Ovr>::reverse (container &c)
  {
    std::reverse (most_derived::begin(c), most_derived::end(c));
  }

  /////////////////////////////////////////////////////////////////////////
  // Sort the contents of a container (std algorithm version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void default_algorithms<ContainerTraits, Ovr>::sort (container &c)
  {
    typedef typename container_traits::value_traits_type vtraits;
    typedef typename vtraits::less comparison;
    std::sort (most_derived::begin(c), most_derived::end(c), comparison());
  }

  /////////////////////////////////////////////////////////////////////////
  // slice_helper factory function (default version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME default_algorithms<ContainerTraits, Ovr>::slice_helper
  default_algorithms<ContainerTraits, Ovr>
  ::make_slice_helper (container &c, slice const &sl)
  {
    return slice_helper (c, integer_slice (sl, most_derived::size (c)));
  }

  /////////////////////////////////////////////////////////////////////////
  // Index into a container (associative version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME assoc_algorithms<ContainerTraits, Ovr>::reference
  assoc_algorithms<ContainerTraits, Ovr>::get (container &c, index_param ix)
  {
    return *most_derived::find_or_throw (c, ix);
  }

  /////////////////////////////////////////////////////////////////////////
  // Erase elements with the given key (associative version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  void
  assoc_algorithms<ContainerTraits, Ovr>::erase_one(
      container &c, key_param key)
  {
    if (c.erase (key) == 0)
      {
        PyErr_SetString(
            PyExc_ValueError, "Container does not hold value to be erased");

        boost::python::throw_error_already_set ();
      }
  }

  /////////////////////////////////////////////////////////////////////////
  // Find an element in an associative container
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME assoc_algorithms<ContainerTraits, Ovr>::iterator
  assoc_algorithms<ContainerTraits, Ovr>
  ::find (container &c, key_param key)
  {
    return c.find (key);
  }

  /////////////////////////////////////////////////////////////////////////
  // Find an element in an associative container
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  bool
  assoc_algorithms<ContainerTraits, Ovr>::contains(
      container &c, key_param key)
  {
    return most_derived::find (c, key) != most_derived::end(c);
  }

  /////////////////////////////////////////////////////////////////////////
  // Find an element in an associative container - throw an exception if
  // not found
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME assoc_algorithms<ContainerTraits, Ovr>::iterator
  assoc_algorithms<ContainerTraits, Ovr>::find_or_throw(
      container &c, index_param ix)
  {
    iterator iter = most_derived::find (c, ix);

    if (iter == most_derived::end(c))
      {
        PyErr_SetString(
            PyExc_ValueError, "associative container: key not found");

        boost::python::throw_error_already_set ();
      }

    return iter;
  }

  /////////////////////////////////////////////////////////////////////////
  // Count occurances of an element in a container (associative version)
  /////////////////////////////////////////////////////////////////////////

  template<typename ContainerTraits, typename Ovr>
  BOOST_DEDUCED_TYPENAME assoc_algorithms<ContainerTraits, Ovr>::size_type
  assoc_algorithms<ContainerTraits, Ovr>::count(
      container &c, key_param key)
  {
    return c.count (key);
  }

  /////////////////////////////////////////////////////////////////////////
  // Some meta-information to select algorithms for const and
  // non-const qualified containers. All algorithms_selector specializations
  // include two publically accessible typedefs, called
  // mutable_algorithms and const_algorithms.  This saves having to
  // have separate partial specializations of algorithms for
  // const and non-const containers. Client code should probably
  // specialize algorithms directly.
  /////////////////////////////////////////////////////////////////////////

  namespace detail {
    template<typename Container> class algorithms_selector
# if defined(BOOST_MPL_MSVC_ETI_BUG)
    {
      // Bogus types to prevent compile errors due to ETI
      typedef algorithms_selector<Container> mutable_algorithms;
      typedef algorithms_selector<Container> const_algorithms;
    }
# endif
    ;
  }

  /////////////////////////////////////////////////////////////////////////
  // Algorithms selection for mutable containers
  /////////////////////////////////////////////////////////////////////////

  template<class Container>
  struct algorithms
    : public detail::algorithms_selector<Container>::mutable_algorithms
  {
  };

# if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  /////////////////////////////////////////////////////////////////////////
  // Algorithms selection for const-qualified containers
  /////////////////////////////////////////////////////////////////////////

  template<class Container>
  struct algorithms<Container const>
    : public detail::algorithms_selector<Container>::const_algorithms
  {
  };
# endif
} } }

#endif // BOOST_PYTHON_INDEXING_ALGORITHMS_HPP


"""
