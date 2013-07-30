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
 * @file cond_dealtor.hpp
 * Contains a conditional deallocator.
 */

#ifndef PB_DS_COND_DEALTOR_HPP
#define PB_DS_COND_DEALTOR_HPP

namespace __gnu_pbds
{

  namespace detail
  {

#define PB_DS_COND_DEALTOR_CLASS_T_DEC		\
    template<typename Entry, class Allocator>

#define PB_DS_COND_DEALTOR_CLASS_C_DEC				\
    cond_dealtor<						\
						Entry,		\
						Allocator>

    template<typename Entry, class Allocator>
    class cond_dealtor
    {
    public:
      typedef
      typename Allocator::template rebind<Entry>::other
      entry_allocator;

      typedef typename entry_allocator::pointer entry_pointer;

    public:
      inline
      cond_dealtor(entry_pointer p_e);

      inline
      ~cond_dealtor();

      inline void
      set_no_action();

    private:
      entry_pointer m_p_e;

      bool m_no_action_destructor;

      static entry_allocator s_alloc;
    };

    PB_DS_COND_DEALTOR_CLASS_T_DEC
    typename PB_DS_COND_DEALTOR_CLASS_C_DEC::entry_allocator
    PB_DS_COND_DEALTOR_CLASS_C_DEC::s_alloc;

    PB_DS_COND_DEALTOR_CLASS_T_DEC
    inline
    PB_DS_COND_DEALTOR_CLASS_C_DEC::
    cond_dealtor(entry_pointer p_e) :
      m_p_e(p_e),
      m_no_action_destructor(false)
    { }

    PB_DS_COND_DEALTOR_CLASS_T_DEC
    inline void
    PB_DS_COND_DEALTOR_CLASS_C_DEC::
    set_no_action()
    {
      m_no_action_destructor = true;
    }

    PB_DS_COND_DEALTOR_CLASS_T_DEC
    inline
    PB_DS_COND_DEALTOR_CLASS_C_DEC::
    ~cond_dealtor()
    {
      if (m_no_action_destructor)
        return;

      s_alloc.deallocate(m_p_e, 1);
    }

#undef PB_DS_COND_DEALTOR_CLASS_T_DEC
#undef PB_DS_COND_DEALTOR_CLASS_C_DEC

  } // namespace detail

} // namespace __gnu_pbds

#endif // #ifndef PB_DS_COND_DEALTOR_HPP

