# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/iterator_range.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file iterator_range.hpp
//
// Emulate an STL container using a pair of iterators. Doesn't support
// insertion or deletion, for the obvious reasons.
//
// History
// =======
// 2003/ 9/ 9   rmg     File creation as iterator_pair.hpp
// 2003/10/27   rmg     Renamed iterator_range.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: iterator_range.hpp,v 1.1.2.7 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_ITERATOR_RANGE_HPP
#define BOOST_PYTHON_INDEXING_ITERATOR_RANGE_HPP

#include <stdexcept>
#include <algorithm>
#include <utility>
#include <boost/type_traits.hpp>
#include <boost/type_traits/ice.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <indexing_suite/container_traits.hpp>
#include <indexing_suite/container_suite.hpp>
#include <indexing_suite/algorithms.hpp>
#include <indexing_suite/iterator_traits.hpp>

namespace boost { namespace python { namespace indexing {
  template<typename Iterator>
  class iterator_range
  {
  private:
    typedef typename BOOST_PYTHON_INDEXING_CALL_TRAITS <Iterator>::param_type
        iterator_param;

    typedef ::boost::detail::iterator_traits<Iterator> std_traits;

  public:
    typedef typename std_traits::reference       reference;
    typedef Iterator                             iterator;
    typedef typename std_traits::difference_type size_type;
    typedef typename std_traits::difference_type difference_type;
    typedef typename std_traits::value_type      value_type;
    typedef typename std_traits::pointer         pointer;

    typedef iterator                             const_iterator;
    // Can't tell what the const version of our iterator should
    // be. The client code will have to instantiate iterator_range
    // directly with a const_iterator if that's what it wants.

    // Also can't provide: allocator_type, reverse_iterator or
    // const_reverse_iterator. Could probably provide (but don't)
    // const_reference and const_pointer. These would be the same as
    // reference and pointer if Iterator is itself a const_iterator.

  public:
    iterator_range (iterator_param, iterator_param);
    iterator_range (std::pair<iterator, iterator> const &);

    iterator begin() const;
    iterator end() const;

  public:
    // Only sensible for random_access iterators
    size_type size () const;
    reference operator[] (size_type) const;
    reference at (size_type) const;

  private:
    iterator m_begin;
    iterator m_end;
  };

  // Array support function(s).
  template<typename T> iterator_range<T *> make_iterator_range (T *, T*);

#if !BOOST_WORKAROUND (BOOST_MSVC, <= 1200)
  template<typename T, std::size_t N> iterator_range<T *> make_iterator_range(
      T (&array)[N]);

  template<typename T, std::size_t N> T *begin (T (&array)[N]);
  template<typename T, std::size_t N> T *end   (T (&array)[N]);

# define BOOST_MAKE_ITERATOR_RANGE \
      ::boost::python::indexing::make_iterator_range

#else
  // For compilers that can't deduce template argument array bounds
# define BOOST_MAKE_ITERATOR_RANGE(array) \
      ::boost::python::indexing::make_iterator_range ( \
          (array), ((array) + sizeof(array) / sizeof((array)[0])))
#endif

  template<typename Iterator>
  iterator_range<Iterator>::iterator_range(
      iterator_param begin, iterator_param end)
    : m_begin (begin),
    m_end (end)
  {
  }

  template<typename Iterator>
  iterator_range<Iterator>
  ::iterator_range (std::pair<iterator, iterator> const &pair)
    : m_begin (pair.first),
    m_end (pair.second)
  {
  }

  template<typename Iterator>
  typename iterator_range<Iterator>::iterator
  iterator_range<Iterator>::begin() const
  {
    return m_begin;
  }

  template<typename Iterator>
  typename iterator_range<Iterator>::iterator
  iterator_range<Iterator>::end() const
  {
    return m_end;
  }

  template<typename Iterator>
  typename iterator_range<Iterator>::size_type
  iterator_range<Iterator>::size() const
  {
    return std::distance (begin(), end());
  }

  template<typename Iterator>
  typename iterator_range<Iterator>::reference
  iterator_range<Iterator>::operator[](size_type index) const
  {
    iterator temp (begin());
    std::advance (temp, index);
    return *temp;
  }

  template<typename Iterator>
  typename iterator_range<Iterator>::reference
  iterator_range<Iterator>::at (size_type index) const
  {
    if (index >= size())
      {
        throw std::out_of_range
          (std::string ("iterator_range: index out of range"));
      }

    else
      {
        return (*this)[index];
      }
  }

  template<typename T> iterator_range<T *> make_iterator_range (T *p1, T* p2)
  {
    return iterator_range<T *> (p1, p2);
  }

#if !BOOST_WORKAROUND (BOOST_MSVC, <= 1200)
  template<typename T, std::size_t N>
  T *begin (T (&array)[N]) {
    return array;
  }

  template<typename T, std::size_t N>
  T *end (T (&array)[N]) {
    return array + N;
  }

  template<typename T, std::size_t N>
  iterator_range<T *> make_iterator_range (T (&array)[N]) {
    return iterator_range<T *>(begin (array), end (array));
  }
#endif

  template<typename Container, typename ValueTraits = detail::no_override>
  class iterator_range_traits
    : public base_container_traits<Container, ValueTraits>
  {
    typedef base_container_traits<Container, ValueTraits> base_class;

    typedef ::boost::python::indexing::iterator_traits<
      typename Container::iterator
    > iterator_traits_type;

  public:
    typedef typename base_class::value_traits_type value_traits_type;

  private:
    // Methods that we *can't* support because of our value type
    BOOST_STATIC_CONSTANT(
        method_set_type,
        disabled_methods = (
            detail::method_set_if<
               type_traits::ice_not<
                   value_traits_type::equality_comparable
               >::value,
                 method_index      // Impossible if !equality_comparable
               | method_contains   // Impossible if !equality_comparable
               | method_count      // Impossible if !equality_comparable
            >::value

          | detail::method_set_if<
               type_traits::ice_not<
                   value_traits_type::less_than_comparable
               >::value,
               method_sort         // Impossible if !less_than_comparable
            >::value
        ));

  public:
    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods =
        iterator_traits_type::supported_methods & ~disabled_methods);
  };

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  namespace detail {
    ///////////////////////////////////////////////////////////////////////
    // algorithms support for iterator_range instances
    ///////////////////////////////////////////////////////////////////////

    template <typename Iterator>
    class algorithms_selector<iterator_range<Iterator> >
    {
      typedef iterator_range<Iterator> Container;

      typedef iterator_range_traits<Container>       mutable_traits;
      typedef iterator_range_traits<Container const> const_traits; // ?

    public:
      typedef default_algorithms<mutable_traits> mutable_algorithms;
      typedef default_algorithms<const_traits>   const_algorithms;
    };
  }
#endif

  template<
    class Container,
    method_set_type MethodMask = all_methods,
    class Traits = iterator_range_traits<Container>
  >
  struct iterator_range_suite
    : container_suite<Container, MethodMask, default_algorithms<Traits> >
  {
  };

} } }

#endif // BOOST_PYTHON_INDEXING_ITERATOR_RANGE_HPP


"""
