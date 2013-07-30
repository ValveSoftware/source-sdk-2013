# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import compound
from . import code_creator
from . import ctypes_formatter
from . import declaration_based
from pygccxml import declarations

CCT = declarations.CALLING_CONVENTION_TYPES

function_prototype_mapping = {
    CCT.UNKNOWN : 'CFUNCTYPE'
    , CCT.CDECL : 'CFUNCTYPE'
    , CCT.STDCALL : 'WINFUNCTYPE'
    , CCT.THISCALL : 'CPPMETHODTYPE'
    , CCT.FASTCALL : '<<<fastcall unsupported>>>'
    , CCT.SYSTEM_DEFAULT : 'CFUNCTYPE'
}

assert len( function_prototype_mapping ) == len( CCT.all )

class METHOD_MODE:
    STAND_ALONE = "stand alone"
    MULTI_METHOD = "multi method"
    all = [ STAND_ALONE, MULTI_METHOD ]

class opaque_init_introduction_t(code_creator.code_creator_t, declaration_based.declaration_based_t):
    def __init__( self, class_ ):
        code_creator.code_creator_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, class_ )

    def _create_impl(self):
        tmpl = ['def __init__( self, *args, **keywd ):']
        tmpl.append( self.indent('raise RuntimeError( "Unable to create instance of opaque type." )') )
        return os.linesep.join( tmpl )

    def _get_system_files_impl( self ):
        return []

class callable_definition_t(code_creator.code_creator_t, declaration_based.declaration_based_t):
    def __init__( self, callable ):
        code_creator.code_creator_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, callable )

    @property
    def ftype( self ):
        return self.declaration.function_type()

    def join_arguments( self, args, group_in_list=True ):
        args_str = ''
        arg_separator = ', '
        if 1 == len( args ):
            args_str = ' ' + args[0] + ' '
        else:
            args_str = ' ' + arg_separator.join( args ) + ' '
        if args_str.endswith( '  ' ):
            args_str = args_str[:-1]
        if group_in_list:
            return '[%s]' % args_str
        else:
            return args_str

    def restype_code(self):
        if not declarations.is_void( self.ftype.return_type ):
            return ctypes_formatter.as_ctype( self.ftype.return_type, self.top_parent.treat_char_ptr_as_binary_data )
        else:
            return ''

    def argtypes_code(self, group_in_list=True):
        if not self.ftype.arguments_types:
            return ''
        args = [ctypes_formatter.as_ctype( type_, self.top_parent.treat_char_ptr_as_binary_data ) for type_ in self.ftype.arguments_types]
        return self.join_arguments( args, group_in_list )

    def _get_system_files_impl( self ):
        return []


class function_definition_t(callable_definition_t):
    def __init__( self, free_fun ):
        callable_definition_t.__init__( self, free_fun )

    def _create_impl(self):
        global function_prototype_mapping
        result = []
        result.append( '%(alias)s_type = ctypes.%(prototype)s( %(restype)s%(argtypes)s )' )
        result.append( '%(alias)s = %(alias)s_type( ( %(library_var_name)s.undecorated_names["%(undecorated_decl_name)s"], %(library_var_name)s ) )' )

        restype = self.restype_code()
        argtypes = self.argtypes_code( group_in_list=False )

        return os.linesep.join( result ) \
               % dict( alias=self.declaration.alias
                       , prototype=function_prototype_mapping[ self.declaration.calling_convention ]
                       , restype=self.iif( restype, restype, 'None' )
                       , argtypes=self.iif( argtypes, ',' + argtypes, '' )
                       , library_var_name=self.top_parent.library_var_name
                       , undecorated_decl_name=self.undecorated_decl_name )

        return os.linesep.join( result )
