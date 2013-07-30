# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines types visitor class interface
"""

class type_visitor_t(object):
    """
    types visitor interface

    All functions within this class should be redefined in derived classes.
    """
    def __init__(self):
        object.__init__(self)

    def visit_void( self ):
        raise NotImplementedError()

    def visit_char( self ):
        raise NotImplementedError()

    def visit_unsigned_char( self ):
        raise NotImplementedError()

    def visit_signed_char( self ):
        raise NotImplementedError()

    def visit_wchar( self ):
        raise NotImplementedError()

    def visit_short_int( self ):
        raise NotImplementedError()

    def visit_short_unsigned_int( self ):
        raise NotImplementedError()

    def visit_bool( self ):
        raise NotImplementedError()

    def visit_int( self ):
        raise NotImplementedError()

    def visit_unsigned_int( self ):
        raise NotImplementedError()

    def visit_long_int( self ):
        raise NotImplementedError()

    def visit_long_unsigned_int( self ):
        raise NotImplementedError()

    def visit_long_long_int( self ):
        raise NotImplementedError()

    def visit_long_long_unsigned_int( self ):
        raise NotImplementedError()

    def visit_int128( self ):
        raise NotImplementedError()

    def visit_uint128( self ):
        raise NotImplementedError()

    def visit_float( self ):
        raise NotImplementedError()

    def visit_double( self ):
        raise NotImplementedError()

    def visit_long_double( self ):
        raise NotImplementedError()

    def visit_complex_long_double(self):
        raise NotImplementedError()

    def visit_complex_double(self):
        raise NotImplementedError()

    def visit_complex_float(self):
        raise NotImplementedError()

    def visit_jbyte(self):
        raise NotImplementedError()

    def visit_jshort(self):
        raise NotImplementedError()

    def visit_jint(self):
        raise NotImplementedError()

    def visit_jlong(self):
        raise NotImplementedError()

    def visit_jfloat(self):
        raise NotImplementedError()

    def visit_jdouble(self):
        raise NotImplementedError()

    def visit_jchar(self):
        raise NotImplementedError()

    def visit_jboolean(self):
        raise NotImplementedError()

    def visit_volatile( self ):
        raise NotImplementedError()

    def visit_const( self ):
        raise NotImplementedError()

    def visit_pointer( self ):
        raise NotImplementedError()

    def visit_reference( self ):
        raise NotImplementedError()

    def visit_array( self ):
        raise NotImplementedError()

    def visit_free_function_type( self ):
        raise NotImplementedError()

    def visit_member_function_type( self ):
        raise NotImplementedError()

    def visit_member_variable_type( self ):
        raise NotImplementedError()

    def visit_declarated( self ):
        raise NotImplementedError()

    def visit_restrict( self ):
        raise NotImplementedError()

    def visit_ellipsis( self ):
        raise NotImplementedError()
