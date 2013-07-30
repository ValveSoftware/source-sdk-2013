// Pair implementation -*- C++ -*-

// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/** @file bits/stl_pair.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{utility}
 */

#ifndef _STL_PAIR_H
#define _STL_PAIR_H 1

#include <bits/move.h> // for std::move / std::forward, and std::swap

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#include <type_traits> // for std::__decay_and_strip too
#endif

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

#ifdef __GXX_EXPERIMENTAL_CXX0X__
  /// piecewise_construct_t
  struct piecewise_construct_t { };

  /// piecewise_construct
  constexpr piecewise_construct_t piecewise_construct = piecewise_construct_t();

  // Forward declarations.
  template<typename...>
    class tuple;

  template<int...>
    struct _Index_tuple;
#endif

  /// Struct holding two objects of arbitrary type.
  template<class _T1, class _T2>
    struct pair
    {
      typedef _T1 first_type;    /// @c first_type is the first bound type
      typedef _T2 second_type;   /// @c second_type is the second bound type

      _T1 first;                 /// @c first is a copy of the first object
      _T2 second;                /// @c second is a copy of the second object

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 265.  std::pair::pair() effects overly restrictive
      /** The default constructor creates @c first and @c second using their
       *  respective default constructors.  */
      _GLIBCXX_CONSTEXPR pair()
      : first(), second() { }

      /** Two objects may be passed to a @c pair constructor to be copied.  */
      _GLIBCXX_CONSTEXPR pair(const _T1& __a, const _T2& __b)
      : first(__a), second(__b) { }

      /** There is also a templated copy ctor for the @c pair class itself.  */
      template<class _U1, class _U2>
	_GLIBCXX_CONSTEXPR pair(const pair<_U1, _U2>& __p)
	: first(__p.first), second(__p.second) { }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
      constexpr pair(const pair&) = default;

      // Implicit.
      // pair(pair&&) = default;

      // DR 811.
      template<class _U1, class = typename
	       std::enable_if<std::is_convertible<_U1, _T1>::value>::type>
	pair(_U1&& __x, const _T2& __y)
	: first(std::forward<_U1>(__x)), second(__y) { }

      template<class _U2, class = typename
	       std::enable_if<std::is_convertible<_U2, _T2>::value>::type>
	pair(const _T1& __x, _U2&& __y)
	: first(__x), second(std::forward<_U2>(__y)) { }

      template<class _U1, class _U2, class = typename
	       std::enable_if<std::is_convertible<_U1, _T1>::value
			      && std::is_convertible<_U2, _T2>::value>::type>
	pair(_U1&& __x, _U2&& __y)
	: first(std::forward<_U1>(__x)), second(std::forward<_U2>(__y)) { }

      template<class _U1, class _U2>
	pair(pair<_U1, _U2>&& __p)
	: first(std::forward<_U1>(__p.first)),
	  second(std::forward<_U2>(__p.second)) { }

      template<class... _Args1, class... _Args2>
	pair(piecewise_construct_t,
	     tuple<_Args1...> __first, tuple<_Args2...> __second)
	: first(__cons<first_type>(std::move(__first))),
	  second(__cons<second_type>(std::move(__second))) { }

      pair&
      operator=(const pair& __p)
      {
	first = __p.first;
	second = __p.second;
	return *this;
      }

      pair&
      operator=(pair&& __p)
      {
	first = std::move(__p.first);
	second = std::move(__p.second);
	return *this;
      }

      template<class _U1, class _U2>
	pair&
	operator=(const pair<_U1, _U2>& __p)
	{
	  first = __p.first;
	  second = __p.second;
	  return *this;
	}

      template<class _U1, class _U2>
	pair&
	operator=(pair<_U1, _U2>&& __p)
	{
	  first = std::move(__p.first);
	  second = std::move(__p.second);
	  return *this;
	}

      void
      swap(pair& __p)
      {
	using std::swap;
	swap(first, __p.first);
	swap(second, __p.second);
      }

    private:
      template<typename _Tp, typename... _Args>
	static _Tp
	__cons(tuple<_Args...>&&);

      template<typename _Tp, typename... _Args, int... _Indexes>
	static _Tp
	__do_cons(tuple<_Args...>&&, const _Index_tuple<_Indexes...>&);
#endif
    };

  /// Two pairs of the same type are equal iff their members are equal.
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator==(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return __x.first == __y.first && __x.second == __y.second; }

  /// <http://gcc.gnu.org/onlinedocs/libstdc++/manual/utilities.html>
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator<(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return __x.first < __y.first
	     || (!(__y.first < __x.first) && __x.second < __y.second); }

  /// Uses @c operator== to find the result.
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator!=(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return !(__x == __y); }

  /// Uses @c operator< to find the result.
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator>(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return __y < __x; }

  /// Uses @c operator< to find the result.
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator<=(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return !(__y < __x); }

  /// Uses @c operator< to find the result.
  template<class _T1, class _T2>
    inline _GLIBCXX_CONSTEXPR bool
    operator>=(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& __y)
    { return !(__x < __y); }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
  /// See std::pair::swap().
  // Note:  no std::swap overloads in C++03 mode, this has performance
  //        implications, see, eg, libstdc++/38466.
  template<class _T1, class _T2>
    inline void
    swap(pair<_T1, _T2>& __x, pair<_T1, _T2>& __y)
    { __x.swap(__y); }
#endif

  /**
   *  @brief A convenience wrapper for creating a pair from two objects.
   *  @param  x  The first object.
   *  @param  y  The second object.
   *  @return   A newly-constructed pair<> object of the appropriate type.
   *
   *  The standard requires that the objects be passed by reference-to-const,
   *  but LWG issue #181 says they should be passed by const value.  We follow
   *  the LWG by default.
   */
  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 181.  make_pair() unintended behavior
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  // NB: DR 706.
  template<class _T1, class _T2>
    inline pair<typename __decay_and_strip<_T1>::__type,
		typename __decay_and_strip<_T2>::__type>
    make_pair(_T1&& __x, _T2&& __y)
    {
      typedef typename __decay_and_strip<_T1>::__type __ds_type1;
      typedef typename __decay_and_strip<_T2>::__type __ds_type2;
      typedef pair<__ds_type1, __ds_type2> 	      __pair_type;
      return __pair_type(std::forward<_T1>(__x), std::forward<_T2>(__y));
    }
#else
  template<class _T1, class _T2>
    inline pair<_T1, _T2>
    make_pair(_T1 __x, _T2 __y)
    { return pair<_T1, _T2>(__x, __y); }
#endif

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace

#endif /* _STL_PAIR_H */
