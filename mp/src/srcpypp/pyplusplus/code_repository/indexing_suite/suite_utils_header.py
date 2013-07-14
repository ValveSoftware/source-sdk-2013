# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/suite_utils.hpp"

code = """// Header file suite_utils.hpp
//
// Shared utilities for the indexing suite.
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 8/23   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: suite_utils.hpp,v 1.1.2.7 2003/11/24 14:28:31 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_SUITE_UTILS_HPP
#define BOOST_PYTHON_INDEXING_SUITE_UTILS_HPP

#include <boost/type_traits.hpp>

namespace boost { namespace python { namespace indexing {
#if BOOST_WORKAROUND (BOOST_MSVC, BOOST_TESTED_AT (1310)) \
  || (defined (__GNUC__) && (__GNUC__ < 3))
  // MSVC and GCC 2.96 seem to have problems comparing enumerated
  // values in a static constant expression, and don't believe that an
  // expression like (traits::index_style >= index_style_nonlinear) is
  // a compile-time constant. However, the problem doesn't exist for
  // int.
  typedef int index_style_t;
  index_style_t const index_style_none = 0;
  index_style_t const index_style_nonlinear = 1;
  index_style_t const index_style_linear = 2;
#else
  enum index_style_t {
    index_style_none,         // No random access (iteration only)
    index_style_nonlinear,  // Random access by key (no slicing)
    index_style_linear     // Random access by integer index (allows slicing)
  };
#endif

  template<typename T>
  class is_mutable_ref
  {
    typedef typename boost::remove_reference<T>::type maybe_const;

  public:
    BOOST_STATIC_CONSTANT (bool, value = !boost::is_const<maybe_const>::value);
  };

  // make_signed attempts to identify the signed version of any
  // numeric type (useful in this case because Python container
  // indexes can be negative).
  template<typename T> struct make_signed {
    typedef T type;
  };

  template<> struct make_signed<char> {
    // Raw "char" could be signed or unsigned. "signed char"
    // guarantess signedness
    typedef signed char type;
  };

  template<> struct make_signed<unsigned char> {
    typedef signed char type;
  };

  template<> struct make_signed<unsigned short> {
    typedef short type;
  };

  template<> struct make_signed<unsigned int> {
    typedef int type;
  };

  template<> struct make_signed<unsigned long> {
    typedef long type;
  };

#if defined (BOOST_HAS_LONG_LONG)
  template<> struct make_signed<unsigned long long> {
    typedef long long type;
  };
#elif defined (BOOST_HAS_MS_INT64)
  template<> struct make_signed<unsigned __int64> {
    typedef __int64 type;
  };
#endif

  namespace detail {
    struct no_override { };

    template<typename Base, typename Override>
    struct maybe_override
    {
      // Probably need to disable this if there is no partial
      // specialization support, because Override is almost certain to
      // be an incomplete type. If that is right, the workaround
      // version would just have to do "typedef Base type;"

      typedef typename mpl::if_
        <is_same <Override, no_override>, Base, Override>
        ::type type;
    };
  }

} } }

#endif // BOOST_PYTHON_INDEXING_SUITE_UTILS_HPP


"""
