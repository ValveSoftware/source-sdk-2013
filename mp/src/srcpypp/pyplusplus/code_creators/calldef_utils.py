# Copyright 2004-2008 Roman Yakovenko
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import code_creator
from pygccxml import declarations
from pyplusplus import decl_wrappers
#virtual functions that returns const reference to something
#could not be overriden by Python. The reason is simple:
#in boost::python::override::operator(...) result of marshaling
#(Python 2 C++) is saved on stack, after functions returns the result
#will be reference to no where - access violetion.
#For example see temporal variable tester

use_enum_workaround = False

class argument_utils_t:

    PARAM_SEPARATOR = code_creator.code_creator_t.PARAM_SEPARATOR

    def __init__( self, declaration, identifier_creator, arguments=None ):
        self.__decl = declaration
        if None is arguments:
            arguments = self.__decl.arguments
        self.__args = arguments
        self.__id_creator = identifier_creator

    def __should_use_enum_wa( self, arg ):
        global use_enum_workaround
        if not declarations.is_enum( arg.type ):
            return False
        if use_enum_workaround:
            return True
        #enum belongs to the class we are working on
        if self.__decl.parent is declarations.enum_declaration( arg.type ).parent \
           and isinstance( self.__decl, declarations.constructor_t ):
            return True
        return False

    def keywords_args(self):
        if not self.__args:
            return ''
        boost_arg = self.__id_creator( '::boost::python::arg' )
        boost_obj = self.__id_creator( '::boost::python::object' )
        result = ['( ']
        for arg in self.__args:
            if 1 < len( result ):
                result.append( self.PARAM_SEPARATOR )
            result.append( boost_arg )
            result.append( '("%s")' % arg.name )
            if self.__decl.use_default_arguments and arg.default_value:
                if not declarations.is_pointer( arg.type ) or arg.default_value != '0':
                    arg_type_no_alias = declarations.remove_alias( arg.type )
                    if declarations.is_fundamental( arg_type_no_alias ) \
                       and declarations.is_integral( arg_type_no_alias ) \
                       and not arg.default_value.startswith( arg_type_no_alias.decl_string ):
                        result.append( '=(%s)(%s)' % ( arg.type.partial_decl_string
                                                       , arg.default_value ) )
                    elif self.__should_use_enum_wa( arg ):
                        #Work around for bug/missing functionality in boost.python.
                        #registration order
                        result.append( '=(long)(%s)' % arg.default_value )
                    else:
                        result.append( '=%s' % arg.default_value )
                else:
                    result.append( '=%s()' % boost_obj )
        result.append( ' )' )
        return ''.join( result )

    def argument_name( self, index ):
        arg = self.__args[ index ]
        if arg.name:
            return arg.name
        else:
            return 'p%d' % index

    def args_declaration( self ):
        args = []
        for index, arg in enumerate( self.__args ):
            result = arg.type.partial_decl_string + ' ' + self.argument_name(index)
            if arg.default_value:
                result += '=%s' % arg.default_value
            args.append( result )
        if len( args ) == 1:
            return args[ 0 ]
        return self.PARAM_SEPARATOR.join( args )

    def call_args( self ):
        params = []
        for index, arg in enumerate( self.__args ):
            params.append( decl_wrappers.python_traits.call_traits( arg.type )
                           % self.argument_name( index ) )
        return ', '.join( params )

class return_stmt_creator_t( object ):
    def __init__( self, creator, controller, result_var, return_vars ):
        object.__init__( self )
        self.__creator = creator
        self.__controller = controller
        self.__function = controller.function
        self.__return_vars = return_vars[:]
        self.__pre_return_code = None
        self.__return_stmt = None
        self.__result_var = result_var
        self.__call_policy_alias = controller.register_variable_name( 'call_policies_t' )

    @property
    def pre_return_code( self ):
        if None is self.__pre_return_code:
            if declarations.is_void( self.__function.return_type ) \
               and ( self.__function.call_policies.is_default() \
                     or False == bool( self.__controller.return_variables ) ):
                self.__pre_return_code = ''
            elif self.__function.call_policies.is_default():
                self.__pre_return_code = ''
            else:
                c_p_typedef = 'typedef %s %s;' \
                              % ( self.__function.call_policies.create_template_arg( self.__creator )
                                  , self.__call_policy_alias )
                self.__pre_return_code = c_p_typedef
        return self.__pre_return_code

    @property
    def statement( self ):
        if None is self.__return_stmt:
            stmt = ''
            bpl_object = algorithm.create_identifier( self.__creator, 'boost::python::object' )
            make_tuple = algorithm.create_identifier( self.__creator, 'boost::python::make_tuple' )
            make_object = algorithm.create_identifier( self.__creator, 'pyplusplus::call_policies::make_object' )

            if not declarations.is_void( self.__function.return_type ):
                if self.__function.call_policies.is_default():
                    self.__return_vars.insert( 0, self.__result_var.name )
                else:
                    self.__return_vars.insert( 0
                            , declarations.call_invocation.join(
                                declarations.templates.join( make_object
                                                            , [self.__call_policy_alias, self.__result_var.type.decl_string] )
                                , [self.__result_var.name] ) )

            if 0 == len( self.__return_vars ):
                pass
            elif 1 == len( self.__return_vars ):
                stmt = bpl_object + '( %s )' % self.__return_vars[ 0 ]
            else: # 1 <
                stmt = declarations.call_invocation.join( make_tuple, self.__return_vars )
                if self.__creator.LINE_LENGTH < len( stmt ):
                    stmt = declarations.call_invocation.join(
                                  make_tuple
                                , self.__return_vars
                                , os.linesep + self.__creator.indent( self.__creator.PARAM_SEPARATOR, 6 ) )

            if stmt:
                stmt = 'return ' + stmt + ';'
            self.__return_stmt = stmt
        return self.__return_stmt
