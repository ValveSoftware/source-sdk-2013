# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/python_iterator.hpp"

code = """// -*- mode:c++ -*-
//
// Header file python_iterator.hpp
//
// Handy Python iterable iterators
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
// $Id: python_iterator.hpp,v 1.1.2.5 2003/11/24 16:35:09 raoulgough Exp $
//
// 2008 November 27 Roman Yakovenko
// implementation of the member functions was moved from cpp to header.
// this was done to simplify "installation" procedure.

#ifndef BOOST_PYTHON_INDEXING_PYTHON_ITERATOR_HPP
#define BOOST_PYTHON_INDEXING_PYTHON_ITERATOR_HPP

#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>

namespace boost { namespace python { namespace indexing {
  struct /*BOOST_PYTHON_DECL*/ python_iterator
  {
    python_iterator (boost::python::object obj)
    : m_iter_obj (handle<> (PyObject_GetIter (obj.ptr()))),
      m_next_method (m_iter_obj.attr ("next")),
      m_current()
    {
    }

    // Sets a python type exception and calls throw_error_already_set if
    // the passed object is not iterable via PyObject_GetIter

    bool next ()
    {
        bool result = true; // Assume success

        try
        {
            m_current = m_next_method ();
        }
        catch (boost::python::error_already_set const &)
        {
            if (PyErr_ExceptionMatches (PyExc_StopIteration))
            {
                // Eat this exception
                PyErr_Clear ();
                m_current = boost::python::object (); // No current object
                result = false; // Report failure via return value only
            }
            else
            {
                // Pass it up the line
                throw;
            }
        }
        return result;
    }
    // Get the next item from the iterator, returning true for success

    boost::python::object current() const
    { return m_current; }
    // Callable only after a successful next()

  private:
    ::boost::python::object m_iter_obj;
    ::boost::python::object m_next_method;
    ::boost::python::object m_current;
  };
} } }

#endif // BOOST_PYTHON_INDEXING_PYTHON_ITERATOR_HPP


"""
