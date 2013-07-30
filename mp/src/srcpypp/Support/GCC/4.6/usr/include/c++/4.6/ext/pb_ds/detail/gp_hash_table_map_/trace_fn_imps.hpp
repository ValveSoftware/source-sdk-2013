// -*- C++ -*-

// Copyright (C) 2005, 2006, 2009 Free Software Foundation, Inc.
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
 * @file trace_fn_imps.hpp
 * Contains implementations of gp_ht_map_'s trace-mode functions.
 */

#ifdef PB_DS_HT_MAP_TRACE_

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
trace() const
{
  std::cerr << static_cast<unsigned long>(m_num_e) << " " <<
    static_cast<unsigned long>(m_num_used_e) << std::endl;

  for (size_type i = 0; i < m_num_e; ++i)
    {
      std::cerr << static_cast<unsigned long>(i) << " ";

      switch(m_entries[i].m_stat)
        {
	case empty_entry_status:
	  std::cerr << "<empty>";
	  break;
	case erased_entry_status:
	  std::cerr << "<erased>";
	  break;
	case valid_entry_status:
	  std::cerr << PB_DS_V2F(m_entries[i].m_value);
	  break;
	default:
	  _GLIBCXX_DEBUG_ASSERT(0);
	};

      std::cerr << std::endl;
    }
}

#endif // #ifdef PB_DS_HT_MAP_TRACE_
