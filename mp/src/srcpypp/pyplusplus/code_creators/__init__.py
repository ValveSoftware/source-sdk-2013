# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""Code creators.

This sub-package contains the code creator classes which are nodes in
the code creator tree. This tree represents the entire source code of
the final extension module (even when the source code will later be
distributed among several source files) and each individual code
creator represents a single block of source code.

The base class for all code creators is :class:`code_creators.code_creator_t`.
"""


from .code_creator import code_creator_t
from .code_creator import separator_t
from .compound import compound_t

from .algorithm import (make_flatten, make_flatten_list, make_flatten_generator)
from .algorithm import creator_finder
from .algorithm import create_identifier
from .algorithm import creators_affect_on_me

from .custom import custom_t
from .custom import custom_text_t

from .declaration_based import declaration_based_t

from .registration_based import registration_based_t

from .scoped import scoped_t

from .module_body import module_body_t

from .include import include_t
from .include import import_t

from .unnamed_enum import unnamed_enum_t

from .namespace import namespace_alias_t
from .namespace import namespace_using_t
from .namespace import namespace_as_pyclass_t

from .enum import enum_t
from .enum import pyenum_t

from .calldef import free_function_t
from .calldef import mem_fun_t
from .calldef import make_constructor_t

from .calldef import mem_fun_pv_t
from .calldef import mem_fun_pv_wrapper_t
from .calldef import mem_fun_v_t
from .calldef import mem_fun_v_wrapper_t

from .calldef import mem_fun_protected_t
from .calldef import mem_fun_protected_wrapper_t
from .calldef import mem_fun_protected_s_t
from .calldef import mem_fun_protected_s_wrapper_t
from .calldef import mem_fun_protected_v_t
from .calldef import mem_fun_protected_v_wrapper_t
from .calldef import mem_fun_protected_pv_t
from .calldef import mem_fun_protected_pv_wrapper_t

from .calldef import mem_fun_private_v_wrapper_t
from .calldef import mem_fun_private_pv_wrapper_t

from .calldef import operator_t
from .calldef import constructor_t
from .calldef import static_method_t
from .calldef import casting_operator_t
from .calldef import mem_fun_overloads_t
from .calldef import free_fun_overloads_t
from .calldef import destructor_wrapper_t
from .calldef import casting_constructor_t
from .calldef import constructor_wrapper_t
from .calldef import mem_fun_overloads_class_t
from .calldef import casting_member_operator_t
from .calldef import free_fun_overloads_class_t
from .calldef import copy_constructor_wrapper_t
from .calldef import null_constructor_wrapper_t

from .calldef_transformed import mem_fun_transformed_t
from .calldef_transformed import mem_fun_transformed_wrapper_t
from .calldef_transformed import free_fun_transformed_t
from .calldef_transformed import free_fun_transformed_wrapper_t
from .calldef_transformed import mem_fun_v_transformed_t
from .calldef_transformed import mem_fun_v_transformed_wrapper_t
#TODO: FT for constructor
#~ from calldef_transformed import constructor_transformed_t
#~ from calldef_transformed import constructor_transformed_wrapper_t

from .calldef_ctypes import function_definition_t
from .calldef_ctypes import opaque_init_introduction_t

from .global_variable import global_variable_base_t
from .global_variable import global_variable_t
from .global_variable import array_gv_t
from .global_variable import array_gv_wrapper_t
from .global_variable import global_variable_addressof_t
from .global_variable import global_variable_reference_t

from .member_variable import member_variable_base_t
from .member_variable import member_variable_t
from .member_variable import member_variable_wrapper_t
from .member_variable import bit_field_t
from .member_variable import bit_field_wrapper_t
from .member_variable import array_mv_t
from .member_variable import array_mv_wrapper_t
from .member_variable import mem_var_ref_t
from .member_variable import mem_var_ref_wrapper_t
from .member_variable import member_variable_addressof_t
from .member_variable import fields_definition_t

from .class_declaration import class_t
from .class_declaration import class_wrapper_t
from .class_declaration import class_declaration_t
from .class_declaration import class_introduction_t
from .class_declaration import class_declaration_introduction_t

from .instruction import instruction_t

from .include_directories import include_directories_t

from .license import license_t

from .module import bpmodule_t
from .module import ctypes_module_t

from .smart_pointers import held_type_t
from .smart_pointers import smart_pointers_converter_t
from .smart_pointers import smart_pointer_registrator_t

from .target_configuration import target_configuration_t

from .array_1_registrator import array_1_registrator_t

from .indexing_suites import indexing_suite1_t
from .indexing_suites import indexing_suite2_t
from .indexing_suites import value_traits_t

from .exception_translator import exception_translator_t
from .exception_translator import exception_translator_register_t

from .opaque_type_registrator import opaque_type_registrator_t

from .properties import property_t

from .ctypes_integration_creators import expose_this_t
from .ctypes_integration_creators import expose_sizeof_t

from .bookmark import bookmark_t

from .library_reference import library_reference_t
from .name_mappings import name_mappings_t

from .typedef_as_pyvar import typedef_as_pyvar_t
from .embedded_code_repository import embedded_code_repository_t
