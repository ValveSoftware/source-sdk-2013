# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""Contains definition of call policies classes"""

from . import algorithm
from . import python_traits
from pygccxml import declarations

#keeps file name, where `Py++` defined call policies will be defined
PYPP_CALL_POLICIES_HEADER_FILE = "__call_policies.pypp.hpp"

class CREATION_POLICY:
    """Implementation details"""
    AS_INSTANCE = 'as instance'
    AS_TEMPLATE_ARGUMENT = 'as template argument'

class call_policy_t(object):
    """base class for all classes, which generate "call policies" code"""
    def __init__(self):
        object.__init__(self)

    def create(self, function_creator, creation_policy=CREATION_POLICY.AS_INSTANCE):
        """Creates code from the call policies class instance.
        :param function_creator: parent code creator
        :type function_creator: :class:`code_creators.function_t` or :class:`code_creators.constructor_t`

        :param creation_policy: indicates whether we this call policy used as template
                                argument or as an instance
        :type creation_policy: :class:`decl_wrappers.CREATION_POLICY`
        """
        code = self._create_impl( function_creator )
        if code and creation_policy == CREATION_POLICY.AS_INSTANCE:
            code = code + '()'
        return code

    def create_type(self):
        """return call policies class declaration as string"""
        return self.create( None, CREATION_POLICY.AS_TEMPLATE_ARGUMENT )

    def create_template_arg( self, function_creator ):
        """return call policies class declaration as string"""
        return self.create( function_creator, CREATION_POLICY.AS_TEMPLATE_ARGUMENT )

    def is_default( self ):
        """return True is self is instance of :class:`decl_wrappers.default_call_policies_t` class"""
        return False

    def is_predefined( self ):
        """return True if call policy is defined in Boost.Python library, False otherwise"""
        return True

    def _create_impl( self, function_creator ):
        raise NotImplementedError()

    @property
    def header_file(self):
        """return a name of the header file the call policy is defined in"""
        return "boost/python.hpp"

class default_call_policies_t(call_policy_t):
    """implements code generation for boost::python::default_call_policies"""
    def __init__( self ):
        call_policy_t.__init__( self )

    def _create_impl(self, function_creator ):
        return algorithm.create_identifier( function_creator, '::boost::python::default_call_policies' )

    def is_default( self ):
        return True

    def __str__(self):
        return 'default_call_policies'

def default_call_policies():
    """create ::boost::python::default_call_policies call policies code generator"""
    return default_call_policies_t()

class compound_policy_t( call_policy_t ):
    """base class for all call policies, except the default one"""
    def __init__( self, base=None ):
        call_policy_t.__init__( self )
        self._base = base
        if not base:
            self._base = default_call_policies_t()

    def _get_base_policy( self ):
        return self._base
    def _set_base_policy( self, new_policy ):
        self._base = new_policy
    base_policy = property( _get_base_policy, _set_base_policy
                            , doc="base call policy, by default is reference to :class:`decl_wrappers.default_call_policies_t` call policy")

    def _get_args(self, function_creator):
        return []

    def _get_name(self, function_creator):
        raise NotImplementedError()

    def _create_impl( self, function_creator ):
        args = self._get_args(function_creator)
        if not self._base.is_default():
            args.append( self._base.create( function_creator, CREATION_POLICY.AS_TEMPLATE_ARGUMENT ) )
        name = algorithm.create_identifier( function_creator, self._get_name(function_creator) )
        return declarations.templates.join( name, args )

    def __str__(self):
        name = self._get_name(None).replace('::boost::python::', '' )
        args = [text.replace( '::boost::python::', '' ) for text in self._get_args( None )]
        return declarations.templates.join( name, args )

class return_argument_t( compound_policy_t ):
    """implements code generation for boost::python::return_argument call policies"""
    def __init__( self, position=1, base=None):
        compound_policy_t.__init__( self, base )
        self._position = position

    def _get_position( self ):
        return self._position
    def _set_position( self, new_position):
        self._position = new_position
    position = property( _get_position, _set_position )

    def _get_name(self, function_creator):
        if self.position == 1:
            return '::boost::python::return_self'
        else:
            return '::boost::python::return_arg'

    def _get_args(self, function_creator):
        if self.position == 1:
            return []
        else:
            return [ str( self.position ) ]

def return_arg( arg_pos, base=None ):
    """create boost::python::return_arg call policies code generator"""
    return return_argument_t( arg_pos, base )

def return_self(base=None):
    """create boost::python::return_self call policies code generator"""
    return return_argument_t( 1, base )

class return_internal_reference_t( compound_policy_t ):
    """implements code generation for boost::python::return_internal_reference call policies"""
    def __init__( self, position=1, base=None):
        compound_policy_t.__init__( self, base )
        self._position = position

    def _get_position( self ):
        return self._position
    def _set_position( self, new_position):
        self._position = new_position
    position = property( _get_position, _set_position )

    def _get_name(self, function_creator):
        return '::boost::python::return_internal_reference'

    def _get_args(self, function_creator):
        if self.position == 1:
            return [] #don't generate default template arguments
        else:
            return [ str( self.position ) ]

def return_internal_reference( arg_pos=1, base=None):
    """create boost::python::return_internal_reference call policies code generator"""
    return return_internal_reference_t( arg_pos, base )

class with_custodian_and_ward_t( compound_policy_t ):
    """implements code generation for boost::python::with_custodian_and_ward call policies"""
    def __init__( self, custodian, ward, base=None):
        compound_policy_t.__init__( self, base )
        self._custodian = custodian
        self._ward = ward

    def _get_custodian( self ):
        return self._custodian
    def _set_custodian( self, new_custodian):
        self._custodian = new_custodian
    custodian = property( _get_custodian, _set_custodian )

    def _get_ward( self ):
        return self._ward
    def _set_ward( self, new_ward):
        self._ward = new_ward
    ward = property( _get_ward, _set_ward )

    def _get_name(self, function_creator):
        return '::boost::python::with_custodian_and_ward'

    def _get_args(self, function_creator):
        return [ str( self.custodian ), str( self.ward ) ]

def with_custodian_and_ward( custodian, ward, base=None):
    """create boost::python::with_custodian_and_ward call policies code generator"""
    return with_custodian_and_ward_t( custodian, ward, base )

class with_custodian_and_ward_postcall_t( with_custodian_and_ward_t ):
    """implements code generation for boost::python::with_custodian_and_ward_postcall call policies"""
    def __init__( self, custodian, ward, base=None):
        with_custodian_and_ward_t.__init__( self, custodian, ward, base )

    def _get_name(self, function_creator):
        return '::boost::python::with_custodian_and_ward_postcall'

def with_custodian_and_ward_postcall( custodian, ward, base=None):
    """create boost::python::with_custodian_and_ward_postcall call policies code generator"""
    return with_custodian_and_ward_postcall_t( custodian, ward, base )

class return_value_policy_t( compound_policy_t ):
    """implements code generation for boost::python::return_value_policy call policies"""
    def __init__( self, result_converter_generator, base=None):
        compound_policy_t.__init__( self, base )
        self._result_converter_generator = result_converter_generator

    def _get_result_converter_generator( self ):
        return self._result_converter_generator
    def _set_result_converter_generator( self, new_result_converter_generator):
        self._result_converter_generator = new_result_converter_generator
    result_converter_generator = property( _get_result_converter_generator
                                           , _set_result_converter_generator )

    def _get_name(self, function_creator):
        return '::boost::python::return_value_policy'

    def _get_args(self, function_creator):
        if function_creator:
            rcg = algorithm.create_identifier( function_creator, self.result_converter_generator )
            return [ rcg ]
        else:
            return [self.result_converter_generator]

    def is_predefined( self ):
        """Returns True if call policy is defined in Boost.Python library, False otherwise"""
        global return_addressof
        global return_pointee_value
        if self.result_converter_generator in (return_pointee_value, return_addressof ):
            return False
        else:
            return True

    @property
    def header_file(self):
        """Return name of the header file to be included"""
        if self.is_predefined():
            return super( return_value_policy_t, self ).header_file
        else:
            return PYPP_CALL_POLICIES_HEADER_FILE


copy_const_reference = '::boost::python::copy_const_reference'
copy_non_const_reference = '::boost::python::copy_non_const_reference'
manage_new_object = '::boost::python::manage_new_object'
reference_existing_object = '::boost::python::reference_existing_object'
return_by_value = '::boost::python::return_by_value'
return_opaque_pointer = '::boost::python::return_opaque_pointer'
return_pointee_value = '::pyplusplus::call_policies::return_pointee_value'
return_addressof = '::pyplusplus::call_policies::return_addressof'

def return_value_policy( result_converter_generator, base=None):
    """create boost::python::return_value_policy call policies code generator"""
    return return_value_policy_t( result_converter_generator, base )

def is_return_opaque_pointer_policy( policy ):
    """returns True is policy represents return_value_policy<return_opaque_pointer>, False otherwise"""
    return isinstance( policy, return_value_policy_t ) \
            and policy.result_converter_generator == return_opaque_pointer

class custom_call_policies_t(call_policy_t):
    """implements code generation for user defined call policies"""
    def __init__( self, call_policies, header_file=None ):
        call_policy_t.__init__( self )
        self.__call_policies = call_policies
        self.__header_file = header_file

    def _create_impl(self, function_creator ):
        return str( self.__call_policies )

    def __str__(self):
        return 'custom call policies'

    def get_header_file( self ):
        return self.__header_file
    def set_header_file( self, header_file_name ):
        self.__header_file = header_file_name
    header_file = property( get_header_file, set_header_file
                            , doc="""Return name of the header file to be included""" )

def custom_call_policies(call_policies, header_file=None):
    """create custom\\user defined call policies code generator"""
    return custom_call_policies_t(call_policies, header_file)

class memory_managers:
    """implements code generation for `Py++` defined memory managers

    For complete documentation and usage example see "Call policies" document.
    """
    none = 'none'
    delete_ = 'delete_'
    all = [ none, delete_ ]

    @staticmethod
    def create( manager, function_creator=None):
        mem_manager = 'pyplusplus::call_policies::memory_managers::' + manager
        if function_creator:
            mem_manager = algorithm.create_identifier( function_creator, mem_manager )
        return mem_manager

class convert_array_to_tuple_t( compound_policy_t ):
    """implements code generation for `Py++` defined "as_tuple" value policy

    For complete documentation and usage example see "Call policies" document.
    """
    def __init__( self, array_size, memory_manager, make_object_call_policies=None, base=None):
        compound_policy_t.__init__( self, base )
        self._array_size = array_size
        self._memory_manager = memory_manager
        self._make_objec_call_policies = make_object_call_policies

    def is_predefined( self ):
        """Returns True if call policy is defined in Boost.Python library, False otherwise"""
        return False

    @property
    def header_file(self):
        """Return name of the header file to be included"""
        return PYPP_CALL_POLICIES_HEADER_FILE

    def _get_array_size( self ):
        return self._array_size
    def _set_array_size( self, new_array_size):
        self._array_size = new_array_size
    array_size = property( _get_array_size, _set_array_size )

    def _get_memory_manager( self ):
        return self._memory_manager
    def _set_memory_manager( self, new_memory_manager):
        self._memory_manager = new_memory_manager
    memory_manager = property( _get_memory_manager, _set_memory_manager )

    def _get_make_objec_call_policies( self ):
        if None is self._make_objec_call_policies:
            self._make_objec_call_policies = default_call_policies()
        return self._make_objec_call_policies
    def _set_make_objec_call_policies( self, new_make_objec_call_policies):
        self._make_objec_call_policies = new_make_objec_call_policies
    make_objec_call_policies = property( _get_make_objec_call_policies, _set_make_objec_call_policies )

    def _get_name(self, function_creator):
        return '::boost::python::return_value_policy'

    def _get_args(self, function_creator):
        as_tuple_args = [ str( self.array_size ) ]
        as_tuple_args.append( memory_managers.create( self.memory_manager, function_creator ) )
        if not self.make_objec_call_policies.is_default():
            as_tuple_args.append( self.make_objec_call_policies.create_template_arg( function_creator ) )
        as_tuple = '::pyplusplus::call_policies::arrays::as_tuple'
        if function_creator:
            as_tuple = algorithm.create_identifier( function_creator, as_tuple )
        return [ declarations.templates.join( as_tuple, as_tuple_args ) ]

def convert_array_to_tuple( array_size, memory_manager, make_object_call_policies=None, base=None ):
    """create boost::python::return_value_policy< py++::as_tuple > call policies code generator"""
    return convert_array_to_tuple_t( array_size, memory_manager, make_object_call_policies, base )

class return_range_t( call_policy_t ):
    """implements code generation for `Py++` defined "return_range" call policies

    For complete documentation and usage example see "Call policies" document.
    """
    HEADER_FILE = "__return_range.pypp.hpp"
    def __init__( self, get_size_class, value_type, value_policies):
        call_policy_t.__init__( self )
        self._value_type = value_type
        self._get_size_class = get_size_class
        self._value_policies = value_policies

    def is_predefined( self ):
        """Returns True if call policy is defined in Boost.Python library, False otherwise"""
        return False

    @property
    def header_file(self):
        """Return name of the header file to be included"""
        return self.HEADER_FILE

    def _get_get_size_class( self ):
        return self._get_size_class
    def _set_get_size_class( self, new_get_size_class):
        self._get_size_class = new_get_size_class
    get_size_class = property( _get_get_size_class, _set_get_size_class )

    def _get_value_type( self ):
        return self._value_type
    def _set_value_type( self, new_value_type):
        self._value_type = new_value_type
    value_type = property( _get_value_type, _set_value_type )

    def _get_value_policies( self ):
        return self._value_policies
    def _set_value_policies( self, new_value_policies):
        self._value_policies = new_value_policies
    value_policies = property( _get_value_policies, _set_value_policies )

    def _create_impl(self, function_creator ):
        name = algorithm.create_identifier( function_creator, '::pyplusplus::call_policies::return_range' )
        args = [ self.get_size_class, self.value_type.decl_string ]
        if not self.value_policies.is_default():
            args.append( self.value_policies.create_type() )
        return declarations.templates.join( name, args )

def return_range( function, get_size_class, value_policies=None ):
    """create `Py++` defined return_range call policies code generator"""
    r_type = function.return_type
    if not declarations.is_pointer( r_type ):
        raise TypeError( 'Function "%s" return type should be pointer, got "%s"'
                         % r_type.decl_string )

    value_type = declarations.remove_pointer( r_type )
    if None is value_policies:
        if python_traits.is_immutable( value_type ):
            value_policies = default_call_policies()
        else:
            raise RuntimeError( "return_range call policies requieres specification of value_policies" )
    return return_range_t( get_size_class, value_type, value_policies )

