# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/proxy_iterator.hpp"

code = """
// Header file proxy_iterator.hpp
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/10/ 8   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: proxy_iterator.hpp,v 1.1.2.8 2003/12/05 17:36:14 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_PROXY_ITERATOR_HPP
#define BOOST_PYTHON_INDEXING_PROXY_ITERATOR_HPP

#include <iterator>
#include <boost/config.hpp>
#include <boost/iterator.hpp>
#include <boost/detail/workaround.hpp>

namespace boost { namespace python { namespace indexing {

  template <class ContainerProxy, typename ElementProxy, typename Traits,
      typename Size, typename Iter>
  class proxy_iterator
    : public boost::iterator<
          std::random_access_iterator_tag,
          ElementProxy,
          typename Traits::difference_type,
          ElementProxy *,
          ElementProxy // Already has reference semantics
      >
  {
#if !defined (BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template<class C, class H, class G> friend class container_proxy;
#endif

    typedef boost::iterator<
        std::random_access_iterator_tag,
        ElementProxy,
        typename Traits::difference_type,
        ElementProxy *,
        ElementProxy
    > base_type;

  public:
    typedef ContainerProxy container_proxy_type;

    typedef Iter raw_iterator;
    typedef Traits raw_iterator_traits;
    typedef Size size_type;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::difference_type difference_type;

    typedef value_type *pointer;
    typedef value_type reference; // Already has reference semantics

    proxy_iterator (container_proxy_type *p, size_type i)
      : ptr (p), index (i)
    {
    }

    proxy_iterator (container_proxy_type *p, raw_iterator iter)
      : ptr (p), index (iter - p->raw_container().begin())
    {
    }

    reference operator*() const { return ptr->at(index); }
    pointer operator->() const { return &ptr->at(index); }
    reference operator[](size_type s) { return ptr->at (index + s); }

    proxy_iterator &operator++ () { ++index; return *this; }
    proxy_iterator &operator+= (size_type s) { index += s; return *this; }

    proxy_iterator &operator-- () { --index; return *this; }

    proxy_iterator operator++ (int) {
      proxy_iterator temp(*this);
      ++index;
      return temp;
    }

    proxy_iterator operator-- (int) {
      proxy_iterator temp(*this);
      --index;
      return temp;
    }

    proxy_iterator &operator-= (size_type s) { index -= s; return *this; }

    proxy_iterator operator+ (size_type s) const {
      return proxy_iterator(*this) += s;
    }

    proxy_iterator operator- (size_type s) const {
      return proxy_iterator(*this) -= s;
    }

    difference_type operator- (proxy_iterator i) const {
      return index - i.index;
    }

    bool operator== (proxy_iterator const &other) const {
      return (ptr == other.ptr) && (index == other.index);
    }

    bool operator!= (proxy_iterator const &other) const {
      return !(*this == other);
    }

    bool operator< (proxy_iterator const &other) const {
      return index < other.index;
    }

    bool operator<= (proxy_iterator const &other) const {
      return index <= other.index;
    }

    bool operator> (proxy_iterator const &other) const {
      return index > other.index;
    }

    bool operator>= (proxy_iterator const &other) const {
      return index >= other.index;
    }

    void iter_swap (proxy_iterator const &other) const {
      ptr->swap_elements (index, other.index);
    }

    //  public:
    // Extensions to the normal iterator interface
    //    void replace (value_type const &copy) { ptr->replace (index, copy); }

#if defined (BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    // Can't declare container_proxy as friend, so make data public
  public:
#else
  private:
#endif
    container_proxy_type *ptr;
    size_type index;
  };
} } }

#if !BOOST_WORKAROUND (BOOST_MSVC, == 1300)
// MSVC7.0 can't decide between this and the unspecialized version
namespace std {
  template <class C, typename E, typename T, typename S, typename I>
  void iter_swap(
      boost::python::indexing::proxy_iterator<C, E, T, S, I> const &first,
      boost::python::indexing::proxy_iterator<C, E, T, S, I> const &second)
  {
    first.iter_swap (second);
  }
}
#endif

#endif // BOOST_PYTHON_INDEXING_PROXY_ITERATOR_HPP


"""
