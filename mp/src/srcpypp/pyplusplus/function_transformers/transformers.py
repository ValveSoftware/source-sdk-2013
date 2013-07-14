# Copyright 2006 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
# Initial author: Matthias Baas

"""defines few argument transformation classes"""

import os
import string
from . import transformer
from . import controllers
from pygccxml import declarations
from pyplusplus import code_repository

#TODO: pointers should be checked for NULL

def is_ref_or_ptr( type_ ):
    return declarations.is_pointer( type_ ) or declarations.is_reference( type_ )

def is_ptr_or_array( type_ ):
    return declarations.is_pointer( type_ ) or declarations.is_array( type_ )

def remove_ref_or_ptr( type_ ):
    if declarations.is_pointer( type_ ):
        return declarations.remove_pointer( type_ )
    elif declarations.is_reference( type_ ):
        return declarations.remove_reference( type_ )
    else:
        raise TypeError( 'Type should be reference or pointer, got %s.' % type_ )


# output_t
class output_t( transformer.transformer_t ):
    """Handles a single output variable.

    The specified variable is removed from the argument list and is turned
    into a return value.

    void get_value(int& v) -> v = get_value()
    """

    def __init__(self, function, arg_ref):
        transformer.transformer_t.__init__( self, function )
        """Constructor.

        The specified argument must be a reference or a pointer.

        :param arg_ref: Index of the argument that is an output value
        :type arg_ref: int
        """
        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ref_or_ptr( self.arg.type ):
            raise ValueError( '%s\nin order to use "output" transformation, argument %s type must be a reference or a pointer (got %s).' ) \
                  % ( function, self.arg_ref.name, arg.type)

    def __str__(self):
        return "output(%d)"%(self.arg.name)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed( self, controller ):
        #removing arg from the function wrapper definition
        controller.remove_wrapper_arg( self.arg.name )
        #declaring new variable, which will keep result
        var_name = controller.declare_variable( remove_ref_or_ptr( self.arg.type ), self.arg.name )
        #adding just declared variable to the original function call expression
        controller.modify_arg_expression( self.arg_index, var_name )
        #adding the variable to return variables list
        controller.return_variable( var_name )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        controller.remove_py_arg( self.arg_index )
        tmpl = string.Template(
            '$name = boost::python::extract< $type >( pyplus_conv::get_out_argument( $py_result, "$name" ) );' )
        store_py_result_in_arg = tmpl.substitute( name=self.arg.name
                                                  , type=remove_ref_or_ptr( self.arg.type ).decl_string
                                                  , py_result=controller.py_result_variable.name )
        controller.add_py_post_call_code( store_py_result_in_arg )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_default( controller.default_controller )
        self.__configure_v_mem_fun_override( controller.override_controller )

# input_t
class type_modifier_t(transformer.transformer_t):
    """Change/modify type of the argument.

    Right now compiler should be able to use implicit conversion
    """

    def __init__(self, function, arg_ref, modifier):
        """Constructor.

        modifier is callable, which take the type of the argument and should return new type
        
        :param arg_ref: Index of the argument which will be transformed
        :type arg_ref: int
        """
        transformer.transformer_t.__init__( self, function )
        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )
        self.modifier = modifier

    def __str__(self):
        return "type_modifier(%s)" % self.arg.name

    def __configure_sealed( self, controller ):
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = self.modifier( self.arg.type )
        if not declarations.is_convertible( w_arg.type, self.arg.type ):
            casting_code = 'reinterpret_cast< %s >( %s )' % ( self.arg.type, w_arg.name )
            controller.modify_arg_expression(self.arg_index, casting_code)

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_default( controller.default_controller )

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return []

# input_t
class input_t(type_modifier_t):
    """Handles a single input variable.

    The reference on the specified variable is removed.

    void set_value(int& v) -> void set_value(int v)
    """

    def __init__(self, function, arg_ref):
        """
        :param arg_ref: Index of the argument that is an input value
        :type arg_ref: int
        """
        type_modifier_t.__init__( self, function, arg_ref, remove_ref_or_ptr )

        if not is_ref_or_ptr( self.arg.type ):
            raise ValueError( '%s\nin order to use "input" transformation, argument %s type must be a reference or a pointer (got %s).' ) \
                  % ( function, self.arg_ref.name, arg.type)

    def __str__(self):
        return "input(%s)"%(self.arg.name)

# from_address_t
class from_address_t(type_modifier_t):
    """Handles a single input variable.

    Replaces the actual argument type with some integral type, so you
    could use :mod:`ctypes` package.

    void do_something(int** image) -> do_something(unsigned int image_address)
    """

    def __init__(self, function, arg_ref):
        """Constructor.

        The specified argument must be a reference or a pointer.

        :param arg_ref: Index of the argument that is an output value
        :type arg_ref: int
        """
        modifier = lambda type_: declarations.FUNDAMENTAL_TYPES[ 'unsigned int' ]
        type_modifier_t.__init__( self, function, arg_ref, modifier )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "from_address_t" transformation, argument %s type must be a pointer or a array (got %s).' ) \
                  % ( function, self.arg_ref.name, arg.type)

    def __str__(self):
        return "from_address(%s)"%(self.arg.name)

# inout_t
class inout_t(transformer.transformer_t):
    """Handles a single input/output variable.

    void do_something(int& v) -> v = do_something(v)
    """

    def __init__(self, function, arg_ref):
        """Constructor.

        The specified argument must be a reference or a pointer.

        :param arg_ref: Index of the argument that is an in/out value
        :type arg_ref: int
        """
        transformer.transformer_t.__init__( self, function )
        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ref_or_ptr( self.arg.type ):
            raise ValueError( '%s\nin order to use "inout" transformation, argument %s type must be a reference or a pointer (got %s).' ) \
                  % ( function, self.arg_ref.name, arg.type)

    def __str__(self):
        return "inout(%s)"%(self.arg.name)

    def __configure_sealed(self, controller):
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = remove_ref_or_ptr( self.arg.type )
        #adding the variable to return variables list
        controller.return_variable( w_arg.name )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        tmpl = string.Template(
            '$name = boost::python::extract< $type >( pyplus_conv::get_out_argument( $py_result, "$name" ) );' )
        store_py_result_in_arg = tmpl.substitute( name=self.arg.name
                                                  , type=remove_ref_or_ptr( self.arg.type ).decl_string
                                                  , py_result=controller.py_result_variable.name )
        controller.add_py_post_call_code( store_py_result_in_arg )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_override( controller.override_controller )
        self.__configure_v_mem_fun_default( controller.default_controller )

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]


_seq2arr = string.Template( os.linesep.join([
              'pyplus_conv::ensure_uniform_sequence< $type >( $pylist, $array_size );'
            , 'pyplus_conv::copy_sequence( $pylist, pyplus_conv::array_inserter( $native_array, $array_size ) );']))

_seq2vector = string.Template( os.linesep.join([
                 'pyplus_conv::ensure_uniform_sequence< $type >( $pylist );'
               , 'pyplus_conv::copy_sequence( $pylist, std::back_inserter( $native_array), boost::type< $type >() );']))

_arr2seq = string.Template(
            'pyplus_conv::copy_container( $native_array, $native_array + $array_size, pyplus_conv::list_inserter( $pylist ) );' )

class input_static_array_t(transformer.transformer_t):
    """Handles an input array with fixed size.

    void do_something(double* v) ->  do_something(object v2)

    where v2 is a Python sequence
    """

    def __init__(self, function, arg_ref, size):
        """Constructor.

        :param arg_ref: Index of the argument that is an input array
        :type arg_ref: int
        :param size: The fixed size of the input array        
        :type size: int
        """
        transformer.transformer_t.__init__( self, function )

        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "input_array" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.array_size = size
        self.array_item_type = declarations.remove_const( declarations.array_item_type( self.arg.type ) )

    def __str__(self):
        return "input_array(%s,%d)"%( self.arg.name, self.array_size)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _seq2arr
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = declarations.dummy_type_t( "boost::python::object" )

        # Declare a variable that will hold the C array...
        native_array = controller.declare_variable( self.array_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d]' % self.array_size )

        copy_pylist2arr = _seq2arr.substitute( type=self.array_item_type
                                                , pylist=w_arg.name
                                                , array_size=self.array_size
                                                , native_array=native_array )

        controller.add_pre_call_code( copy_pylist2arr )

        controller.modify_arg_expression( self.arg_index, native_array )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        global _arr2seq
        pylist = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::list' )
                                                 , 'py_' + self.arg.name )

        copy_arr2pylist = _arr2seq.substitute( native_array=self.arg.name
                                                , array_size=self.array_size
                                                , pylist=pylist )

        controller.add_py_pre_call_code( copy_arr2pylist )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_override( controller.override_controller )
        self.__configure_v_mem_fun_default( controller.default_controller )


# s - static
class output_static_array_t(transformer.transformer_t):
    """Handles an output array of a fixed size.

    void get_vec3(double* v) -> v = get_vec3()
    # v will be a list with 3 floats
    """

    def __init__(self, function, arg_ref, size):
        """Constructor.

        :param arg_ref: Index of the argument that is an output array
        :type arg_ref: int        
        :param size: The fixed size of the output array
        :type size: int
        """
        transformer.transformer_t.__init__( self, function )
        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "output_array" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.array_size = size
        self.array_item_type = declarations.array_item_type( self.arg.type )

    def __str__(self):
        return "output_array(%s,%d)"%( self.arg.name, self.array_size)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _arr2seq
        #removing arg from the function wrapper definition
        controller.remove_wrapper_arg( self.arg.name )

        # Declare a variable that will hold the C array...
        native_array = controller.declare_variable( self.array_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d]' % self.array_size )

        #adding just declared variable to the original function call expression
        controller.modify_arg_expression( self.arg_index, native_array )

        # Declare a Python list which will receive the output...
        pylist = controller.declare_variable( declarations.dummy_type_t( "boost::python::list" )
                                              , 'py_' + self.arg.name )

        copy_arr2pylist = _arr2seq.substitute( native_array=native_array
                                               , array_size=self.array_size
                                               , pylist=pylist )

        controller.add_post_call_code( copy_arr2pylist )

        #adding the variable to return variables list
        controller.return_variable( pylist )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        global _seq2arr
        seq = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::object' )
                                              , 'py_' + self.arg.name )
        controller.remove_py_arg( self.arg_index )
        tmpl = string.Template( '$seq = pyplus_conv::get_out_argument( $py_result, "$name" );' )
        get_ref_to_seq = tmpl.substitute( seq=seq
                                          , py_result=controller.py_result_variable.name
                                          , name=self.arg.name )
        controller.add_py_post_call_code( get_ref_to_seq )

        copy_pylist2arr = _seq2arr.substitute( type=self.array_item_type
                                               , pylist=seq
                                               , array_size=self.array_size
                                               , native_array=self.arg.name )
        controller.add_py_post_call_code( copy_pylist2arr )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_override( controller.override_controller )
        self.__configure_v_mem_fun_default( controller.default_controller )

# inout_static_array_t
class inout_static_array_t(transformer.transformer_t):
    """Handles an input/output array with fixed size.

    void do_something(double* v) ->  v2 = do_something(object v2)

    where v2 is a Python sequence
    """

    def __init__(self, function, arg_ref, size):
        """Constructor.

        :param arg_ref: Index of the argument that is an input/output array
        :type arg_ref: int
        :param size: The fixed size of the input/output array
        :type size: int
        """
        transformer.transformer_t.__init__( self, function )

        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "inout_array" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.array_size = size
        self.array_item_type = declarations.remove_const( declarations.array_item_type( self.arg.type ) )

    def __str__(self):
        return "inout_array(%s,%d)"%( self.arg.name, self.array_size)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _seq2arr
        global _arr2seq
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = declarations.dummy_type_t( "boost::python::object" )

        # Declare a variable that will hold the C array...
        native_array = controller.declare_variable( self.array_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d]' % self.array_size )

        copy_pylist2arr = _seq2arr.substitute( type=self.array_item_type
                                                , pylist=w_arg.name
                                                , array_size=self.array_size
                                                , native_array=native_array )

        controller.add_pre_call_code( copy_pylist2arr )
        controller.modify_arg_expression( self.arg_index, native_array )
        
        # Declare a Python list which will receive the output...
        pylist = controller.declare_variable( declarations.dummy_type_t( "boost::python::list" )
                                              , 'py_' + self.arg.name )

        copy_arr2pylist = _arr2seq.substitute( native_array=native_array
                                              , array_size=self.array_size
                                              , pylist=pylist )
        controller.add_post_call_code( copy_arr2pylist )

        #adding the variable to return variables list
        controller.return_variable( pylist )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        global _arr2seq
        pylist = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::list' )
                                                 , 'py_' + self.arg.name )

        copy_arr2pylist = _arr2seq.substitute( native_array=self.arg.name
                                                , array_size=self.array_size
                                                , pylist=pylist )

        controller.add_py_pre_call_code( copy_arr2pylist )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_override( controller.override_controller )
        self.__configure_v_mem_fun_default( controller.default_controller )

_pymatrix2cmatrix = string.Template( os.linesep.join([
   'pyplus_conv::ensure_uniform_sequence< boost::python::list >( $pymatrix, $rows );'
 , 'for( size_t $row = 0; $row < $rows; ++$row ){'
 , '    pyplus_conv::ensure_uniform_sequence< $type >( $pymatrix[$row], $columns );'
 , '    pyplus_conv::copy_sequence( $pymatrix[$row], pyplus_conv::array_inserter( $native_matrix[$row], $columns ) );'
 , '}']))


_cmatrix2pymatrix = string.Template( os.linesep.join([
    'for (int $row = 0; $row < $rows; ++$row ){'
  , '    boost::python::list $pyrow;'
  , '    pyplus_conv::copy_container( $native_matrix[$row]'
  , '                                 , $native_matrix[$row] + $columns'
  , '                                 , pyplus_conv::list_inserter( $pyrow ) );'
  , '    $pymatrix.append( $pyrow ); '
  , '}' ]))


# input_static_matrix_t
class input_static_matrix_t(transformer.transformer_t):
    """Handles an input matrix with fixed size.

    is_identity(double m[3][3]) ->  is_identity(object m)
    # m must be a sequence of 3 sequences of 3 floats
    """

    def __init__(self, function, arg_ref, rows, columns):
        """Constructor.

        :param rows, columns: The fixed size of the input matrix
        :type rows, columns: int
        """
        transformer.transformer_t.__init__( self, function )

        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "input_matrix" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.rows = rows
        self.columns = columns
        self.matrix_item_type = declarations.remove_const( declarations.array_item_type( declarations.array_item_type( self.arg.type ) ) )

    def __str__(self):
        return "input_matrix(%s,%d,%d)"%( self.arg.name, self.rows, self.columns)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _pymatrix2cmatrix
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = declarations.dummy_type_t( "boost::python::object" )

        # Declare a variable that will hold the C matrix...
        native_matrix = controller.declare_variable( self.matrix_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d][%d]' % (self.rows, self.columns) )

        conversion_code = _pymatrix2cmatrix.substitute( type=self.matrix_item_type
                                                        , pymatrix=w_arg.name
                                                        , columns='%d' % self.columns
                                                        , row=controller.register_variable_name( "row" )
                                                        , rows='%d' % self.rows
                                                        , native_matrix=native_matrix )

        controller.add_pre_call_code( conversion_code )

        controller.modify_arg_expression( self.arg_index, native_matrix )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):       
        global _arr2seq
        pylist = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::list' )
                                                 , 'py_' + self.arg.name )

        #TODO: may be a better idea is move this loop to the generated code.
        for i in range(0, self.rows):
            copy_arr2pylist = _arr2seq.substitute( native_array=self.arg.name+'[%d]'%i
                                                   , array_size=self.columns
                                                   , pylist=pylist )

            controller.add_py_pre_call_code( copy_arr2pylist )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        raise RuntimeError( '"input_static_matrix" transformation does not support virtual functions yet.' )
        #self.__configure_v_mem_fun_override( controller.override_controller )
        #self.__configure_v_mem_fun_default( controller.default_controller )

# output_static_matrix_t
class output_static_matrix_t(transformer.transformer_t):
    """Handles an output matrix with fixed size.

    get_matrix(double m[3][3]) ->  m = get_matrix()
    # m will be a sequence of 3 sequences of 3 floats
    """

    def __init__(self, function, arg_ref, rows, columns):
        """Constructor.

        :param arg_ref: Index of the argument that is an output matrix
        :type arg_ref: int
        :param rows, columns: The fixed size of the output matrix
        :type rows, columns: int
        
        """
        transformer.transformer_t.__init__( self, function )

        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "output_matrix" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.rows = rows
        self.columns = columns
        self.matrix_item_type = declarations.remove_const( declarations.array_item_type( declarations.array_item_type( self.arg.type ) ) )

    def __str__(self):
        return "output_matrix(%s,%d,%d)"%( self.arg.name, self.rows, self.columns)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _cmatrix2pymatrix
        #removing arg from the function wrapper definition
        controller.remove_wrapper_arg( self.arg.name )

        # Declare a variable that will hold the C matrix...
        native_matrix = controller.declare_variable( self.matrix_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d][%d]' % (self.rows, self.columns ) )
        #adding just declared variable to the original function call expression
        controller.modify_arg_expression( self.arg_index, native_matrix )

        # Declare a Python list which will receive the output...
        pymatrix = controller.declare_variable( declarations.dummy_type_t( "boost::python::list" )
                                              , 'py_' + self.arg.name )

        conversion_code = _cmatrix2pymatrix.substitute( pymatrix=pymatrix
                                                        , columns='%d' % self.columns
                                                        , row=controller.register_variable_name( "row" )
                                                        , pyrow=controller.register_variable_name( "pyrow" )
                                                        , rows='%d' % self.rows
                                                        , native_matrix=native_matrix )

        controller.add_post_call_code( conversion_code )
        
        #adding the variable to return variables list
        controller.return_variable( pymatrix )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        global _seq2arr
        seq = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::object' )
                                              , 'py_' + self.arg.name )
        controller.remove_py_arg( self.arg_index )
        tmpl = string.Template( '$seq = pyplus_conv::get_out_argument( $py_result, "$name" );' )
        get_ref_to_seq = tmpl.substiture( seq=seq
                                          , py_result=controller.py_result_variable_name
                                          , name=self.arg.name )
        controller.add_py_post_call_code( get_ref_to_seq )

        #TODO: may be a better idea is move this loop to the generated code.
        for i in range(0, self.rows):
            copy_pylist2arr = _seq2arr.substitute( type=self.matrix_item_type
                                                   , pylist=seq
                                                   , array_size=self.columns
                                                   , native_array=self.arg.name+'[%d]'%i )
            controller.add_py_post_call_code( copy_pylist2arr )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        raise RuntimeError( '"output_static_matrix" transformation does not support virtual functions yet.' )
        #self.__configure_v_mem_fun_override( controller.override_controller )
        #self.__configure_v_mem_fun_default( controller.default_controller )

# inout_static_matrix_t
class inout_static_matrix_t(transformer.transformer_t):
    """Handles an input/output matrix with fixed size.

    transpose_matrix(double m[2][3]) ->  m = transpose_matrix(object m)
    # m must be a sequence of 2 sequences of 3 floats
    """

    def __init__(self, function, arg_ref, rows, columns):
        """Constructor.

        :param arg_ref: Index of the argument that is an input/output matrix
        :type arg_ref: int
        :param rows,columns: The fixed size of the input/output matrix
        :type rows,columns: int
        """
        transformer.transformer_t.__init__( self, function )

        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )

        if not is_ptr_or_array( self.arg.type ):
            raise ValueError( '%s\nin order to use "inout_matrix" transformation, argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.arg.name, self.arg.type)

        self.rows = rows
        self.columns = columns
        self.matrix_item_type = declarations.remove_const( declarations.array_item_type( declarations.array_item_type( self.arg.type ) ) )

    def __str__(self):
        return "inout_matrix(%s,%d,%d)"%( self.arg.name, self.rows, self.columns)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name ]

    def __configure_sealed(self, controller):
        global _pymatrix2cmatrix
        global _cmatrix2pymatrix
        w_arg = controller.find_wrapper_arg( self.arg.name )
        w_arg.type = declarations.dummy_type_t( "boost::python::object" )

        # Declare a variable that will hold the C matrix...
        native_matrix = controller.declare_variable( self.matrix_item_type
                                                    , "native_" + self.arg.name
                                                    , '[%d][%d]' % (self.rows, self.columns) )

        conversion_code = _pymatrix2cmatrix.substitute( type=self.matrix_item_type
                                                        , pymatrix=w_arg.name
                                                        , columns='%d' % self.columns
                                                        , row=controller.register_variable_name( "row" )
                                                        , rows='%d' % self.rows
                                                        , native_matrix=native_matrix )

        controller.add_pre_call_code( conversion_code )

        controller.modify_arg_expression( self.arg_index, native_matrix )


        #adding just declared variable to the original function call expression
        controller.modify_arg_expression( self.arg_index, native_matrix )

        # Declare a Python list which will receive the output...
        pymatrix = controller.declare_variable( declarations.dummy_type_t( "boost::python::list" )
                                              , 'py_' + self.arg.name )

        conversion_code = _cmatrix2pymatrix.substitute( pymatrix=pymatrix
                                                        , columns='%d' % self.columns
                                                        , row=controller.register_variable_name( "row" )
                                                        , pyrow=controller.register_variable_name( "pyrow" )
                                                        , rows='%d' % self.rows
                                                        , native_matrix=native_matrix )

        controller.add_post_call_code( conversion_code )
        
        #adding the variable to return variables list
        controller.return_variable( pymatrix )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        global _mat2seq
        pylist = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::list' )
                                                 , 'py_' + self.arg.name )

        copy_mat2pylist = _mat2seq.substitute( native_matrix=self.arg.name
                                               , rows=self.rows
                                               , columns=self.columns
                                               , pylist=pylist )

        controller.add_py_pre_call_code( copy_arr2pylist )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        raise RuntimeError( '"output_static_matrix" transformation does not support virtual functions yet.' )
        #self.__configure_v_mem_fun_override( controller.override_controller )
        #self.__configure_v_mem_fun_default( controller.default_controller )

# input_c_buffer_t
class input_c_buffer_t(transformer.transformer_t):
    """
    handles an input of C buffer:

    void write( byte \\*buffer, int size ) -> void write( python sequence )
    """

    def __init__(self, function, buffer_arg_ref, size_arg_ref):
        """Constructor.

        :param buffer_arg_ref: "reference" to the buffer argument
        :param buffer_arg_ref: "reference" to argument, which holds buffer size
        """
        transformer.transformer_t.__init__( self, function )

        self.buffer_arg = self.get_argument( buffer_arg_ref )
        self.buffer_arg_index = self.function.arguments.index( self.buffer_arg )

        self.size_arg = self.get_argument( size_arg_ref )
        self.size_arg_index = self.function.arguments.index( self.size_arg )

        if not is_ptr_or_array( self.buffer_arg.type ):
            raise ValueError( '%s\nin order to use "input_c_buffer" transformation, "buffer" argument %s type must be a array or a pointer (got %s).' ) \
                  % ( function, self.buffer_arg.name, self.buffer_arg.type)

        if not declarations.is_integral( self.size_arg.type ):
            raise ValueError( '%s\nin order to use "input_c_buffer" transformation, "size" argument %s type must be an integral type (got %s).' ) \
                  % ( function, self.size_arg.name, self.size_arg.type)

        self.buffer_item_type = declarations.remove_const( declarations.array_item_type( self.buffer_arg.type ) )

    def __str__(self):
        return "input_c_buffer(buffer arg=%s, size arg=%s)" \
               % ( self.buffer_arg.name, self.size_arg.name)

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return [ code_repository.convenience.file_name, '<vector>', '<iterator>' ]

    def __configure_sealed(self, controller):
        global _seq2arr
        w_buffer_arg = controller.find_wrapper_arg( self.buffer_arg.name )
        w_buffer_arg.type = declarations.dummy_type_t( "boost::python::object" )

        controller.remove_wrapper_arg( self.size_arg.name )

        size_var = controller.declare_variable(
                          declarations.remove_const( self.size_arg.type )
                        , self.size_arg.name
                        , ' = boost::python::len(%s)' % w_buffer_arg.name )

        # Declare a variable that will hold the C array...
        buffer_var = controller.declare_variable(
                          declarations.dummy_type_t( "std::vector< %s >" % self.buffer_item_type.decl_string )
                        , "native_" + self.buffer_arg.name )

        controller.add_pre_call_code( '%s.reserve( %s );' % ( buffer_var, size_var ) )

        copy_pylist2arr = _seq2vector.substitute( type=self.buffer_item_type
                                                  , pylist=w_buffer_arg.name
                                                  , native_array=buffer_var )

        controller.add_pre_call_code( copy_pylist2arr )

        controller.modify_arg_expression( self.buffer_arg_index, '&%s[0]' % buffer_var )
        controller.modify_arg_expression( self.size_arg_index, '%s' % size_var )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def __configure_v_mem_fun_override( self, controller ):
        raise NotImplementedError()
        #global _arr2seq
        #pylist = controller.declare_py_variable( declarations.dummy_type_t( 'boost::python::list' )
                                                 #, 'py_' + self.arg.name )

        #copy_arr2pylist = _arr2seq.substitute( native_array=self.arg.name
                                                #, array_size=self.array_size
                                                #, pylist=pylist )

        #controller.add_py_pre_call_code( copy_arr2pylist )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        self.__configure_v_mem_fun_override( controller.override_controller )
        self.__configure_v_mem_fun_default( controller.default_controller )

class transfer_ownership_t(type_modifier_t):
    """see http://boost.org/libs/python/doc/v2/faq.html#ownership
    """
    def __init__(self, function, arg_ref):
        """Constructor.
        
        :param arg_ref: Index of the argument on which to transfer ownership
        :type arg_ref: int
        """
        transformer.transformer_t.__init__( self, function )
        self.arg = self.get_argument( arg_ref )
        self.arg_index = self.function.arguments.index( self.arg )
        if not declarations.is_pointer( self.arg.type ):
            raise ValueError( '%s\nin order to use "transfer_ownership" transformation, argument %s type must be a pointer (got %s).' ) \
                  % ( function, self.arg_ref.name, arg.type)

    def __str__(self):
        return "transfer_ownership(%s)" % self.arg.name

    def __configure_sealed( self, controller ):
        w_arg = controller.find_wrapper_arg( self.arg.name )
        naked_type = declarations.remove_pointer( self.arg.type )
        naked_type = declarations.remove_declarated( naked_type )
        w_arg.type = declarations.dummy_type_t( 'std::auto_ptr< %s >' % naked_type.decl_string )
        controller.modify_arg_expression(self.arg_index, w_arg.name + '.release()' )

    def __configure_v_mem_fun_default( self, controller ):
        self.__configure_sealed( controller )

    def configure_mem_fun( self, controller ):
        self.__configure_sealed( controller )

    def configure_free_fun(self, controller ):
        self.__configure_sealed( controller )

    def configure_virtual_mem_fun( self, controller ):
        raise NotImplementedError(self.__class__.__name__)

#TODO: FT for constructor
    #~ def configure_constructor( self, controller ):
        #~ self.__configure_sealed( controller )

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        return []

