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
 * @file erase_fn_imps.hpp
 * Contains an implementation class for a pairing heap.
 */

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
pop()
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  _GLIBCXX_DEBUG_ASSERT(!base_type::empty());

  node_pointer p_new_root = join_node_children(base_type::m_p_root);
  _GLIBCXX_DEBUG_ONLY(assert_node_consistent(p_new_root, false);)
  if (p_new_root != 0)
    p_new_root->m_p_prev_or_parent = 0;

  base_type::actual_erase_node(base_type::m_p_root);
  base_type::m_p_root = p_new_root;
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
}

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
erase(point_iterator it)
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  _GLIBCXX_DEBUG_ASSERT(!base_type::empty());
  remove_node(it.m_p_nd);
  base_type::actual_erase_node(it.m_p_nd);
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
}

PB_DS_CLASS_T_DEC
void
PB_DS_CLASS_C_DEC::
remove_node(node_pointer p_nd)
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  _GLIBCXX_DEBUG_ASSERT(!base_type::empty());
  node_pointer p_new_child = join_node_children(p_nd);

#ifdef _GLIBCXX_DEBUG
  if (p_new_child != 0)
    base_type::assert_node_consistent(p_new_child, false);
#endif 

  if (p_nd == base_type::m_p_root)
    {
      if (p_new_child != 0)
	p_new_child->m_p_prev_or_parent = 0;
      base_type::m_p_root = p_new_child;
      _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(base_type::m_p_root, false);)
      return;
    }

  _GLIBCXX_DEBUG_ASSERT(p_nd->m_p_prev_or_parent != 0);
  if (p_nd->m_p_prev_or_parent->m_p_l_child == p_nd)
    {
      if (p_new_child != 0)
        {
	  p_new_child->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
	  p_new_child->m_p_next_sibling = p_nd->m_p_next_sibling;
	  if (p_new_child->m_p_next_sibling != 0)
	    p_new_child->m_p_next_sibling->m_p_prev_or_parent = p_new_child;
	  p_nd->m_p_prev_or_parent->m_p_l_child = p_new_child;
	  _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd->m_p_prev_or_parent, false);)
          return;
        }

      p_nd->m_p_prev_or_parent->m_p_l_child = p_nd->m_p_next_sibling;
      if (p_nd->m_p_next_sibling != 0)
	p_nd->m_p_next_sibling->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
      _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd->m_p_prev_or_parent, false);)
      return;
    }

  if (p_new_child != 0)
    {
      p_new_child->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
      p_new_child->m_p_next_sibling = p_nd->m_p_next_sibling;
      if (p_new_child->m_p_next_sibling != 0)
	p_new_child->m_p_next_sibling->m_p_prev_or_parent = p_new_child;
      p_new_child->m_p_prev_or_parent->m_p_next_sibling = p_new_child;
      _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd->m_p_prev_or_parent, false);)
      return;
    }

  p_nd->m_p_prev_or_parent->m_p_next_sibling = p_nd->m_p_next_sibling;
  if (p_nd->m_p_next_sibling != 0)
    p_nd->m_p_next_sibling->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
  _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd->m_p_prev_or_parent, false);)
}

PB_DS_CLASS_T_DEC
typename PB_DS_CLASS_C_DEC::node_pointer
PB_DS_CLASS_C_DEC::
join_node_children(node_pointer p_nd)
{
  _GLIBCXX_DEBUG_ASSERT(p_nd != 0);
  node_pointer p_ret = p_nd->m_p_l_child;
  if (p_ret == 0)
    return 0;
  while (p_ret->m_p_next_sibling != 0)
    p_ret = forward_join(p_ret, p_ret->m_p_next_sibling);
  while (p_ret->m_p_prev_or_parent != p_nd)
    p_ret = back_join(p_ret->m_p_prev_or_parent, p_ret);
  _GLIBCXX_DEBUG_ONLY(assert_node_consistent(p_ret, false);)
  return p_ret;
}

PB_DS_CLASS_T_DEC
typename PB_DS_CLASS_C_DEC::node_pointer
PB_DS_CLASS_C_DEC::
forward_join(node_pointer p_nd, node_pointer p_next)
{
  _GLIBCXX_DEBUG_ASSERT(p_nd != 0);
  _GLIBCXX_DEBUG_ASSERT(p_nd->m_p_next_sibling == p_next);
  if (Cmp_Fn::operator()(p_nd->m_value, p_next->m_value))
    {
      p_next->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
      base_type::make_child_of(p_nd, p_next);
      return p_next->m_p_next_sibling == 0 
	? p_next : p_next->m_p_next_sibling;
    }

  if (p_next->m_p_next_sibling != 0)
    {
      p_next->m_p_next_sibling->m_p_prev_or_parent = p_nd;
      p_nd->m_p_next_sibling = p_next->m_p_next_sibling;
      base_type::make_child_of(p_next, p_nd);
      return p_nd->m_p_next_sibling;
    }

  p_nd->m_p_next_sibling = 0;
  base_type::make_child_of(p_next, p_nd);
  _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd, false));
  return p_nd;
}

PB_DS_CLASS_T_DEC
typename PB_DS_CLASS_C_DEC::node_pointer
PB_DS_CLASS_C_DEC::
back_join(node_pointer p_nd, node_pointer p_next)
{
  _GLIBCXX_DEBUG_ASSERT(p_nd != 0);
  _GLIBCXX_DEBUG_ASSERT(p_next->m_p_next_sibling == 0);

  if (Cmp_Fn::operator()(p_nd->m_value, p_next->m_value))
    {
      p_next->m_p_prev_or_parent = p_nd->m_p_prev_or_parent;
      base_type::make_child_of(p_nd, p_next);
      _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_next, false));
      return p_next;
    }

  p_nd->m_p_next_sibling = 0;
  base_type::make_child_of(p_next, p_nd);
  _GLIBCXX_DEBUG_ONLY(base_type::assert_node_consistent(p_nd, false));
  return p_nd;
}

PB_DS_CLASS_T_DEC
template<typename Pred>
typename PB_DS_CLASS_C_DEC::size_type
PB_DS_CLASS_C_DEC::
erase_if(Pred pred)
{
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
    if (base_type::empty())
      {
        _GLIBCXX_DEBUG_ONLY(assert_valid();)
	return 0;
      }
  base_type::to_linked_list();
  node_pointer p_out = base_type::prune(pred);
  size_type ersd = 0;
  while (p_out != 0)
    {
      ++ersd;
      node_pointer p_next = p_out->m_p_next_sibling;
      base_type::actual_erase_node(p_out);
      p_out = p_next;
    }

  node_pointer p_cur = base_type::m_p_root;
  base_type::m_p_root = 0;
  while (p_cur != 0)
    {
      node_pointer p_next = p_cur->m_p_next_sibling;
      p_cur->m_p_l_child = p_cur->m_p_next_sibling = p_cur->m_p_prev_or_parent = 0;

      push_imp(p_cur);
      p_cur = p_next;
    }
  _GLIBCXX_DEBUG_ONLY(assert_valid();)
  return ersd;
}

