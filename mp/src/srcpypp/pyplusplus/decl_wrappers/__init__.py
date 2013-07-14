# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""Code generator configuration classes

:mod:`pygccxml.declarations` package contains classes, which describe C++ declarations.
This package contains classes that derive from the :mod:`pygccxml.declarations` classes.
The classes in this package allow you to configure the code generator.
"""

from . import algorithm

from .decl_wrapper import CODE_GENERATOR_TYPES
from .decl_wrapper import decl_wrapper_t

from .calldef_wrapper import calldef_t
from .calldef_wrapper import member_function_t
from .calldef_wrapper import constructor_t
from .calldef_wrapper import destructor_t
from .calldef_wrapper import member_operator_t
from .calldef_wrapper import casting_operator_t
from .calldef_wrapper import free_function_t
from .calldef_wrapper import free_operator_t

from .class_wrapper import class_declaration_t
from .class_wrapper import class_t

from .enumeration_wrapper import enumeration_t

from .namespace_wrapper import namespace_t

from .typedef_wrapper import typedef_t

from .variable_wrapper import variable_t

from .scopedef_wrapper import scopedef_t

from pygccxml import declarations

from .call_policies import call_policy_t
from .call_policies import default_call_policies_t
from .call_policies import default_call_policies
from .call_policies import compound_policy_t
from .call_policies import return_argument_t
from .call_policies import return_arg
from .call_policies import return_self
from .call_policies import return_internal_reference_t
from .call_policies import return_internal_reference
from .call_policies import with_custodian_and_ward_t
from .call_policies import with_custodian_and_ward
from .call_policies import with_custodian_and_ward_postcall_t
from .call_policies import with_custodian_and_ward_postcall
from .call_policies import return_value_policy_t
from .call_policies import copy_const_reference
from .call_policies import copy_non_const_reference
from .call_policies import manage_new_object
from .call_policies import reference_existing_object
from .call_policies import return_by_value
from .call_policies import return_opaque_pointer
from .call_policies import return_value_policy
from .call_policies import return_pointee_value
from .call_policies import return_addressof
from .call_policies import is_return_opaque_pointer_policy
from .call_policies import custom_call_policies_t
from .call_policies import custom_call_policies
from .call_policies import convert_array_to_tuple_t
from .call_policies import convert_array_to_tuple
from .call_policies import memory_managers
from .call_policies import return_range
from .call_policies import return_range_t

from .decl_wrapper_printer import decl_wrapper_printer_t
from .decl_wrapper_printer import print_declarations

from .user_text import user_text_t
from .user_text import class_user_text_t

from .indexing_suite1 import indexing_suite1_t
from .indexing_suite2 import indexing_suite2_t

from .doc_extractor import doc_extractor_i

from .properties import property_t
from .properties import property_recognizer_i
from .properties import name_based_recognizer_t

from . import python_traits

class dwfactory_t( declarations.decl_factory_t ):
    """declarations factory class"""
    def __init__(self):
        declarations.decl_factory_t.__init__(self)

    def create_member_function( self, *arguments, **keywords ):
        return member_function_t(*arguments, **keywords)

    def create_constructor( self, *arguments, **keywords ):
        return constructor_t(*arguments, **keywords)

    def create_destructor( self, *arguments, **keywords ):
        return destructor_t(*arguments, **keywords)

    def create_member_operator( self, *arguments, **keywords ):
        return member_operator_t(*arguments, **keywords)

    def create_casting_operator( self, *arguments, **keywords ):
        return casting_operator_t(*arguments, **keywords)

    def create_free_function( self, *arguments, **keywords ):
        return free_function_t(*arguments, **keywords)

    def create_free_operator( self, *arguments, **keywords ):
        return free_operator_t(*arguments, **keywords)

    def create_class_declaration(self, *arguments, **keywords ):
        return class_declaration_t(*arguments, **keywords)

    def create_class( self, *arguments, **keywords ):
        return class_t(*arguments, **keywords)

    def create_enumeration( self, *arguments, **keywords ):
        return enumeration_t(*arguments, **keywords)

    def create_namespace( self, *arguments, **keywords ):
        return namespace_t(*arguments, **keywords)

    def create_typedef( self, *arguments, **keywords ):
        return typedef_t(*arguments, **keywords)

    def create_variable( self, *arguments, **keywords ):
        return variable_t(*arguments, **keywords)
