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
 * @file traits.hpp
 * Contains an implementation class for tree-like classes.
 */

#ifndef PB_DS_NODE_AND_IT_TRAITS_HPP
#define PB_DS_NODE_AND_IT_TRAITS_HPP

#include <ext/pb_ds/detail/types_traits.hpp>
#include <ext/pb_ds/detail/bin_search_tree_/traits.hpp>
#include <ext/pb_ds/detail/tree_policy/node_metadata_selector.hpp>
#include <ext/pb_ds/detail/trie_policy/node_metadata_selector.hpp>

namespace __gnu_pbds
{
  namespace detail
  {    
    template<typename Key,
	     typename Data,
	     class Cmp_Fn,
	     template<typename Const_Node_Iterator,
		      class Node_Iterator,
		      class Cmp_Fn_,
		      class Allocator>
    class Node_Update,
	     class Tag,
	     class Allocator>
    struct tree_traits;

    template<typename Key,
	     typename Data,
	     class E_Access_Traits,
	     template<typename Const_Node_Iterator,
		      class Node_Iterator,
		      class E_Access_Traits_,
		      class Allocator>
    class Node_Update,
	     class Tag,
	     class Allocator>
    struct trie_traits;

  } // namespace detail
} // namespace __gnu_pbds

#include <ext/pb_ds/detail/rb_tree_map_/traits.hpp>
#include <ext/pb_ds/detail/splay_tree_/traits.hpp>
#include <ext/pb_ds/detail/ov_tree_map_/traits.hpp>
#include <ext/pb_ds/detail/pat_trie_/traits.hpp>

#endif // #ifndef PB_DS_NODE_AND_IT_TRAITS_HPP
