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
 * @file erase_no_store_hash_fn_imps.hpp
 * Contains implementations of gp_ht_map_'s erase related functions,
 * when the hash value is not stored.
 */

PB_DS_CLASS_T_DEC
inline bool
PB_DS_CLASS_C_DEC::
erase_imp(const_key_reference r_key,  false_type)
{
  _GLIBCXX_DEBUG_ONLY(PB_DS_CLASS_C_DEC::assert_valid();)
  size_type hash = ranged_probe_fn_base::operator()(r_key);
  size_type i;
  resize_base::notify_erase_search_start();

  for (i = 0; i < m_num_e; ++i)
    {
      const size_type pos = ranged_probe_fn_base::operator()(r_key,  hash, i);
      entry* const p_e = m_entries + pos;
      switch(p_e->m_stat)
        {
        case empty_entry_status:
	  {
	    resize_base::notify_erase_search_end();
	    _GLIBCXX_DEBUG_ONLY(debug_base::check_key_does_not_exist(
								    r_key));
	    return false;
	  }
	  break;
        case valid_entry_status:
	  if (hash_eq_fn_base::operator()(PB_DS_V2F(p_e->m_value), r_key))
            {
	      resize_base::notify_erase_search_end();
	      erase_entry(p_e);
	      do_resize_if_needed_no_throw();
	      return true;
            }
	  break;
        case erased_entry_status:
	  break;
        default:
	  _GLIBCXX_DEBUG_ASSERT(0);
        };
      resize_base::notify_erase_search_collision();
    }
  resize_base::notify_erase_search_end();
  return false;
}

