# Copyright 2006 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines controller classes which help to define the function transformation

The idea behind implementation of "Function Transformation" functionality is simple:
`Py++` defines few templates. Transformers are just editors for the templates.
In most cases, transformers don't directly edit the template, but use controller
classes for this purpose. Controller classes provide an abstraction of the templates.
"""

import string
from . import templates
from pygccxml import declarations

class variable_t( object ):
    """defines C++ variable"""
    def __init__( self, type, name, initialize_expr='' ):
        """
        :param type: type of the variable
        :type type: instance of :class:`pygccxml.declarations.type_t`
        
        :param name: name( str ) of the variable
        
        :param initialize_expr: an expression that initialize the variable
        """
        self.__name = name
        self.__type = type
        self.__initialize_expr = initialize_expr
        
    @property
    def name( self ):
        "variable name"
        return self.__name
        
    @property 
    def type( self ):
        "variable type"
        return self.__type
        
    @property 
    def initialize_expr( self ):
        "initialize expression"
        return self.__initialize_expr
        
    def declare_var_string( self ):
        return templates.substitute( "$type $name$initialize_expr;"
                                     , name=self.name
                                     , type=self.type
                                     , initialize_expr=self.initialize_expr )

class variables_manager_t( object ):
    """function wrapper variables manager
    
    Almost every time we define new transformer, we need to define variables.
    It is important to keep the variable names unique. This class will ensure this.
    Every time you declare new variable, it will return the unique variable name.
    The name will be built from the original variable name and some index, which
    will make the variable to be unique.
    """
    def __init__( self ):
        object.__init__( self )
        self.__variables = [] #variables
        self.__names_in_use = set()
    
    @property
    def variables( self ):
        "list of all declared variables"
        return self.__variables
        
    def declare_variable( self, type, name, initialize_expr='' ):
        """declare variable
        
        :param type: type of the variable
        :type type: instance of :class:`pygccxml.declarations.type_t`
        
        :param name: name( str ) of the variable
        
        :param initialize_expr: an expression that initialize the variable

        :returns: the variable name, which is unique in a whole scope, based on "name" argument
        
        :rtype: str
        """
        unique_name = self.__create_unique_var_name( name )
        self.__variables.append( variable_t( type, unique_name, initialize_expr ) )
        return unique_name
    
    def register_name( self, name ):
        """register predefined variable name
        
        There are use cases, where it is convenience to define variables within
        a template. In such cases, the only thing that should be done is registering
        a unique name of the variable.
        """
        return self.__create_unique_var_name( name )
        
    def __create_unique_var_name( self, name ):
        n = 2
        unique_name = name
        while 1:
            if unique_name in self.__names_in_use:
                unique_name = "%s%d" % ( name, n )
                n += 1
            else:
                self.__names_in_use.add( unique_name )
                return unique_name

def create_variables_manager( function ):
    vm = variables_manager_t()
    for arg in function.arguments: vm.register_name( arg.name )
    return vm

class controller_base_t( object ):
    """base class for all controller classes"""
    
    def __init__( self, function ):
        self.__function = function

    @property
    def function( self ):
        return self.__function

    def apply( self, transformations ):
        """asks all transformations to configure the controller"""
        raise NotImplementedError()

class sealed_fun_controller_t( controller_base_t ): 
    """base class for free and member function controllers"""
    def __init__( self, function ):
        controller_base_t.__init__( self, function )
        self.__vars_manager = create_variables_manager( function )
        self.__wrapper_args = [ arg.clone() for arg in function.arguments ]

        initialize_expr = ''
        result_type = self.function.return_type
        if declarations.is_reference( self.function.return_type ):
            initialize_expr = ' = 0'
            result_type = declarations.pointer_t( declarations.remove_reference( self.function.return_type ) )
        self.__result_var = variable_t( result_type
                                        , self.register_variable_name( 'result' )
                                        , initialize_expr=initialize_expr )
        self.__return_variables = []
        self.__pre_call = []
        self.__post_call = []
        self.__arg_expressions = [ arg.name for arg in function.arguments ]

    @property
    def variables( self ):
        return self.__vars_manager.variables
        
    def declare_variable( self, type, name, initialize_expr='' ):
        return self.__vars_manager.declare_variable( type, name, initialize_expr)
    
    def register_variable_name( self, name ):
        return self.__vars_manager.register_name( name )
        
    @property 
    def result_variable( self ):
        return self.__result_var
    
    @property
    def template( self ):
        return templates.sealed_fun.body
        
    @property
    def wrapper_args( self ):
        return [_f for _f in self.__wrapper_args if _f]

    def find_wrapper_arg( self, name ):
        for arg in self.wrapper_args:
            if arg.name == name:
                return arg
        return None

    def remove_wrapper_arg( self, name ):
        arg = self.find_wrapper_arg( name )
        if not arg:
            raise LookupError( "Unable to remove '%s' argument - not found!" % name ) 
        self.__wrapper_args[ self.__wrapper_args.index(arg) ] = None

    @property
    def arg_expressions( self ):
        return self.__arg_expressions

    def modify_arg_expression( self, index, expression ):
        self.arg_expressions[ index ] = expression
    
    @property
    def wrapper_return_type( self ):
        return_vars_count = len( self.return_variables )
        if not declarations.is_void( self.function.return_type ):
            return_vars_count += 1
        if 0 == return_vars_count:
            return self.function.return_type #return type is void
        elif 1 == return_vars_count:
            return declarations.dummy_type_t( 'boost::python::object' )
        else:
            return declarations.dummy_type_t( 'boost::python::tuple' )
        
    @property
    def return_variables( self ):
        return self.__return_variables
    
    def return_variable( self, variable_name ):
        self.__return_variables.append( variable_name )
    
    @property
    def pre_call( self ):
        return self.__pre_call
        
    def add_pre_call_code( self, code ):
        self.__pre_call.append( code )
        
    @property
    def post_call( self ):
        return self.__post_call
    
    def add_post_call_code( self, code ):
        self.__post_call.append( code )
    
class mem_fun_controller_t( sealed_fun_controller_t ):
    def __init__( self, function ):
        sealed_fun_controller_t.__init__( self, function )
        
        inst_arg_type = declarations.declarated_t( self.function.parent )
        if self.function.has_const:
            inst_arg_type = declarations.const_t( inst_arg_type )
        inst_arg_type = declarations.reference_t( inst_arg_type )
        
        self.__inst_arg = None
        if not self.function.has_static:
            self.__inst_arg = declarations.argument_t( name=self.register_variable_name( 'inst' )
                                                       , type=inst_arg_type )

    def apply( self, transformations ):
        for t in transformations: t.configure_mem_fun( self )

    @property 
    def inst_arg( self ):
        return self.__inst_arg

class free_fun_controller_t( sealed_fun_controller_t ):
    def __init__( self, function ):
        sealed_fun_controller_t.__init__( self, function )

    def apply( self, transformations ):
        for t in transformations: t.configure_free_fun( self )


class virtual_mem_fun_controller_t( controller_base_t ):
    class override_fun_controller_t( controller_base_t ):
        def __init__( self, function ):
            controller_base_t.__init__( self, function )
            self.__py_vars_manager = create_variables_manager( function )
            self.__py_function_var \
                = self.__py_vars_manager.register_name( 'func_' + function.alias )
            self.__py_pre_call = []
            self.__py_post_call = []
            self.__py_result_var = variable_t( declarations.dummy_type_t( 'boost::python::object' )
                                               , self.register_py_variable_name( 'py_result' ) )
    
            self.__py_arg_expressions = [ arg.name for arg in function.arguments ]

        @property
        def template( self ):
            return templates.virtual_mem_fun.override

        @property
        def py_variables( self ):
            return self.__py_vars_manager.variables
            
        def declare_py_variable( self, type, name, initialize_expr='' ):
            return self.__py_vars_manager.declare_variable( type, name, initialize_expr)
        
        def register_py_variable_name( self, name ):
            return self.__py_vars_manager.register_name( name )
            
        @property
        def py_function_var( self ):
            return self.__py_function_var
        
        @property
        def py_pre_call( self ):
            return self.__py_pre_call
            
        def add_py_pre_call_code( self, code ):
            self.__py_pre_call.append( code )
            
        @property
        def py_post_call( self ):
            return self.__py_post_call
        
        def add_py_post_call_code( self, code ):
            self.__py_post_call.append( code )
    
        @property
        def py_result_variable( self ):
            return self.__py_result_var
        
        @property
        def py_arg_expressions( self ):
            return [_f for _f in self.__py_arg_expressions if _f]
    
        def remove_py_arg( self, index ):
            self.__py_arg_expressions[ index ] = None
            
        def modify_py_arg_expression( self, index, expression ):
            self.arg_expressions[ index ] = expression
    
    class default_fun_controller_t( sealed_fun_controller_t ):
        def __init__( self, function ):
            sealed_fun_controller_t.__init__( self, function )
            
            inst_arg_type = declarations.declarated_t( self.function.parent )
            if self.function.has_const:
                inst_arg_type = declarations.const_t( inst_arg_type )
            inst_arg_type = declarations.reference_t( inst_arg_type )
            
            self.__inst_arg = declarations.argument_t( name=self.register_variable_name( 'inst' )
                                                       , type=inst_arg_type )

        @property 
        def inst_arg( self ):
            return self.__inst_arg
        
        @property
        def template( self ):
            return templates.virtual_mem_fun.default

    def __init__( self, function ):
        controller_base_t.__init__( self, function )
        self.__override_cntrl = self.override_fun_controller_t( function )
        self.__default_cntrl = self.default_fun_controller_t( function )

    def apply( self, transformations ):
        for t in transformations: t.configure_virtual_mem_fun( self )

    @property
    def override_controller( self ):
        return self.__override_cntrl

    @property
    def default_controller( self ):
        return self.__default_cntrl

class pure_virtual_mem_fun_controller_t( virtual_mem_fun_controller_t ):
    def __init__( self, function ):
        virtual_mem_fun_controller_t.__init__(self, function)
        
    class override_fun_controller_t( virtual_mem_fun_controller_t.override_fun_controller_t ):
        def __init__( self, function ):
            virtual_mem_fun_controller_t.override_fun_controller_t.__init__(self, function)
            
        @property
        def template( self ):
            return templates.pure_virtual_mem_fun.override

    class default_fun_controller_t( virtual_mem_fun_controller_t.default_fun_controller_t ):
        def __init__( self, function ):
            virtual_mem_fun_controller_t.default_fun_controller_t.__init__(self,function)   

        @property
        def template( self ):
            return templates.pure_virtual_mem_fun.default

#TODO: FT for constructor

#~ class constructor_controller_t( controller_base_t ):
    #~ def __init__( self, function ):
        #~ controller_base_t.__init__( self, function )
        #~ self.__vars_manager = create_variables_manager( function )
        #~ self.__wrapper_args = [ arg.clone() for arg in function.arguments ]
        #~ self.__result_var = variable_t( self.wrapper_return_type
                                        #~ , self.register_variable_name( 'result' ) )
        #~ self.__pre_call = []
        #~ self.__post_call = []
        #~ self.__arg_expressions = [ arg.name for arg in function.arguments ]
        
    #~ @property
    #~ def variables( self ):
        #~ return self.__vars_manager.variables
        
    #~ def declare_variable( self, type, name, initialize_expr='' ):
        #~ return self.__vars_manager.declare_variable( type, name, initialize_expr)
    
    #~ def register_variable_name( self, name ):
        #~ return self.__vars_manager.register_name( name )
        
    #~ @property 
    #~ def result_variable( self ):
        #~ return self.__result_var
    
    #~ @property
    #~ def template( self ):
        #~ return templates.constructor.body
        
    #~ @property
    #~ def wrapper_args( self ):
        #~ return filter( None, self.__wrapper_args )

    #~ def find_wrapper_arg( self, name ):
        #~ for arg in self.wrapper_args:
            #~ if arg.name == name:
                #~ return arg
        #~ return None

    #~ def remove_wrapper_arg( self, name ):
        #~ arg = self.find_wrapper_arg( name )
        #~ if not arg:
            #~ raise LookupError( "Unable to remove '%s' argument - not found!" % name ) 
        #~ self.__wrapper_args[ self.__wrapper_args.index(arg) ] = None

    #~ @property
    #~ def arg_expressions( self ):
        #~ return self.__arg_expressions

    #~ def modify_arg_expression( self, index, expression ):
        #~ self.arg_expressions[ index ] = expression
    
    #~ @property
    #~ def wrapper_return_type( self ):
        #~ return declarations.dummy_type_t( 'std::auto_ptr< %s >' % self.function.parent.decl_string )
            
    #~ @property
    #~ def pre_call( self ):
        #~ return self.__pre_call
        
    #~ def add_pre_call_code( self, code ):
        #~ self.__pre_call.append( code )
        
    #~ @property
    #~ def post_call( self ):
        #~ return self.__post_call
    
    #~ def add_post_call_code( self, code ):
        #~ self.__post_call.append( code )

    #~ def apply( self, transformations ):
        #~ map( lambda t: t.configure_mem_fun( self ), transformations )

