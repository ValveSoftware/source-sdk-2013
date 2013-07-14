# Copyright 2004-2008 Roman Yakovenko
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import code_creator
from . import calldef_utils
from . import class_declaration
from pygccxml import declarations
from pyplusplus import code_repository
from .calldef import calldef_t, calldef_wrapper_t
import pyplusplus.function_transformers as function_transformers

#TODO: constructors also can have transformation defined. We should use make _init
# function for this purpose

def remove_duplicate_linesep( code ):
    lines = code.split( os.linesep )
    lines = [line for line in lines if line.strip()]
    return os.linesep.join( lines )
    
class sealed_fun_transformed_t( calldef_t ):
    def __init__( self, function, wrapper=None ):
        calldef_t.__init__( self, function=function, wrapper=wrapper )

    @property
    def ft( self ): #function transformation
        return self.declaration.transformations[0]

    @property 
    def controller( self ):
        return self.ft.controller
    
    def _get_alias_impl( self ):
        return self.wrapper.ft.alias
    
    def create_function_type_alias_code( self, exported_class_alias=None  ):
        ftype = self.wrapper.function_type()
        return 'typedef %s;' % ftype.create_typedef( self.function_type_alias, exported_class_alias )

    def create_function_ref_code(self, use_function_alias=False):
        full_name = self.wrapper.full_name()

        if use_function_alias:
            return '%s( &%s )' \
                   % ( self.function_type_alias, full_name )
        elif self.declaration.create_with_signature:
            func_type = self.wrapper.function_type()
            return '(%s)( &%s )' % ( func_type, full_name )
        else:
            return '&%s' % full_name

    def create_call_policies( self ):
        return ''

class sealed_fun_transformed_wrapper_t( calldef_wrapper_t ):
    def __init__( self, function ):
        calldef_wrapper_t.__init__( self, function=function )

    @property
    def ft( self ): #function transformation
        return self.declaration.transformations[0]

    @property 
    def controller( self ):
        return self.ft.controller

    def resolve_function_ref( self ):
        raise NotImplementedError()

    def create_fun_definition(self):
        cntrl = self.controller

        make_object = algorithm.create_identifier( self, 'pyplusplus::call_policies::make_object' )
        make_tuple = algorithm.create_identifier( self, 'boost::python::make_tuple' )
        
        tmpl_values = dict()

        tmpl_values['unique_function_name'] = self.wrapper_name()
        tmpl_values['return_type'] = self.controller.wrapper_return_type.decl_string
        tmpl_values['arg_declarations'] = self.args_declaration()
        
        tmpl_values['declare_variables'] \
            = os.linesep + os.linesep.join( [self.indent( var.declare_var_string() ) for var in cntrl.variables] )
                
        tmpl_values['pre_call'] = os.linesep + self.indent( os.linesep.join( cntrl.pre_call ) )

        tmpl_values['save_result'] = ''
        if not declarations.is_void( self.declaration.return_type ):
            tmpl_tmp = '%(type)s %(name)s = '
            if declarations.is_reference( self.declaration.return_type ):
                tmpl_tmp = tmpl_tmp + '&'

            tmpl_values['save_result'] = tmpl_tmp \
                                         % { 'type': cntrl.result_variable.type.decl_string
                                             , 'name' : cntrl.result_variable.name }

        tmpl_values['function_name'] = self.resolve_function_ref()
        tmpl_values['arg_expressions'] = self.PARAM_SEPARATOR.join( cntrl.arg_expressions )
        return_stmt_creator = calldef_utils.return_stmt_creator_t( self
                                    , self.controller
                                    , self.controller.result_variable
                                    , self.controller.return_variables )

        tmpl_values['post_call'] = os.linesep + self.indent( os.linesep.join( cntrl.post_call ) )
        if return_stmt_creator.pre_return_code:
            tmpl_values['post_call'] \
                = os.linesep.join([ tmpl_values['post_call']
                                    , self.indent( return_stmt_creator.pre_return_code )])
        tmpl_values['return'] = os.linesep + self.indent( return_stmt_creator.statement )
            
        f_def = self.controller.template.substitute(tmpl_values)
        return remove_duplicate_linesep( f_def )
        
    def _create_impl(self):
        return self.create_fun_definition()
        
class free_fun_transformed_t( sealed_fun_transformed_t ):
    """Creates code for public non-virtual member functions.
    """

    def __init__( self, function, wrapper=None ):
        sealed_fun_transformed_t.__init__( self, function=function, wrapper=wrapper )
        self.works_on_instance = False

    def create_def_code( self ):
        return self.def_identifier()

    def create_keywords_args(self):
        arg_utils = calldef_utils.argument_utils_t( self.declaration
                                                   , algorithm.make_id_creator( self )
                                                   , self.controller.wrapper_args )
        return arg_utils.keywords_args()


class free_fun_transformed_wrapper_t( sealed_fun_transformed_wrapper_t ):
    def __init__( self, function ):
        """Constructor.

        :param function: Function declaration
        :type function: calldef_t
        """
        sealed_fun_transformed_wrapper_t .__init__( self, function=function )
        
    def function_type(self):
        return declarations.free_function_type_t(
                  return_type=self.controller.wrapper_return_type
                , arguments_types=[ arg.type for arg in self.controller.wrapper_args ] )

    def wrapper_name( self ):
        return self.ft.unique_name

    def full_name(self):
        return self.ft.unique_name

    def args_declaration( self ):
        arg_utils = calldef_utils.argument_utils_t( 
                          self.declaration
                        , algorithm.make_id_creator( self )
                        , self.controller.wrapper_args )
        return arg_utils.args_declaration()

    def create_declaration(self, name):
        template = 'static %(return_type)s %(name)s( %(args)s )'

        return template % {
            'return_type' : self.controller.wrapper_return_type.decl_string
            , 'name' : self.wrapper_name()
            , 'args' : self.args_declaration()
        }

    def resolve_function_ref( self ):
        return declarations.full_name( self.declaration )


class mem_fun_transformed_t( sealed_fun_transformed_t ):
    """Creates code for public non-virtual member functions.
    """
    def __init__( self, function, wrapper=None ):
        sealed_fun_transformed_t.__init__( self, function=function, wrapper=wrapper )

    def create_keywords_args(self):
        args = self.controller.wrapper_args[:]
        if self.controller.inst_arg:
            args.insert( 0, self.controller.inst_arg )

        arg_utils = calldef_utils.argument_utils_t( self.declaration
                                                   , algorithm.make_id_creator( self )
                                                   , args )
        return arg_utils.keywords_args()

class mem_fun_transformed_wrapper_t( sealed_fun_transformed_wrapper_t ):
    def __init__( self, function ):
        """Constructor.

        :param function: Function declaration
        :type function: calldef_t
        """
        sealed_fun_transformed_wrapper_t.__init__( self, function=function )

    def __is_global( self ):
        return not isinstance( self.parent, class_declaration.class_wrapper_t )

    def function_type(self):
        args = [arg.type for arg in self.controller.wrapper_args] 
        if self.controller.inst_arg:
            args.insert( 0, self.controller.inst_arg.type )
        return declarations.free_function_type_t(
                  return_type=self.controller.wrapper_return_type
                , arguments_types=args )

    def wrapper_name( self ):
        if self.__is_global():
            return self.ft.unique_name
        else:
            if self.declaration.overloads: 
                #it is possible that other functions will have same signature
                return self.ft.unique_name
            else:
                return self.declaration.name            

    def full_name(self):
        if self.__is_global():
            return self.ft.unique_name
        else:
            return self.parent.full_name + '::' + self.wrapper_name()

    def args_declaration( self ):
        args = self.controller.wrapper_args[:]
        if self.controller.inst_arg:
            args.insert( 0, self.controller.inst_arg )
        arg_utils = calldef_utils.argument_utils_t( 
                          self.declaration
                        , algorithm.make_id_creator( self )
                        , args )
        return arg_utils.args_declaration()

    def resolve_function_ref( self ):
        if self.controller.inst_arg:
            return self.controller.inst_arg.name + '.' + self.declaration.name
        else:
            return declarations.full_name( self.declaration )

class mem_fun_v_transformed_t( calldef_t ):
    def __init__( self, function, wrapper=None ):
        calldef_t.__init__( self, function=function, wrapper=wrapper )

    @property
    def ft( self ): #function transformation
        return self.declaration.transformations[0]

    @property 
    def controller( self ):
        return self.ft.controller
    
    @property
    def function_type_alias( self ):
        return 'default_' + self.alias + '_function_type'

    def _get_alias_impl( self ):
        return self.wrapper.ft.alias
    
    def create_function_type_alias_code( self, exported_class_alias=None ):
        ftype = self.wrapper.default_function_type()
        return 'typedef %s;' % ftype.create_typedef( self.function_type_alias )

    def create_keywords_args(self):
        cntrl = self.controller.default_controller
        arg_utils = calldef_utils.argument_utils_t( self.declaration
                                                   , algorithm.make_id_creator( self )
                                                   , [cntrl.inst_arg] + cntrl.wrapper_args )
        return arg_utils.keywords_args()

    def create_function_ref_code(self, use_function_alias=False):
        full_name = self.wrapper.default_full_name()
        if use_function_alias:
            return '%s( &%s )' % ( self.function_type_alias, full_name )
        elif self.declaration.create_with_signature:
            func_type = self.wrapper.default_function_type()
            return '(%s)( &%s )' % ( func_type, full_name )
        else:
            return '&%s' % full_name

    def create_call_policies( self ):
        return ''

class mem_fun_v_transformed_wrapper_t( calldef_wrapper_t ):
    def __init__( self, function ):
        calldef_wrapper_t.__init__( self, function=function )

    @property
    def ft( self ): #function transformation
        return self.declaration.transformations[0]

    @property 
    def controller( self ):
        return self.ft.controller

    def default_name(self):
        if self.declaration.overloads: 
            #it is possible that other functions will have same signature
            return 'default_' + self.ft.unique_name
        else:
            return 'default_' + self.declaration.alias 

    def default_full_name(self):
        return self.parent.full_name + '::' + self.default_name()

    def args_override_declaration( self ):
        return self.args_declaration()

    def args_default_declaration( self ):
        cntrl = self.controller.default_controller
        arg_utils = calldef_utils.argument_utils_t( self.declaration
                                                    , algorithm.make_id_creator( self )
                                                    , [cntrl.inst_arg] + cntrl.wrapper_args )
        return arg_utils.args_declaration()

    def default_function_type(self):
        cntrl = self.controller.default_controller
        args = [cntrl.inst_arg.type] + [arg.type for arg in cntrl.wrapper_args] 
        return declarations.free_function_type_t( return_type=cntrl.wrapper_return_type
                                                  , arguments_types=args )

    def create_default(self):
        cntrl = self.controller.default_controller

        make_object = algorithm.create_identifier( self, 'pyplusplus::call_policies::make_object' )
        make_tuple = algorithm.create_identifier( self, 'boost::python::make_tuple' )
        
        tmpl_values = dict()

        tmpl_values['unique_function_name'] = self.default_name()
        tmpl_values['return_type'] = cntrl.wrapper_return_type.decl_string
        tmpl_values['arg_declarations'] = self.args_default_declaration()        
        tmpl_values['wrapper_class'] = self.parent.wrapper_alias
        tmpl_values['wrapped_class'] = declarations.full_name( self.declaration.parent )
        tmpl_values['wrapped_inst'] = cntrl.inst_arg.name
        tmpl_values['wrapped_inst_constness'] = ''
        if declarations.is_const( declarations.remove_reference( cntrl.inst_arg.type ) ):
            tmpl_values['wrapped_inst_constness'] = 'const'
            
        decl_vars = cntrl.variables[:]
        if not declarations.is_void( self.declaration.return_type ):
            decl_vars.append( cntrl.result_variable )
        tmpl_values['declare_variables'] \
            = os.linesep + os.linesep.join( [self.indent( var.declare_var_string() ) for var in decl_vars] )
                
        tmpl_values['pre_call'] = os.linesep + self.indent( os.linesep.join( cntrl.pre_call ) )

        tmpl_values['save_result'] = ''
        if not declarations.is_void( self.declaration.return_type ):
            tmpl_tmp = '%(result_var_name)s = '
            if declarations.is_reference( self.declaration.return_type ):
                tmpl_tmp = tmpl_tmp + '&'
            tmpl_values['save_result'] = tmpl_tmp % dict( result_var_name=cntrl.result_variable.name )

        tmpl_values['function_name'] = self.declaration.name
        tmpl_values['arg_expressions'] = self.PARAM_SEPARATOR.join( cntrl.arg_expressions )
        return_stmt_creator = calldef_utils.return_stmt_creator_t( self
                                    , cntrl
                                    , cntrl.result_variable
                                    , cntrl.return_variables )

        tmpl_values['post_call'] = os.linesep + self.indent( os.linesep.join( cntrl.post_call ) )
        if return_stmt_creator.pre_return_code:
            tmpl_values['post_call'] \
                = os.linesep.join([ tmpl_values['post_call']
                                    , self.indent( return_stmt_creator.pre_return_code )])
        tmpl_values['return'] = os.linesep + self.indent( return_stmt_creator.statement )
            
        f_def = cntrl.template.substitute(tmpl_values)
        return remove_duplicate_linesep( f_def )

    def wrapped_class_identifier( self ):
        return algorithm.create_identifier( self, declarations.full_name( self.declaration.parent ) )

    def create_override(self):
        cntrl = self.controller.override_controller       
        
        tmpl_values = dict()
        tmpl_values['return_type' ] = self.declaration.return_type.decl_string 
        tmpl_values['function_name'] = self.declaration.name
        tmpl_values['arg_declarations'] = self.args_override_declaration()
        tmpl_values['constness'] = ''
        if self.declaration.has_const:
            tmpl_values['constness'] = ' const '
        tmpl_values['throw'] = self.throw_specifier_code()        
        tmpl_values['py_function_var'] = cntrl.py_function_var
        tmpl_values['function_alias'] = self.declaration.alias
        tmpl_values['declare_py_variables'] \
            = os.linesep + os.linesep.join( [self.indent( var.declare_var_string(), 2 ) for var in cntrl.py_variables] )

        tmpl_values['py_pre_call'] = os.linesep + self.indent( os.linesep.join( cntrl.py_pre_call ), 2 )
        tmpl_values['py_post_call'] = os.linesep + self.indent( os.linesep.join( cntrl.py_post_call ), 2 )
        tmpl_values['py_arg_expressions'] = ''
        if cntrl.py_arg_expressions:
            tmpl_values['py_arg_expressions'] \
                = ', ' + self.PARAM_SEPARATOR.join( cntrl.py_arg_expressions )
            
        tmpl_values['save_py_result'] = "bpl::object %s = " % cntrl.py_result_variable.name
        tmpl_values['py_return'] = ''
        tmpl_values['cpp_return'] = ''
        if not declarations.is_void( self.declaration.return_type ):
            tmpl_values['py_return'] \
                = 'return bpl::extract< %(type)s >( pyplus_conv::get_out_argument( %(py_result)s, 0 ) );' \
                  % { 'type' : self.declaration.return_type.decl_string
                    , 'py_result' : cntrl.py_result_variable.name }
            tmpl_values['cpp_return'] = 'return '

        tmpl_values['wrapped_class'] = self.wrapped_class_identifier()

        arg_utils = calldef_utils.argument_utils_t( self.declaration
                                                   , algorithm.make_id_creator( self )
                                                   , self.declaration.arguments )
        tmpl_values['cpp_arg_expressions'] = arg_utils.call_args()

        f_def_code = cntrl.template.substitute(tmpl_values)
        return remove_duplicate_linesep( f_def_code )
        
    def _create_impl(self):
        return os.linesep.join([ self.create_override(), '', self.create_default() ])

    def _get_system_files_impl( self ):
        files = super( mem_fun_v_transformed_wrapper_t, self )._get_system_files_impl()
        files.append( code_repository.convenience.file_name )
        return files


class constructor_transformed_t( calldef_t ):
    def __init__( self, constructor, wrapper=None ):
        calldef_t.__init__( self, function=constructor, wrapper=wrapper )

    @property
    def ft( self ): #function transformation
        return self.declaration.transformations[0]

    @property 
    def controller( self ):
        return self.ft.controller
        
    def create_call_policies( self ):
        return ''

    def _create_impl( self ):
        make_constructor = algorithm.create_identifier( self, 'boost::python::make_constructor' )
        
        code = 'def( "__init__, %s( &::%s ) )' % self.wrapper.wrapper_name()
        if not self.works_on_instance:
            code = self.parent.class_var_name + '.' + code + ';'
        return code

#TODO: FT for constructor
#~ class constructor_transformed_wrapper_t( calldef_wrapper_t ):
    #~ def __init__( self, constructor ):
        #~ calldef_wrapper_t.__init__( self, function=constructor )

    #~ @property
    #~ def ft( self ): #function transformation
        #~ return self.declaration.transformations[0]

    #~ @property 
    #~ def controller( self ):
        #~ return self.ft.controller

    #~ def resolve_function_ref( self ):
        #~ raise NotImplementedError()

    #~ def wrapper_name( self ):
        #~ return self.ft.unique_name()

    #~ def create_fun_definition(self):
        #~ cntrl = self.controller

        #~ tmpl_values = dict()

        #~ tmpl_values['unique_function_name'] = self.wrapper_name()
        #~ tmpl_values['exposed_class'] = self.decl_identifier
        #~ tmpl_values['arg_declarations'] = self.args_declaration()
        #~ tmpl_values['result'] = self.ft.result_variable
        
        #~ tmpl_values['declare_variables'] \
            #~ = os.linesep + os.linesep.join( map( lambda var: self.indent( var.declare_var_string() )
                                                 #~ , cntrl.variables ) )
                
        #~ tmpl_values['pre_call'] = os.linesep + self.indent( os.linesep.join( cntrl.pre_call ) )

        #~ tmpl_values['arg_expressions'] = self.PARAM_SEPARATOR.join( cntrl.arg_expressions )
        #~ return_stmt_creator = calldef_utils.return_stmt_creator_t( self
                                    #~ , self.controller
                                    #~ , self.controller.result_variable
                                    #~ , self.controller.return_variables )

        #~ tmpl_values['post_call'] = os.linesep + self.indent( os.linesep.join( cntrl.post_call ) )
        #~ if return_stmt_creator.pre_return_code:
            #~ tmpl_values['post_call'] \
                #~ = os.linesep.join([ tmpl_values['post_call']
                                    #~ , self.indent( return_stmt_creator.pre_return_code )])
            
        #~ f_def = self.controller.template.substitute(tmpl_values)
        #~ return remove_duplicate_linesep( f_def )
        
    #~ def _create_impl(self):
        #~ return self.create_fun_definition()
