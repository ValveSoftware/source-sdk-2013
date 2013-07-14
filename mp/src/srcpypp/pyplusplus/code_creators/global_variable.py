# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import pygccxml
from . import algorithm
from . import code_creator
from . import ctypes_formatter
from . import declaration_based
from . import registration_based
from pygccxml import declarations
from pyplusplus import code_repository
from pyplusplus import decl_wrappers

class global_variable_base_t( registration_based.registration_based_t
                              , declaration_based.declaration_based_t ):
    """
    Base class for all global variables code creators. Mainly exists to
    simplify file writers algorithms.
    """
    def __init__(self, variable, wrapper=None ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)
        self._wrapper = wrapper

    def _get_wrapper( self ):
        return self._wrapper
    def _set_wrapper( self, new_wrapper ):
        self._wrapper = new_wrapper
    wrapper = property( _get_wrapper, _set_wrapper )

    def _get_system_files_impl( self ):
        return []

class global_variable_t( global_variable_base_t ):
    """
    Creates boost.python code that exposes global variable.
    """
    def __init__(self, variable ):
        global_variable_base_t.__init__( self, variable=variable )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''

        assert isinstance( self.declaration, pygccxml.declarations.variable_t )
        result = []
        result.append( algorithm.create_identifier( self, '::boost::python::scope' ) )
        result.append( '().attr("%s")' % self.alias )
        dtype = self.declaration.type
        if decl_wrappers.python_traits.is_immutable( dtype ) \
           or pygccxml.declarations.is_const( dtype ) \
           or pygccxml.declarations.smart_pointer_traits.is_smart_pointer( dtype ):
            result.append( ' = %s;' % self.decl_identifier )
        else:
            obj_identifier = algorithm.create_identifier( self, '::boost::python::object' )
            ref_identifier = algorithm.create_identifier( self, '::boost::ref' )
            result.append( ' = %s( %s( %s ) );' % ( obj_identifier, ref_identifier, self.decl_identifier ) )
        return ''.join( result )

class array_gv_t( global_variable_base_t ):
    """
    Creates boost.python code that exposes array global variable.
    """

    _PARAM_SEPARATOR = ', '
    def __init__(self, variable, wrapper ):
        global_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )

    def _get_system_files_impl( self ):
        return []

    def _create_impl( self ):
        if self.declaration.already_exposed:
            return ''

        answer = []
        answer.append( algorithm.create_identifier( self, '::boost::python::scope' ) )
        answer.append( '().attr("%s")' % self.alias )
        answer.append( ' = ' )
        answer.append( self.wrapper.wrapper_creator_full_name )
        answer.append( '();' )
        return ''.join( answer )

class array_gv_wrapper_t( code_creator.code_creator_t
                          , declaration_based.declaration_based_t ):
    """
    Creates C++ code that register array class.
    """

    def __init__(self, variable ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)

    def _get_wrapper_type( self ):
        ns_name = code_repository.array_1.namespace
        if declarations.is_const( self.declaration.type ):
            class_name = 'const_array_1_t'
        else:
            class_name = 'array_1_t'

        decl_string = declarations.templates.join(
              '::'.join( [ns_name, class_name] )
            , [ declarations.array_item_type( self.declaration.type ).decl_string
                , str( declarations.array_size( self.declaration.type ) )
        ])

        return declarations.dummy_type_t( decl_string )
    wrapper_type = property( _get_wrapper_type )

    def _get_wrapper_creator_type(self):
        return declarations.free_function_type_t.create_decl_string(
                return_type=self.wrapper_type
                , arguments_types=[]
                , with_defaults=False)
    wrapper_creator_type = property( _get_wrapper_creator_type )

    def _get_wrapper_creator_name(self):
        return '_'.join( [self.declaration.name, 'wrapper'] )
    wrapper_creator_name = property( _get_wrapper_creator_name )

    def _create_namespaces(self):
        ns_names = declarations.declaration_path( self.declaration.parent )
        if len(ns_names) >= 1 and ns_names[0] == '::':
            ns_names = ns_names[1:]
        return ns_names

    def _get_wrapper_creator_full_name(self):
        names = self._create_namespaces()
        names.append( self.wrapper_creator_name )
        return '::'.join( names )
    wrapper_creator_full_name = property( _get_wrapper_creator_full_name )

    def _create_namespaces_name(self):
        temp = []
        for ns_name in self._create_namespaces():
            temp.append( ''.join( ['namespace ', ns_name, '{ '] ) )
        return ''.join( temp )

    def _create_impl( self ):
        if self.declaration.already_exposed:
            return ''

        answer = [self._create_namespaces_name()]
        answer.append( self.wrapper_type.decl_string )
        answer.append( ''.join([ self.wrapper_creator_name, '(){']) )
        temp = ''.join([ 'return '
                         , self.wrapper_type.decl_string
                         , '( '
                         , declarations.full_name( self.declaration )
                         , ' );'])
        answer.append( self.indent( temp ) )
        answer.append('}')
        answer.append( '}' * len( self._create_namespaces() ) )
        return os.linesep.join( answer )

    def _get_system_files_impl( self ):
        return [code_repository.array_1.file_name]

class global_variable_addressof_t( global_variable_base_t ):
    """
    Creates boost.python code that exposes address of global variable.

    This functionality is pretty powerful if you use it with "ctypes" -
    standard package.
    """
    def __init__(self, variable ):
        global_variable_base_t.__init__( self, variable=variable )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''

        assert isinstance( self.declaration, pygccxml.declarations.variable_t )
        result = []
        #TODO: porting to 64Bit is welcome
        result.append( algorithm.create_identifier( self, '::boost::python::scope' ) )
        result.append( '().attr("%s")' % self.alias )
        result.append( ' = size_t( boost::addressof( %s ) );' % self.decl_identifier )
        return ''.join( result )

    def _get_system_files_impl( self ):
        return [code_repository.ctypes_integration.file_name]

class global_variable_reference_t( code_creator.code_creator_t, declaration_based.declaration_based_t ):
    def __init__( self, var ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, var )

    def _create_impl( self ):
        return '%(alias)s = %(type)s.in_dll( %(library_var_name)s, %(library_var_name)s.undecorated_names["%(undecorated_decl_name)s"] )' \
               % dict( alias=self.alias
                       , type=ctypes_formatter.as_ctype( self.declaration.type, self.top_parent.treat_char_ptr_as_binary_data )
                       , library_var_name=self.top_parent.library_var_name
                       , undecorated_decl_name=self.undecorated_decl_name )

    def _get_system_files_impl( self ):
        return []
