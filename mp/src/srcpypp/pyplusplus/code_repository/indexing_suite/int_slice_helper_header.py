# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/int_slice_helper.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file int_slice_helper.hpp
//
// History
// =======
// 2003/10/13   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: int_slice_helper.hpp,v 1.1.2.10 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_INT_SLICE_HELPER_HPP
#define BOOST_PYTHON_INDEXING_INT_SLICE_HELPER_HPP

# include <boost/python/errors.hpp>
# include <indexing_suite/workaround.hpp>

namespace boost { namespace python { namespace indexing {
  template<typename Algorithms, typename SliceType>
  struct int_slice_helper
  {
    // Works with a SliceType that provides an int-like index_type
    // that is convertible to the algorithm's index_param

    typedef Algorithms algorithms_type;
    typedef SliceType slice_type;

    typedef typename algorithms_type::container container;
    typedef typename algorithms_type::reference reference;
    typedef typename algorithms_type::value_param value_param;
    typedef typename algorithms_type::container_traits container_traits;
    typedef typename slice_type::index_type index_type;

    int_slice_helper (container &c, slice_type const &);

    bool next();

    reference current () const;
    void write (value_param val);
    void erase_remaining () const;

  private:
    void assign (value_param val) const;
    void insert (value_param val);

  private:
    slice_type m_slice;
    container *m_ptr;
    index_type m_pos;
  };

  template<typename Algorithms, typename SliceType>
  int_slice_helper<Algorithms, SliceType>
  ::int_slice_helper (container &c, slice_type const &sl)
    : m_slice (sl),
    m_ptr (&c),
    m_pos (-1)
  {
  }

  template<typename Algorithms, typename SliceType>
  bool
  int_slice_helper<Algorithms, SliceType>::next()
  {
    bool result = false; // Assume the worst

    if (m_pos == -1)
      {
        // First time call - get to start of the slice (if any)
        m_pos = m_slice.start();
        result = m_slice.in_range (m_pos);
      }

    else if (m_slice.in_range (m_pos))
      {
        // Subsequent calls - advance by the slice's stride
        m_pos += m_slice.step();
        result = m_slice.in_range (m_pos);
      }

    return result;
  }

  template<typename Algorithms, typename SliceType>
  typename int_slice_helper<Algorithms, SliceType>::reference
  int_slice_helper<Algorithms, SliceType>::current () const
  {
    return algorithms_type::get (*m_ptr, m_pos);
  }

  template<typename Algorithms, typename SliceType>
  void int_slice_helper<Algorithms, SliceType>::write (value_param val)
  {
    if (next())
      {
        assign (val);
      }

    else
      {
        insert (val);
      }
  }

  template<typename Algorithms, typename SliceType>
  void int_slice_helper<Algorithms, SliceType>::assign (value_param val) const
  {
    algorithms_type::assign (*m_ptr, m_pos, val);
  }

  namespace detail {
    template<bool doit> struct maybe_insert {

      // NOTE: use the name "apply_" instead of "apply" to avoid
      // weirdo compiler crash in mpl/aux_/apply.hpp on MSVC7. Don't
      // even ask how I arrived at this fix :-)

      template<class Algorithms>
      static void apply_(
# if defined (BOOST_NO_MEMBER_TEMPLATES) \
        && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          // Can't explicitly instantiate member function - must let
          // the compiler deduce the argument type from a dummy
          // parameter. Same applies throughout
          Algorithms *,
# endif
          typename Algorithms::container &,
          typename Algorithms::index_param,
          typename Algorithms::value_param)
      {
        PyErr_SetString(
            PyExc_TypeError,
            "container does not support insertion into slice");

        boost::python::throw_error_already_set ();
      }
    };

    template<> struct maybe_insert<true> {
      template<class Algorithms>
      static void apply_(
# if defined (BOOST_NO_MEMBER_TEMPLATES) \
        && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          Algorithms *,
# endif
          typename Algorithms::container &c,
          typename Algorithms::index_param i,
          typename Algorithms::value_param v)
      {
        Algorithms::insert (c, i, v);
      }
    };
  }

  template<typename Algorithms, typename SliceType>
  void int_slice_helper<Algorithms, SliceType>::insert (value_param val)
  {
    if (m_slice.step() != 1)
      {
        PyErr_SetString(
            PyExc_ValueError, "attempt to insert via extended slice");

        boost::python::throw_error_already_set ();
      }

    else
      {
        detail::maybe_insert<
          detail::is_member<
            container_traits::supported_methods, method_insert>::value>::
# if defined (BOOST_NO_MEMBER_TEMPLATES) \
  && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          apply_ (static_cast<Algorithms *>(0),
# else
          BOOST_NESTED_TEMPLATE apply_ <Algorithms>(
# endif
              *m_ptr, m_pos, val);

        ++m_pos;  // Advance for any subsequent inserts
      }
  }

  namespace detail {
    template<bool doit> struct maybe_erase {
      template<class Algorithms>
      static void apply_(
# if defined (BOOST_NO_MEMBER_TEMPLATES) \
        && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          Algorithms *,
# endif
          typename Algorithms::container &,
          typename Algorithms::index_param,
          typename Algorithms::index_param)
      {
        PyErr_SetString(
            PyExc_TypeError, "container does not support item deletion");

        boost::python::throw_error_already_set ();
      }
    };

    template<> struct maybe_erase<true> {
      template<class Algorithms>
      static void apply_(
# if defined (BOOST_NO_MEMBER_TEMPLATES) \
        && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          Algorithms *,
# endif
          typename Algorithms::container &c,
          typename Algorithms::index_param from,
          typename Algorithms::index_param to)
      {
        Algorithms::erase_range (c, from, to);
      }
    };
  }

  template<typename Algorithms, typename SliceType>
  void int_slice_helper<Algorithms, SliceType>::erase_remaining () const
  {
    if (m_slice.step() != 1)
      {
        PyErr_SetString(
            PyExc_ValueError, "attempt to delete via extended slice");

        boost::python::throw_error_already_set ();
      }

    else
      {
        detail::maybe_erase<
          detail::is_member<
            container_traits::supported_methods, method_delitem>::value>::

# if defined (BOOST_NO_MEMBER_TEMPLATES) \
  && defined (BOOST_MSVC6_MEMBER_TEMPLATES)
          apply_ (static_cast<Algorithms *>(0),
# else
          BOOST_NESTED_TEMPLATE apply_ <Algorithms>(
# endif
              *m_ptr, m_pos, m_slice.stop());
      }
  }

} } }

#endif // BOOST_PYTHON_INDEXING_INT_SLICE_HELPER_HPP


"""
