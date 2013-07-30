# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/slice_handler.hpp"

code = """// -*- mode:c++ -*-
//
// Header file slice_handler.hpp
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 9/ 9   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
// 2009/01/30   Roman   patch from Maik Beckmann was applied - fixes error:
//                      // error: no class template named 'extract_return_type'
// $Id: slice_handler.hpp,v 1.1.2.10 2003/11/24 16:35:52 raoulgough Exp $
//

#ifndef BOOST_PYTHON_INDEXING_SLICE_HANDLER_HPP
#define BOOST_PYTHON_INDEXING_SLICE_HANDLER_HPP

#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/make_function.hpp>
#include <boost/mpl/apply.hpp>
#include <algorithm>

#include <indexing_suite/slice.hpp>
#include <indexing_suite/python_iterator.hpp>

namespace boost { namespace python { namespace indexing {
  template<class Algorithms, class Policy>
  struct slice_handler
  {
    static boost::python::object make_getitem (Policy const &);
    static boost::python::object make_setitem (Policy const &);
    static boost::python::object make_delitem (Policy const &);
    static boost::python::object make_extend  (Policy const &);

  private:
    typedef typename Algorithms::container container;
    typedef typename Algorithms::reference reference;
    typedef typename Algorithms::slice_helper slice_helper;

    static boost::python::list get_slice (container &, slice);
    static void set_slice (container &, slice, boost::python::object);
    static void del_slice (container &, slice);
    static void extend (container &, boost::python::object);
  };

  namespace detail {
    template<typename Policy>
    struct postcall_override
    {
      // This class overrides the Policy's postcall function and
      // result_conveter to handle the list returned from get_slice.
      // The Policy's result_converter is removed, since it gets
      // applied within get_slice. Our postcall override applies the
      // original postcall to each element of the Python list returned
      // from get_slice.

      typedef boost::python::default_result_converter result_converter;
      typedef typename Policy::argument_package argument_package;

      template<class Sig> struct extract_return_type : 
      Policy::template extract_return_type<Sig> { };

      postcall_override (Policy const &p);

      bool precall (PyObject *args);
      PyObject* postcall (PyObject *args, PyObject *result);

    private:
      Policy m_base;
    };
  }

  //////////////////////////////////////////////////////////////////////////
  // Return a function object that implements the slice version of __getitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  boost::python::object
  slice_handler<Algorithms, Policy>
  ::make_getitem (Policy const &policy)
  {
    return
      boost::python::make_function(
          get_slice, detail::postcall_override<Policy> (policy));
  }

  //////////////////////////////////////////////////////////////////////////
  // Return a function object that implements the slice version of __setitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  boost::python::object
  slice_handler<Algorithms, Policy>
  ::make_setitem (Policy const &policy)
  {
    // should we try to get funky with policy::precall?
    return boost::python::make_function (set_slice, policy);
  }

  //////////////////////////////////////////////////////////////////////////
  // Return a function object that implements the slice version of __delitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  boost::python::object
  slice_handler<Algorithms, Policy>
  ::make_delitem (Policy const &policy)
  {
    // should we try to get funky with policy::precall?
    return boost::python::make_function (del_slice, policy);
  }

  //////////////////////////////////////////////////////////////////////////
  // Return a function object that implements extend
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  boost::python::object
  slice_handler<Algorithms, Policy>
  ::make_extend (Policy const &policy)
  {
    // should we try to get funky with policy::precall?
    return boost::python::make_function (extend, policy);
  }

  namespace detail {
    ////////////////////////////////////////////////////////////////////////
    // postcall_override constructor
    ////////////////////////////////////////////////////////////////////////

    template<class Policy>
    postcall_override<Policy>::postcall_override (Policy const &p)
      : m_base (p)
    {
    }

    ////////////////////////////////////////////////////////////////////////
    // precall forwarder
    ////////////////////////////////////////////////////////////////////////

    template<class Policy>
    bool postcall_override<Policy>::precall (PyObject *args)
    {
      return m_base.precall (args);
    }

    ////////////////////////////////////////////////////////////////////////
    // Apply base postcall to each element of the list returend by get_slice
    ////////////////////////////////////////////////////////////////////////

    template<class Policy>
    PyObject *
    postcall_override<Policy>::postcall (PyObject *args, PyObject *result)
    {
      int size = PyList_Size (result);

      for (int count = 0; count < size; ++count)
        {
          m_base.postcall (args, PyList_GetItem (result, count));
        }

      return result;
    }
  } // namespace detail

  //////////////////////////////////////////////////////////////////////////
  // Implementation for the slice version of __getitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  boost::python::list
  slice_handler<Algorithms, Policy>
  ::get_slice (container &c, slice sl)
  {
    typedef typename Policy::result_converter converter_type;
    typedef typename Algorithms::reference reference;

    typename boost::mpl::apply1<converter_type, reference>::type
      converter;

    boost::python::list result;

    slice_helper helper (Algorithms::make_slice_helper (c, sl));

    while (helper.next())
      {
        // Apply the result converter (only) to each element before
        // appending. postcall is done in postcall_override

        result.append (boost::python::handle<> (converter (helper.current())));
      }

    return result;
  }

  //////////////////////////////////////////////////////////////////////////
  // Implementation for the slice version of __setitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  void
  slice_handler<Algorithms, Policy>
  ::set_slice (container &c, slice sl, boost::python::object values)
  {
    python_iterator value_iter (values);

    // Try two kinds of extractors - the first is more efficient (using
    // a reference to existing object, if possible and sensible) and the
    // second allowing implicit conversions.

    typedef boost::python::extract<
        BOOST_DEDUCED_TYPENAME Algorithms::value_param> extractor1;

    typedef boost::python::extract<
        BOOST_DEDUCED_TYPENAME Algorithms::value_type> extractor2;

    // Note: any error during this operation will probably leave the
    // container partially updated. This can occur (for example) if the
    // replacement sequence is of a different length to the original
    // slice and the container does not support insertion/deletion.
    // This could be prevented if the length of the replacement sequence
    // is known in advance (via __len__, for example) but not otherwise.

    slice_helper write_helper (Algorithms::make_slice_helper (c, sl));

    // Overwrite and/or insert elements
    while (value_iter.next())
      {
        extractor1 ex1 (value_iter.current());

        if (ex1.check())
          {
            write_helper.write (ex1);
          }

        else
          {
            write_helper.write (extractor2 (value_iter.current()));
          }
      }

    if (write_helper.next())
      {
        // We've run out of elements to read, but write_helper is not
        // finished. Erase the remaining element(s) in the slice
        write_helper.erase_remaining();
      }
  }

  //////////////////////////////////////////////////////////////////////////
  // Implementation for the slice version of __delitem__
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  void
  slice_handler<Algorithms, Policy>
  ::del_slice (container &c, slice sl)
  {
    slice_helper helper (Algorithms::make_slice_helper (c, sl));

    if (helper.next())
      {
        helper.erase_remaining();
      }
  }

  //////////////////////////////////////////////////////////////////////////
  // Implementation of extend
  //////////////////////////////////////////////////////////////////////////

  template<class Algorithms, class Policy>
  void
  slice_handler<Algorithms, Policy>
  ::extend (container &c, boost::python::object values)
  {
    // Set up a slice at the tail end of the container, and use
    // set_slice to do all the hard work.

    boost::python::object length
      ((boost::python::handle<>
        (PyLong_FromLong (Algorithms::size (c)))));

    slice sl
      ((boost::python::handle<>
        (PySlice_New
         (length.ptr(),
          boost::python::object().ptr(),
          boost::python::object().ptr()))));

    set_slice (c, sl, values);
  }
} } }

#endif // BOOST_PYTHON_INDEXING_SLICE_HANDLER_HPP


"""
