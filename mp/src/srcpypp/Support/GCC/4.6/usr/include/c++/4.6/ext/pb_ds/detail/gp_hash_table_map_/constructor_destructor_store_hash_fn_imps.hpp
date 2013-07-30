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
 * @file constructor_destructor_store_hash_fn_imps.hpp
 * Contains implementations of gp_ht_map_'s constructors, destructor,
 * and related functions.
 */

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
constructor_insert_new_imp(const_mapped_reference r_val, size_type pos, 
			   true_type)
{
  _GLIBCXX_DEBUG_ASSERT(m_entries[pos].m_stat != valid_entry_status);
  entry* const p_e = m_entries + pos;
  new (&p_e->m_value) mapped_value_type(r_val);
  p_e->m_hash = ranged_probe_fn_base::operator()(PB_DS_V2F(r_val)).second;
  p_e->m_stat = valid_entry_status;
  _GLIBCXX_DEBUG_ONLY(debug_base::insert_new(p_e->m_value.first);)
}
