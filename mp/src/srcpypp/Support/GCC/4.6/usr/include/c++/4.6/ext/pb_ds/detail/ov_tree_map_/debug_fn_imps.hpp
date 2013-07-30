// -*- C++ -*-

// Copyright (C) 2005, 2006, 2009, 2010, 2011 Free Software Foundation, Inc.
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
 * @file debug_fn_imps.hpp
 * Contains an implementation class for ov_tree_.
 */

#ifdef _GLIBCXX_DEBUG

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
assert_valid() const
{
  std::cout << "av1" << std::endl;
  if (m_a_values == 0 || m_end_it == 0 || m_size == 0)
    _GLIBCXX_DEBUG_ASSERT(m_a_values == 0 &&  m_end_it == 0 && m_size == 0);

  std::cout << "av2" << std::endl;
  assert_iterators();
  std::cout << "av3" << std::endl;
}

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
assert_iterators() const
{
  debug_base::check_size(m_size);
  size_type iterated_num = 0;
  const_iterator prev_it = end();
  _GLIBCXX_DEBUG_ASSERT(m_end_it == m_a_values + m_size);
  for (const_iterator it = begin(); it != end(); ++it)
    {
      ++iterated_num;
      _GLIBCXX_DEBUG_ONLY(debug_base::check_key_exists(PB_DS_V2F(*it));)
      _GLIBCXX_DEBUG_ASSERT(lower_bound(PB_DS_V2F(*it)) == it);
      const_iterator upper_bound_it = upper_bound(PB_DS_V2F(*it));
      --upper_bound_it;
      _GLIBCXX_DEBUG_ASSERT(upper_bound_it == it);
      if (prev_it != end())
	_GLIBCXX_DEBUG_ASSERT(Cmp_Fn::operator()(PB_DS_V2F(*prev_it),
						 PB_DS_V2F(*it)));
      prev_it = it;
    }
  _GLIBCXX_DEBUG_ASSERT(iterated_num == m_size);
}

#endif 

