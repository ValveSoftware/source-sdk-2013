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
 * @file insert_fn_imps.hpp
 * Contains an implementation class for a binary_heap.
 */

PB_DS_CLASS_T_DEC
inline typename PB_DS_CLASS_C_DEC::point_iterator
PB_DS_CLASS_C_DEC::
push(const_reference r_val)
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  insert_value(r_val, s_no_throw_copies_ind);
  std::push_heap(m_a_entries, m_a_entries + m_size, 
		 static_cast<entry_cmp&>(*this));
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  return point_iterator(m_a_entries);
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
insert_value(value_type val, true_type)
{
  resize_for_insert_if_needed();

  m_a_entries[m_size++] = val;
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
insert_value(const_reference r_val, false_type)
{
  resize_for_insert_if_needed();
  pointer p_new = s_value_allocator.allocate(1);
  cond_dealtor_t cond(p_new);
  new (p_new) value_type(r_val);
  cond.set_no_action();
  m_a_entries[m_size++] = p_new;
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
insert_entry(entry e)
{
  resize_for_insert_if_needed();
  m_a_entries[m_size++] = e;
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
resize_for_insert_if_needed()
{
  if (!resize_policy::resize_needed_for_grow(m_size))
    {
      _GLIBCXX_DEBUG_ASSERT(m_size < m_actual_size);
      return;
    }

  const size_type new_actual_size = resize_policy::get_new_size_for_grow();
  entry_pointer a_new_entries = s_entry_allocator.allocate(new_actual_size);
  resize_policy::notify_grow_resize();
  std::copy(m_a_entries, m_a_entries + m_size, a_new_entries);
  s_entry_allocator.deallocate(m_a_entries, m_actual_size);
  m_actual_size = new_actual_size;
  m_a_entries = a_new_entries;
}

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
modify(point_iterator it, const_reference r_new_val)
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  swap_value_imp(it.m_p_e, r_new_val, s_no_throw_copies_ind);
  fix(it.m_p_e);
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
}

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
fix(entry_pointer p_e)
{
  size_type i = p_e - m_a_entries;
  if (i > 0 && entry_cmp::operator()(m_a_entries[parent(i)], m_a_entries[i]))
    {
      size_type parent_i = parent(i);
      while (i > 0 
	     && entry_cmp::operator()(m_a_entries[parent_i], m_a_entries[i]))
        {
	  std::swap(m_a_entries[i], m_a_entries[parent_i]);
	  i = parent_i;
	  parent_i = parent(i);
        }

      _GLIBCXX_DEBUG_ONLY(assert_valid();)
      return;
    }

  while (i < m_size)
    {
      const size_type left_child_i = left_child(i);
      const size_type right_child_i = right_child(i);
      _GLIBCXX_DEBUG_ASSERT(right_child_i > left_child_i);
      const bool smaller_than_left_child = left_child_i < m_size && 
	entry_cmp::operator()(m_a_entries[i], m_a_entries[left_child_i]);

      const bool smaller_than_right_child = right_child_i < m_size && 
	entry_cmp::operator()(m_a_entries[i], m_a_entries[right_child_i]);

      const bool swap_with_r_child = smaller_than_right_child && (!smaller_than_left_child || entry_cmp::operator()(m_a_entries[left_child_i], m_a_entries[right_child_i]));

      const bool swap_with_l_child = !swap_with_r_child && smaller_than_left_child;

      if (swap_with_l_child)
        {
	  std::swap(m_a_entries[i], m_a_entries[left_child_i]);
	  i = left_child_i;
        }
      else if (swap_with_r_child)
        {
	  std::swap(m_a_entries[i], m_a_entries[right_child_i]);
	  i = right_child_i;
        }
      else
	i = m_size;
    }
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
swap_value_imp(entry_pointer p_e, value_type new_val, true_type)
{
  * p_e = new_val;
}

PB_DS_CLASS_T_DEC
inline void
PB_DS_CLASS_C_DEC::
swap_value_imp(entry_pointer p_e, const_reference r_new_val, false_type)
{
  value_type tmp(r_new_val);
  (*p_e)->swap(tmp);
}
