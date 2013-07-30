# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines types visitor class interface
"""

from . import algorithm
from pygccxml import declarations

class type_converter_t(declarations.type_visitor_t):
    """
    types visitor interface

    All functions within this class should be redefined in derived classes.
    """
    def __init__(self, type_, treat_char_ptr_as_binary_data, decl_formatter=algorithm.complete_py_name):
        declarations.type_visitor_t.__init__(self)
        self.user_type = type_
        self.decl_formatter = decl_formatter
        self.treat_char_ptr_as_binary_data = treat_char_ptr_as_binary_data

    def create_converter( self, type_):
        return type_converter_t( type_, self.treat_char_ptr_as_binary_data, self.decl_formatter )
            
    def visit_void( self ):
        return "None"

    def visit_char( self ):
        return "ctypes.c_char"

    def visit_unsigned_char( self ):
        return "ctypes.c_ubyte"

    def visit_signed_char( self ):
        return "ctypes.c_byte"

    def visit_wchar( self ):
        return "ctypes.c_wchar"

    def visit_short_int( self ):
        return "ctypes.c_short"

    def visit_short_unsigned_int( self ):
        return "ctypes.c_ushort"

    def visit_bool( self ):
        return "ctypes.c_bool"

    def visit_int( self ):
        return "ctypes.c_int"

    def visit_unsigned_int( self ):
        return "ctypes.c_uint"

    def visit_long_int( self ):
        return "ctypes.c_long"

    def visit_long_unsigned_int( self ):
        return "ctypes.c_ulong"

    def visit_long_long_int( self ):
        return "ctypes.c_longlong"

    def visit_long_long_unsigned_int( self ):
        return "ctypes.c_ulonglong"

    def visit_float( self ):
        return "ctypes.c_float"

    def visit_double( self ):
        return "ctypes.c_double"

    def visit_long_double( self ):
        return "ctypes.c_longdouble"

    #skip complex and jxxx types

    def visit_volatile( self ):
        base_visitor = self.create_converter( self.user_type.base )
        return declarations.apply_visitor( base_visitor, base_visitor.user_type )

    def visit_const( self ):
        base_visitor = self.create_converter( self.user_type.base )
        return declarations.apply_visitor( base_visitor, base_visitor.user_type )

    def visit_pointer( self ):
        no_ptr = declarations.remove_const( declarations.remove_pointer( self.user_type ) )
        if declarations.is_same( declarations.char_t(), no_ptr ) and self.treat_char_ptr_as_binary_data == False:
            return "ctypes.c_char_p"
        elif declarations.is_same( declarations.wchar_t(), no_ptr ) and self.treat_char_ptr_as_binary_data == False:
            return "ctypes.c_wchar_p"
        elif declarations.is_same( declarations.void_t(), no_ptr ):
            return "ctypes.c_void_p"
        else:
            base_visitor = self.create_converter( self.user_type.base )
            internal_type_str = declarations.apply_visitor( base_visitor, base_visitor.user_type )
            if declarations.is_calldef_pointer( self.user_type ):
                return internal_type_str
            else:
                return "ctypes.POINTER( %s )" % internal_type_str

    def visit_reference( self ):
        no_ref = declarations.remove_const( declarations.remove_reference( self.user_type ) )
        if declarations.is_same( declarations.char_t(), no_ref ):
            return "ctypes.c_char_p"
        elif declarations.is_same( declarations.wchar_t(), no_ref ):
            return "ctypes.c_wchar_p"
        elif declarations.is_same( declarations.void_t(), no_ref ):
            return "ctypes.c_void_p"
        else:
            base_visitor = self.create_converter( self.user_type.base )
            internal_type_str = declarations.apply_visitor( base_visitor, base_visitor.user_type )
            return "ctypes.POINTER( %s )" % internal_type_str

    def visit_array( self ):
        item_visitor = self.create_converter( declarations.array_item_type(self.user_type) )
        item_type = declarations.apply_visitor( item_visitor, item_visitor.user_type )
        size = declarations.array_size( self.user_type )
        if size == declarations.array_t.SIZE_UNKNOWN:
            size = 0
        return "( %s * %d )" % ( item_type, size )

    def visit_free_function_type( self ):
        return_visitor = self.create_converter( self.user_type.return_type )
        return_type = declarations.apply_visitor( return_visitor, self.user_type.return_type )
        argtypes = []
        for arg in self.user_type.arguments_types:
            arg_visitor = self.create_converter( arg )
            argtypes.append( declarations.apply_visitor(arg_visitor, arg) )
        return declarations.call_invocation.join( "ctypes.CFUNCTYPE", [return_type] + argtypes )

    #~ def visit_member_function_type( self ):
        #~ raise NotImplementedError()

    #~ def visit_member_variable_type( self ):
        #~ raise NotImplementedError()

    def visit_declarated( self ):
        #TODO: the follwoing code removes typedefs
        if isinstance( self.user_type.declaration, declarations.typedef_t ):
            base_visitor = self.create_converter( self.user_type.declaration.type )
            return declarations.apply_visitor( base_visitor, base_visitor.user_type )
        else:
            return self.decl_formatter( self.user_type.declaration )

    def visit_restrict( self ):
        base_visitor = self.create_converter( self.user_type.base )
        return declarations.apply_visitor( base_visitor, base_visitor.user_type )

    def visit_ellipsis( self ):
        return ''

def as_ctype( type_, treat_char_ptr_as_binary_data=False):
    v = type_converter_t( type_, treat_char_ptr_as_binary_data )
    return declarations.apply_visitor( v, type_ )

