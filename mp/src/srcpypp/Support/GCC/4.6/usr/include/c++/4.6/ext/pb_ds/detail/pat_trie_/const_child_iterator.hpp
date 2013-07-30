// -*- C++ -*-

// Copyright (C) 2005, 2006, 2009, 2010 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software
// Foundation; either version 3, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// Copyright (C) 2004 Ami Tavory and Vladimir Dreizin, IBM-HRL.

// Permission to use, copy, modify, sell, and distribute this software
// is hereby granted without fee, provided that the above copyright
// notice appears in all copies, and that both that copyright notice
// and this permission notice appear in supporting documentation. None
// of the above authors, nor IBM Haifa Research Laboratories, make any
// representation about the suitability of this software for any
// purpose. It is provided "as is" without express or implied
// warranty.

/**
 * @file const_child_iterator.hpp
 * Contains a const_iterator for a patricia tree.
 */

struct const_iterator
{
public:
  typedef std::forward_iterator_tag iterator_category;

  typedef typename Allocator::difference_type difference_type;

  typedef node_pointer value_type;

  typedef node_pointer_pointer pointer;

  typedef node_pointer_reference reference;

public:
  inline
  const_iterator(node_pointer_pointer p_p_cur = 0,  
		 node_pointer_pointer p_p_end = 0) 
  : m_p_p_cur(p_p_cur), m_p_p_end(p_p_end)
  { }

  inline bool
  operator==(const const_iterator& other) const
  { return m_p_p_cur == other.m_p_p_cur; }

  inline bool
  operator!=(const const_iterator& other) const
  { return m_p_p_cur != other.m_p_p_cur; }

  inline const_iterator& 
  operator++()
  {
    do
      ++m_p_p_cur;
    while (m_p_p_cur != m_p_p_end&& * m_p_p_cur == 0);
    return *this;
  }

  inline const_iterator
  operator++(int)
  {
    const_iterator ret_it(*this);
    operator++();
    return ret_it;
  }

  const node_pointer_pointer
  operator->() const
  {
    _GLIBCXX_DEBUG_ONLY(assert_referencible();)
    return (m_p_p_cur);
  }

  const_node_pointer
  operator*() const
  {
    _GLIBCXX_DEBUG_ONLY(assert_referencible();)
    return (*m_p_p_cur);
  }

protected:
#ifdef _GLIBCXX_DEBUG
  void
  assert_referencible() const
  { _GLIBCXX_DEBUG_ASSERT(m_p_p_cur != m_p_p_end&& * m_p_p_cur != 0); }
#endif 

public:
  node_pointer_pointer m_p_p_cur;
  node_pointer_pointer m_p_p_end;
};

