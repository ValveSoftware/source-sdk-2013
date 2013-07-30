# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/iterator_traits.hpp"

code = """// -*- mode:c++ -*-
//
// Header file iterator_traits.hpp
//
// Traits information about iterators for use in determining what
// Python methods to support for a container.
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 8/23   rmg     File creation as iterator_suite.hpp
// 2003/ 9/12   rmg     Renamed iterator_traits.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: iterator_traits.hpp,v 1.1.2.14 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_ITERATOR_TRAITS_HPP
#define BOOST_PYTHON_INDEXING_ITERATOR_TRAITS_HPP

#include <indexing_suite/suite_utils.hpp>
#include <indexing_suite/methods.hpp>
//#include <indexing_suite/workaround.hpp>

//#include <boost/call_traits.hpp>
//#include <boost/type_traits.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/iterator_categories.hpp>

namespace boost { namespace python { namespace indexing {
  //////////////////////////////////////////////////////////////////////////
  // Indexing traits common base class
  //////////////////////////////////////////////////////////////////////////

  template<typename Iterator>
  struct base_iterator_traits
  {
  private:
    typedef ::boost::detail::iterator_traits<Iterator> std_traits;
    typedef typename std_traits::reference reference;

  public:
    //    typedef Iterator                             iterator;
    //    typedef typename std_traits::value_type      value_type;
    //    typedef typename std_traits::difference_type difference_type;

    BOOST_STATIC_CONSTANT(
        bool, has_mutable_ref = is_mutable_ref<reference>::value);
  };

  //////////////////////////////////////////////////////////////////////////
  // Indexing traits for input iterators
  //////////////////////////////////////////////////////////////////////////

  template<typename Iterator>
  struct input_iterator_traits : base_iterator_traits<Iterator>
  {
    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
            method_iter
        ));
  };

  //////////////////////////////////////////////////////////////////////////
  // Indexing traits for forward iterators
  //////////////////////////////////////////////////////////////////////////

  template<typename Iterator>
  struct forward_iterator_traits
    : public base_iterator_traits<Iterator>
  {
    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
              method_len
            | method_iter
        ));
  };

  //////////////////////////////////////////////////////////////////////////
  // Indexing traits for bidirectional iterators
  //////////////////////////////////////////////////////////////////////////

  template<typename Iterator>
  struct bidirectional_iterator_traits
    : public forward_iterator_traits<Iterator>
  {
    typedef forward_iterator_traits<Iterator> base_class;

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
              base_class::supported_methods

            | detail::method_set_if<
                  base_class::has_mutable_ref,
                  method_reverse
              >::value
        ));
  };

  //////////////////////////////////////////////////////////////////////////
  // Indexing traits for random access iterators
  //////////////////////////////////////////////////////////////////////////

  template<typename Iterator>
  struct random_access_iterator_traits
    : public bidirectional_iterator_traits<Iterator>
  {
    typedef bidirectional_iterator_traits<Iterator> base_class;

    BOOST_STATIC_CONSTANT(
        method_set_type,
        supported_methods = (
            (base_class::supported_methods & ~method_iter)

            | method_getitem
            | method_getitem_slice

            | detail::method_set_if<
                  base_class::has_mutable_ref,
                  method_setitem
                | method_setitem_slice
              >::value

            | method_index     // Assumes value is equality_comparable
            | method_contains  // Assumes value is equality_comparable
            | method_count     // Assumes value is equality_comparable

            | detail::method_set_if<
                  base_class::has_mutable_ref,
                  method_sort
              >::value         // Assumes value is less_than_comparable

        ));
  };

  namespace iterator_detail {
    // Some meta-progamming machinery to select the right
    // indexing::iterator_traits template on the basis of
    // BOOST_ITERATOR_CATEGORY

    typedef char basic_iter_sizer[1];
    typedef char forward_iter_sizer[2];
    typedef char bidirectional_iter_sizer[3];
    typedef char random_access_iter_sizer[4];

    basic_iter_sizer         &sizer (void *);
    forward_iter_sizer       &sizer (std::forward_iterator_tag *);
    bidirectional_iter_sizer &sizer (std::bidirectional_iterator_tag *);
    random_access_iter_sizer &sizer (std::random_access_iterator_tag *);

    template<size_t Size> struct traits_by_size {
      // Default implementation gives only the most basic support
      template<typename Iterator>
        struct traits {
          typedef input_iterator_traits<Iterator> type;
        };
    };

    template<>
    struct traits_by_size<sizeof(forward_iter_sizer)> {
      template<typename Iterator>
        struct traits {
          typedef forward_iterator_traits<Iterator> type;
        };
    };

    template<>
    struct traits_by_size<sizeof(bidirectional_iter_sizer)> {
      template<typename Iterator>
        struct traits {
          typedef bidirectional_iterator_traits<Iterator> type;
        };
    };

    template<>
    struct traits_by_size<sizeof(random_access_iter_sizer)> {
      template<typename Iterator>
        struct traits {
          typedef random_access_iterator_traits<Iterator> type;
        };
    };

    template<typename Iterator>
    class deduced_traits {
      typedef typename ::boost::BOOST_ITERATOR_CATEGORY<Iterator>::type
          category;

      BOOST_STATIC_CONSTANT(
          size_t, sizer_result = sizeof (sizer (new category)));

    public:
      typedef typename traits_by_size<sizer_result>
        ::BOOST_NESTED_TEMPLATE traits<Iterator>::type type;
    };
  }

  template<typename Iterator>
  struct iterator_traits
    : public iterator_detail::deduced_traits<Iterator>::type
  {
  };
} } }

#endif // BOOST_PYTHON_INDEXING_ITERATOR_TRAITS_HPP


"""
