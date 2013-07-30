# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains indexing suite v2 code
"""

file_name = "indexing_suite/registry_utils.hpp"

code = """// Header file registry_utils.hpp
//
// provides few convenience functions for dealing with boost:python registry
//
// Copyright (c) 2010 Roman Yakovenko
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)
//
// History
// =======
// 2010/01/12   Roman     File creation
//

#ifndef REGISTRY_UTILS_12_01_2010_HPP
#define REGISTRY_UTILS_12_01_2010_HPP

#include <boost/config.hpp>
#include "boost/python.hpp"
#include "boost/python/converter/registry.hpp"

namespace boost{ namespace python{ namespace registry{ namespace utils{

template<class T>
bool is_registered(){
    namespace bpl = boost::python;
    bpl::handle<> class_obj( bpl::objects::registered_class_object( bpl::type_id< T >()));
    return class_obj.get() ? true : false;
}

template< class T >
void register_alias( const char* name ){
    namespace bpl = boost::python;
    bpl::handle<> class_obj( bpl::objects::registered_class_object( bpl::type_id< T >()));
    boost::python::scope().attr( name ) = bpl::object( class_obj );
}

}}}}

#endif // REGISTRY_UTILS_12_01_2010_HPP


"""
