# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/visitor.hpp"

code = """// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// Header file visitor.hpp:
//
// def_visitor implementation to install the container_suite's Python
// methods in an object of a boost::python::class_<> instance.
//
// History
// =======
// 2003/ 9/11   rmg     File creation from container_suite.hpp
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: visitor.hpp,v 1.1.2.16 2004/02/08 18:57:42 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_VISITOR_HPP
#define BOOST_PYTHON_INDEXING_VISITOR_HPP

#include <indexing_suite/slice_handler.hpp>
#include <indexing_suite/suite_utils.hpp> // Get index_style_t

#include <boost/python/def_visitor.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/type_traits/ice.hpp>
#include <boost/bind.hpp>
#include <functional>

namespace boost { namespace python { namespace indexing {
  //////////////////////////////////////////////////////////////////////////
  // Policy override template that masks everything except the precall
  // functions. i.e. it uses default policies for everything except
  // precall, which must be provided by the template argument.
  //////////////////////////////////////////////////////////////////////////

  namespace detail {
    template<typename PrecallPolicy>
    struct precall_only : public boost::python::default_call_policies
    {
      precall_only () : m_precall () { }
      explicit precall_only (PrecallPolicy const &copy) : m_precall (copy) { }

      bool precall (PyObject *args) { return m_precall.precall (args); }
      bool precall (PyObject *args) const { return m_precall.precall (args); }

    private:
      PrecallPolicy m_precall;
    };
  }

  //////////////////////////////////////////////////////////////////////////
  // Ugly macro to define a template that optionally adds a method to
  // a Python class. This version (OPTIONAL_ALGO_SUPPORT) works with
  // static functions in an Algorithms class.
  //
  // This macro is #undef'd at the end of this header
  //////////////////////////////////////////////////////////////////////////

#define OPTIONAL_ALGO_SUPPORT(ADDER_NAME, METHOD_NAME, ALGO_FN) \
  template<bool doit>  \
  struct ADDER_NAME {  \
    template<class PythonClass, class Algorithms, class Policy>  \
    static void apply (PythonClass &, Algorithms const &, Policy const &)  \
    {  \
    }  \
  };  \
\
  template<>  \
  struct ADDER_NAME<true> {  \
    template<class PythonClass, class Algorithms, class Policy>  \
    static void apply(  \
        PythonClass &pyClass,  \
        Algorithms const &,  \
        Policy const &policy)  \
    {  \
      pyClass.def (METHOD_NAME, &Algorithms::ALGO_FN, policy);  \
    }  \
  }

  //////////////////////////////////////////////////////////////////////////
  // Ugly macro to define a template that optionally adds a method to
  // a Python class. This version (OPTIONAL_SLICE_SUPPORT) works with
  // static functions in the slice_handler template.
  //
  // This macro is #undef'd at the end of this header
  //////////////////////////////////////////////////////////////////////////

#define OPTIONAL_SLICE_SUPPORT(ADDER_NAME, METHOD_NAME, SLICE_HANDLER_FN) \
  template<bool doit>  \
  struct ADDER_NAME {  \
    template<class PythonClass, class Algorithms, class Policy>  \
    static void apply (PythonClass &, Algorithms const &, Policy const &)  \
    {  \
    }  \
  };  \
\
  template<>  \
  struct ADDER_NAME<true> {  \
    template<class PythonClass, class Algorithms, class Policy>  \
    static void apply(  \
        PythonClass &pyClass,  \
        Algorithms const &,  \
        Policy const &policy)  \
    {  \
      pyClass.def ( \
          METHOD_NAME, \
          slice_handler<Algorithms, Policy>::SLICE_HANDLER_FN (policy));  \
    }  \
  }

  //////////////////////////////////////////////////////////////////////////
  // __iter__ is a special case not handled by the above macros. First
  // the unspecialized (do-nothing) version
  //////////////////////////////////////////////////////////////////////////

  template<bool doit>
  struct maybe_add_iter {
    template<class PythonClass, class Algorithms, class Policy>
    static void apply (PythonClass &, Algorithms const &, Policy const &) { }
  };

  //////////////////////////////////////////////////////////////////////////
  // Specialization with the real implementation of __iter__
  //////////////////////////////////////////////////////////////////////////

  template<>
  struct maybe_add_iter<true> {
    template<class PythonClass, class Algorithms, class Policy>
    static void apply(
        PythonClass &pyClass,
        Algorithms const &,
        Policy const &)
    {
      // Should maybe separate precall and postcall portions of the
      // policy (precall when generating the range object, postcall
      // when returing from range.next())?

      pyClass.def(
          "__iter__",
          boost::python::range<Policy>(
              Algorithms::begin,
              Algorithms::end));
    }
  };

  //////////////////////////////////////////////////////////////////////////
  // All other optional methods are covered by the two OPTIONAL_*
  // macros
  //////////////////////////////////////////////////////////////////////////

  OPTIONAL_ALGO_SUPPORT  (maybe_add_len, "__len__", size);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_getitem, "__getitem__", get);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_setitem, "__setitem__", assign);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_delitem, "__delitem__", erase_one);
  OPTIONAL_SLICE_SUPPORT (maybe_add_getslice, "__getitem__", make_getitem);
  OPTIONAL_SLICE_SUPPORT (maybe_add_setslice, "__setitem__", make_setitem);
  OPTIONAL_SLICE_SUPPORT (maybe_add_delslice, "__delitem__", make_delitem);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_sort, "sort", sort);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_reverse, "reverse", reverse);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_append, "append", push_back);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_insert, "insert", insert);
  OPTIONAL_SLICE_SUPPORT (maybe_add_extend, "extend", make_extend);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_index, "index", get_index);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_count, "count", count);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_contains, "__contains__", contains);
  OPTIONAL_ALGO_SUPPORT  (maybe_add_has_key, "has_key", contains);

  //////////////////////////////////////////////////////////////////////////
  // Do-all visitor
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy, method_set_type MethodMask>
  class visitor
    : public def_visitor< visitor< Algorithms, Policy, MethodMask > >
  {
    Policy m_policy;

    BOOST_STATIC_CONSTANT (
        method_set_type,
        enabled_methods = Algorithms::supported_methods & MethodMask);

  public:
    typedef Algorithms algorithms_type;

    explicit visitor (Policy const &policy = Policy()) : m_policy (policy) { }

  public:
    template <class PythonClass>
    void visit (PythonClass &pyClass) const
    {
      detail::precall_only<Policy> precallPolicy (m_policy);

      maybe_add_len<detail::is_member<enabled_methods, method_len>::value>
        ::apply(pyClass, algorithms_type(), precallPolicy);

      maybe_add_getitem<
        detail::is_member<enabled_methods, method_getitem>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_getslice<
          detail::is_member<enabled_methods, method_getitem_slice>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_setitem<
        detail::is_member<enabled_methods, method_setitem>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_setslice<
          detail::is_member<enabled_methods, method_setitem_slice>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_delitem<
        detail::is_member<enabled_methods, method_delitem>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_delslice<
          detail::is_member<enabled_methods, method_delitem_slice>::value
      >::apply(pyClass, algorithms_type(), m_policy);

      maybe_add_iter<
        detail::is_member<enabled_methods, method_iter>::value
      >::apply (pyClass, algorithms_type(), m_policy);

      maybe_add_sort<
        detail::is_member<enabled_methods, method_sort>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_reverse<
        detail::is_member<enabled_methods, method_reverse>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_append<
        detail::is_member<enabled_methods, method_append>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_insert<
        detail::is_member<enabled_methods, method_insert>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_extend<
        detail::is_member<enabled_methods, method_extend>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_index<
        detail::is_member<enabled_methods, method_index>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_count<
        detail::is_member<enabled_methods, method_count>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_contains<
        detail::is_member<enabled_methods, method_contains>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      maybe_add_has_key<
        detail::is_member<enabled_methods, method_has_key>::value
      >::apply (pyClass, algorithms_type(), precallPolicy);

      Algorithms::visit_container_class (pyClass, m_policy);
    }
  };
} } }

#undef OPTIONAL_SLICE_SUPPORT
#undef OPTIONAL_ALGO_SUPPORT

#endif // BOOST_PYTHON_INDEXING_VISITOR_HPP


"""
