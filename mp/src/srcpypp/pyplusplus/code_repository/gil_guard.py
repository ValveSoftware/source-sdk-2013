# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains C++ code to acquire/release the GIL.
"""

file_name = "__gil_guard.pypp.hpp"

code = \
"""// Copyright 2004-2008 Roman Yakovenko.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef __gil_guard_pyplusplus_hpp__
#define __gil_guard_pyplusplus_hpp__

namespace pyplusplus{ namespace threading {

class gil_guard_t
{
    public:
    gil_guard_t( bool lock=false )
     : m_locked( false )
    {
        if( lock )
            ensure();
    }

    ~gil_guard_t() {
        release();
    }

    void ensure() {
        if( !m_locked )
        {
             m_gstate = PyGILState_Ensure();
             m_locked = true;
        }
    }

    void release() {
        if( m_locked )
        {
             PyGILState_Release(m_gstate);
             m_locked = false;
        }
    }
     
    private:
    bool m_locked;
    PyGILState_STATE m_gstate;
};

} /* threading */ } /* pyplusplus*/ 


#endif//__gil_guard_pyplusplus_hpp__
"""