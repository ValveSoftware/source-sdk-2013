# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines few "type traits" functions related to C++ Python bindings"""

from pygccxml import declarations

def is_immutable( type_ ):
    """returns True, if `type_` represents Python immutable type"""
    return declarations.is_fundamental( type_ )      \
           or declarations.is_enum( type_ )          \
           or declarations.is_std_string( type_ )    \
           or declarations.is_std_wstring( type_ )   \
           or declarations.smart_pointer_traits.is_smart_pointer( type_ )
           #todo is_complex, ...

def call_traits( type_ ):
    """http://boost.org/libs/utility/call_traits.htm"""
    type_ = declarations.remove_alias( type_ )
    if is_immutable( type_ ):
        return "%s" #pass by value
    elif declarations.is_reference( type_ ):
        no_ref = declarations.remove_reference( type_ )
        if is_immutable( no_ref ):
            return "%s" #pass by value
        else:
            return "boost::ref(%s)" #pass by ref
    elif declarations.is_pointer( type_ ) \
         and not is_immutable( type_.base ) \
         and not declarations.is_pointer( type_.base ):
        return "boost::python::ptr(%s)" #pass by ptr
    else:
        return "%s" #pass by value
