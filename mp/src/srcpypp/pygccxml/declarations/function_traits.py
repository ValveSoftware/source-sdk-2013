# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines few algorithms, that deals with different properties of functions
"""

from . import calldef
from . import type_traits
from . import class_declaration

def is_same_return_type( f1, f2 ):
    #covariant returns
    #The return type of an overriding function shall be either identical to the
    #return type of the overridden function or covariant with the classes of the
    #functions. If a function D::f overrides a function B::f, the return types
    #of the functions are covariant if they satisfy the following criteria:

    #* both are pointers to classes or references to classes
    #* the class in the return type of B::f is the same class as the class in
    #  the return type of D::f or, is an unambiguous direct or indirect base
    #  class of the class in the return type of D::f and is accessible in D
    #* both pointers or references have the same cv-qualification and the class
    #  type in the return type of D::f has the same cv-qualification as or less
    #  cv-qualification than the class type in the return type of B::f.

    if not f1.__class__ is f2.__class__:
        #it should be assert
        return False #2 different calldef types
    if not isinstance( f1, calldef.member_calldef_t ):
        #for free functions we compare return types as usual
        return type_traits.is_same( f1.return_type, f2.return_type)
    if f1.virtuality == calldef.VIRTUALITY_TYPES.NOT_VIRTUAL \
       or f2.virtuality == calldef.VIRTUALITY_TYPES.NOT_VIRTUAL:
        #for non-virtual member functions we compare types as usual
        return type_traits.is_same( f1.return_type, f2.return_type)
    rt1 = f1.return_type
    rt2 = f2.return_type
    if type_traits.is_pointer( rt1 ) and type_traits.is_pointer( rt2 ):
        rt1 = type_traits.remove_pointer( rt1 )
        rt2 = type_traits.remove_pointer( rt2 )
    elif type_traits.is_reference( rt1 ) and type_traits.is_reference( rt2 ):
        rt1 = type_traits.remove_reference( rt1 )
        rt2 = type_traits.remove_reference( rt2 )
    else:
        return type_traits.is_same( f1.return_type, f2.return_type)
    if ( type_traits.is_const( rt1 ) and type_traits.is_const( rt2 ) ) \
       or ( not type_traits.is_const( rt1 ) and not type_traits.is_const( rt2 ) ):
        rt1 = type_traits.remove_const( rt1 )
        rt2 = type_traits.remove_const( rt2 )
    else:
        return False
    if not type_traits.is_class( rt1 ) or not type_traits.is_class( rt2 ):
        return type_traits.is_same( rt1, rt2 )
    c1 = type_traits.class_traits.get_declaration( rt1 )
    c2 = type_traits.class_traits.get_declaration( rt2 )
    if c1.class_type == class_declaration.CLASS_TYPES.UNION \
       or c2.class_type == class_declaration.CLASS_TYPES.UNION:
        return type_traits.is_same( rt1, rt2 )
    return type_traits.is_same( c1, c2 ) \
           or type_traits.is_base_and_derived( c1, c2 ) \
           or type_traits.is_base_and_derived( c2, c1 )


def is_same_function( f1, f2 ):
    """returns true if f1 and f2 is same function

    Use case: sometimes when user defines some virtual function in base class,
    it overrides it in a derived one. Sometimes we need to know whether two member
    functions is actually same function.
    """
    if f1 is f2:
        return True
    if not f1.__class__ is f2.__class__:
        return False
    if isinstance( f1, calldef.member_calldef_t ) and f1.has_const != f2.has_const:
        return False
    if f1.name != f2.name:
        return False
    if not is_same_return_type( f1, f2 ):
        return False
    if len( f1.arguments ) != len(f2.arguments):
        return False
    for f1_arg, f2_arg in zip( f1.arguments, f2.arguments ):
        if not type_traits.is_same( f1_arg.type, f2_arg.type ):
            return False
    return True



