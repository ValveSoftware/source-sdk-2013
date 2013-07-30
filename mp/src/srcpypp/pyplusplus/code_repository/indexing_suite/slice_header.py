# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/slice.hpp"

code = """// Header file slice.hpp
//
// Copyright (c) 2003 Raoul M. Gough
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2003/ 9/10   rmg     File creation
// 2008/12/08   Roman   Change indexing suite layout
//
// $Id: slice.hpp,v 1.1.2.10 2003/11/24 14:28:31 raoulgough Exp $
//
// 2008 November 27 Roman Yakovenko
// implementation of the member functions was moved from cpp to header.
// this was done to simplify "installation" procedure.

#ifndef BOOST_PYTHON_INDEXING_SLICE_HPP
#define BOOST_PYTHON_INDEXING_SLICE_HPP

#include <boost/python/object.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/converter/pytype_object_mgr_traits.hpp>
#include <algorithm>

namespace boost { namespace python { namespace indexing {
  struct /*BOOST_PYTHON_DECL*/ slice : public boost::python::object
  {
    // This is just a thin wrapper around boost::python::object
    // so that it is possible to register a special converter for
    // PySlice_Type and overload C++ functions on slice

#if defined (BOOST_NO_MEMBER_TEMPLATES)
    // MSVC6 doesn't seem to be able to invoke the templated
    // constructor, so provide explicit overloads to match the
    // (currently) known boost::python::object constructors
    explicit slice (::boost::python::handle<> const& p)
    : object (p)
    {}

    explicit slice (::boost::python::detail::borrowed_reference p)
    : object (p)
    {}

    explicit slice (::boost::python::detail::new_reference p)
    : object (p)
    {}

    explicit slice (::boost::python::detail::new_non_null_reference p)
    : object (p)
    {}
#else
    // Better compilers make life easier
    template<typename T> inline slice (T const &ref);
#endif

    slice (slice const & copy) // Copy constructor
    : object (copy)
    {}
  };

  struct /*BOOST_PYTHON_DECL*/ integer_slice
  {
    // This class provides a convenient interface to Python slice
    // objects that contain integer bound and stride values.

    #if PY_VERSION_HEX < 0x02050000
        typedef int index_type;
    #else
        typedef Py_ssize_t index_type;
    #endif

    integer_slice (slice const & sl, index_type length)
      : m_slice (sl) // Leave index members uninitialized
    {
        PySlice_GetIndices(
    #if PY_VERSION_HEX > 0x03020000
            reinterpret_cast<PyObject *> (m_slice.ptr()),
    #else
            reinterpret_cast<PySliceObject *> (m_slice.ptr()),
    #endif
            length,
            &m_start,
            &m_stop,
            &m_step);

        if (m_step == 0)
        {
            // Can happen with Python prior to 2.3
            PyErr_SetString (PyExc_ValueError, "slice step cannot be zero");
            boost::python::throw_error_already_set ();
        }

        m_start = std::max (static_cast<index_type> (0), std::min (length, m_start));
        m_stop = std::max (static_cast<index_type> (0), std::min (length, m_stop));
        m_direction = (m_step > 0) ? 1 : -1;
    }

    // integer_slice must know how big the container is so it can
    // adjust for negative indexes, etc...

    index_type start() const { return m_start; }
    index_type step() const  { return m_step; }
    index_type stop() const  { return m_stop; }

    index_type size() const { return (m_stop - m_start) / m_step; }

    bool in_range (index_type index)
    { return ((m_stop - index) * m_direction) > 0; }

  private:
    slice m_slice;
    index_type m_start;
    index_type m_step;
    index_type m_stop;
    index_type m_direction;
  };
} } }

#if !defined (BOOST_NO_MEMBER_TEMPLATES)
template<typename T>
boost::python::indexing::slice::slice (T const &ref)
  : boost::python::object (ref)
{
  if (!PySlice_Check (this->ptr()))
    {
      PyErr_SetString(
          PyExc_TypeError, "slice constructor: passed a non-slice object");

      boost::python::throw_error_already_set();
    }
}
#endif

namespace boost { namespace python { namespace converter {
  // Specialized converter to handle PySlice_Type objects
  template<>
  struct object_manager_traits<boost::python::indexing::slice>
    : pytype_object_manager_traits<
         &PySlice_Type, ::boost::python::indexing::slice>
  {
  };
}}}

#endif // BOOST_PYTHON_INDEXING_SLICE_HPP


"""
