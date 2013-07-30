# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""contains classes that describe different C++ declarations"""

from . import compilers

from .dependencies import dependency_info_t
from .declaration import location_t
from .declaration import declaration_t
from .scopedef import scopedef_t
from .enumeration import enumeration_t
from .namespace import namespace_t

from .class_declaration import class_t
from .class_declaration import CLASS_TYPES
from .class_declaration import ACCESS_TYPES
from .class_declaration import hierarchy_info_t
from .class_declaration import class_declaration_t
from .class_declaration import class_types

from .typedef import typedef_t

from .cpptypes import type_t
from .cpptypes import dummy_type_t
from .cpptypes import unknown_t
from .cpptypes import ellipsis_t
from .cpptypes import fundamental_t
from .cpptypes import void_t
from .cpptypes import char_t
from .cpptypes import signed_char_t
from .cpptypes import unsigned_char_t
from .cpptypes import wchar_t
from .cpptypes import short_int_t
from .cpptypes import short_unsigned_int_t
from .cpptypes import bool_t
from .cpptypes import int_t
from .cpptypes import unsigned_int_t
from .cpptypes import long_int_t
from .cpptypes import long_unsigned_int_t
from .cpptypes import long_long_int_t
from .cpptypes import long_long_unsigned_int_t
from .cpptypes import int128_t
from .cpptypes import uint128_t
from .cpptypes import float_t
from .cpptypes import double_t
from .cpptypes import long_double_t
from .cpptypes import FUNDAMENTAL_TYPES
from .cpptypes import compound_t
from .cpptypes import volatile_t
from .cpptypes import const_t
from .cpptypes import pointer_t
from .cpptypes import reference_t
from .cpptypes import restrict_t
from .cpptypes import array_t
from .cpptypes import calldef_type_t
from .cpptypes import free_function_type_t
from .cpptypes import member_function_type_t
from .cpptypes import member_variable_type_t
from .cpptypes import declarated_t
from .cpptypes import type_qualifiers_t
#java types
from .cpptypes import java_fundamental_t
from .cpptypes import jbyte_t
from .cpptypes import jshort_t
from .cpptypes import jint_t
from .cpptypes import jlong_t
from .cpptypes import jfloat_t
from .cpptypes import jdouble_t
from .cpptypes import jchar_t
from .cpptypes import jboolean_t



from .variable import variable_t

from .algorithm import full_name
from .algorithm import full_name_from_declaration_path
from .algorithm import make_flatten
from .algorithm import apply_visitor
from .algorithm import declaration_path
from .algorithm import get_named_parent
from .algorithm import find_declaration
from .algorithm import match_declaration_t
from .algorithm import find_all_declarations
from .algorithm import find_first_declaration
from .algorithm import declaration_files
from .algorithm import visit_function_has_not_been_found_t
from .algorithm import get_global_namespace

from .calldef import VIRTUALITY_TYPES
from .calldef import FUNCTION_VIRTUALITY_TYPES
from .calldef import CALLING_CONVENTION_TYPES
from .calldef import argument_t
from .calldef import calldef_t
from .calldef import member_calldef_t
from .calldef import free_calldef_t
from .calldef import operator_t
from .calldef import member_function_t
from .calldef import constructor_t
from .calldef import destructor_t
from .calldef import member_operator_t
from .calldef import casting_operator_t
from .calldef import free_function_t
from .calldef import free_operator_t

from .decl_visitor import decl_visitor_t
from .type_visitor import type_visitor_t

from .type_traits import decompose_type
from .type_traits import decompose_class


from .type_traits import is_enum
from .type_traits import enum_declaration
from .type_traits import enum_traits

from .type_traits import is_class
from .type_traits import class_traits
from .type_traits import is_class_declaration
from .type_traits import class_declaration_traits


from .type_traits import is_bool
from .type_traits import is_same
from .type_traits import is_void
from .type_traits import is_void_pointer
from .type_traits import is_const
from .type_traits import base_type
from .type_traits import is_array
from .type_traits import is_pointer
from .type_traits import is_volatile
from .type_traits import is_integral
from .type_traits import is_reference
from .type_traits import is_arithmetic
from .type_traits import is_fundamental
from .type_traits import is_floating_point
from .type_traits import is_base_and_derived
from .type_traits import is_convertible
from .type_traits import is_noncopyable
from .type_traits import is_std_string
from .type_traits import is_std_wstring
from .type_traits import is_std_ostream
from .type_traits import is_std_wostream
from .type_traits import is_calldef_pointer

from .type_traits import is_unary_operator
from .type_traits import is_binary_operator

from .type_traits import array_size
from .type_traits import array_item_type

from .type_traits import remove_cv
from .type_traits import remove_const
from .type_traits import remove_alias
from .type_traits import remove_pointer
from .type_traits import remove_volatile
from .type_traits import remove_reference
from .type_traits import remove_declarated

from .type_traits import has_destructor
from .type_traits import has_public_less
from .type_traits import has_copy_constructor
has_trivial_copy = has_copy_constructor #backward comp mode will be removed
from .type_traits import has_public_equal
from .type_traits import has_public_assign
from .type_traits import has_public_destructor
from .type_traits import has_public_constructor
from .type_traits import has_trivial_constructor
from .type_traits import find_trivial_constructor
from .type_traits import has_public_binary_operator
from .type_traits import has_any_non_copyconstructor

from .type_traits import auto_ptr_traits
from .type_traits import smart_pointer_traits
from .type_traits import internal_type_traits

from .container_traits import list_traits
from .container_traits import deque_traits
from .container_traits import queue_traits
from .container_traits import priority_queue_traits
from .container_traits import vector_traits
from .container_traits import stack_traits
from .container_traits import map_traits
from .container_traits import multimap_traits
from .container_traits import hash_map_traits
from .container_traits import hash_multimap_traits
from .container_traits import set_traits
from .container_traits import hash_set_traits
from .container_traits import multiset_traits
from .container_traits import hash_multiset_traits
from .container_traits import find_container_traits

from .function_traits import is_same_function

all_container_traits = container_traits.container_traits
"""tuple of all STD container traits classes"""

sequential_container_traits = \
[
    list_traits
    , deque_traits
    , queue_traits
    , priority_queue_traits
    , vector_traits
    , stack_traits
    , set_traits
    , hash_set_traits
    , multiset_traits
    , hash_multiset_traits
]
"""list, that contains all STD container traits classes"""


from . import templates
from . import call_invocation

from .decl_factory import decl_factory_t

from .matchers import matcher_base_t
from .matchers import or_matcher_t
from .matchers import and_matcher_t
from .matchers import not_matcher_t
from .matchers import declaration_matcher_t
from .matchers import calldef_matcher_t
from .matchers import namespace_matcher_t
from .matchers import variable_matcher_t
from .matchers import regex_matcher_t
from .matchers import access_type_matcher_t
from .matchers import operator_matcher_t
from .matchers import custom_matcher_t
from .matchers import virtuality_type_matcher_t

#make matchers to look more like functors
or_matcher = or_matcher_t
"""see :class:`or_matcher_t` for documentation"""
and_matcher = and_matcher_t
"""see :class:`and_matcher_t` for documentation"""
not_matcher = not_matcher_t
"""see :class:`not_matcher_t` for documentation"""
declaration_matcher = declaration_matcher_t
"""see :class:`declaration_matcher_t` for documentation"""
calldef_matcher = calldef_matcher_t
"""see :class:`calldef_matcher_t` for documentation"""
namespace_matcher = namespace_matcher_t
"""see :class:`namespace_matcher_t` for documentation"""
variable_matcher = variable_matcher_t
"""see :class:`variable_matcher_t` for documentation"""
regex_matcher = regex_matcher_t
"""see :class:`regex_matcher_t` for documentation"""
access_type_matcher = access_type_matcher_t
"""see :class:`access_type_matcher_t` for documentation"""
operator_matcher = operator_matcher_t
"""see :class:`operator_matcher_t` for documentation"""
custom_matcher = custom_matcher_t
"""see :class:`custom_matcher_t` for documentation"""
virtuality_type_matcher = virtuality_type_matcher_t
"""see :class:`virtuality_type_matcher_t` for documentation"""

from .matcher import matcher

from .mdecl_wrapper import mdecl_wrapper_t

from .decl_printer import decl_printer_t
from .decl_printer import dump_declarations
from .decl_printer import print_declarations


from . import scopedef

scopedef.scopedef_t._impl_all_decl_types = \
[
    scopedef.scopedef_t
    , enumeration_t
    , namespace_t
    , class_t
    , class_declaration_t
    , typedef_t
    , variable_t
    , calldef_t
    , member_calldef_t
    , free_calldef_t
    , operator_t
    , member_function_t
    , constructor_t
    , destructor_t
    , member_operator_t
    , casting_operator_t
    , free_function_t
    , free_operator_t
]

__impl_matchers = scopedef.scopedef_t._impl_matchers
__impl_decl_types = scopedef.scopedef_t._impl_decl_types

__impl_matchers[ scopedef.scopedef_t.decl ] = declaration_matcher_t

__impl_matchers[ scopedef.scopedef_t.class_ ] = declaration_matcher_t
__impl_decl_types[ scopedef.scopedef_t.class_ ] = class_t

__impl_matchers[ scopedef.scopedef_t.variable ] = variable_matcher_t

__impl_matchers[ scopedef.scopedef_t.calldef ] = calldef_matcher_t
__impl_decl_types[ scopedef.scopedef_t.calldef ] = calldef_t

__impl_matchers[ scopedef.scopedef_t.operator ] = operator_matcher_t
__impl_decl_types[ scopedef.scopedef_t.operator ] = operator_t

__impl_matchers[ scopedef.scopedef_t.member_function ] = calldef_matcher_t
__impl_decl_types[ scopedef.scopedef_t.member_function ] = member_function_t

__impl_matchers[ scopedef.scopedef_t.constructor ] = calldef_matcher_t
__impl_decl_types[ scopedef.scopedef_t.constructor ] = constructor_t

__impl_matchers[ scopedef.scopedef_t.member_operator ] = operator_matcher_t
__impl_decl_types[ scopedef.scopedef_t.member_operator ] = member_operator_t

__impl_matchers[ scopedef.scopedef_t.member_operator ] = operator_matcher_t
__impl_decl_types[ scopedef.scopedef_t.member_operator ] = member_operator_t

__impl_matchers[ scopedef.scopedef_t.casting_operator ] = calldef_matcher_t
__impl_decl_types[ scopedef.scopedef_t.casting_operator ] = casting_operator_t

__impl_matchers[ scopedef.scopedef_t.enumeration ] = declaration_matcher_t
__impl_decl_types[ scopedef.scopedef_t.enumeration ] = enumeration_t

__impl_matchers[ scopedef.scopedef_t.typedef ] = declaration_matcher_t
__impl_decl_types[ scopedef.scopedef_t.typedef ] = typedef_t

__impl_matchers[ namespace_t.namespace ] = namespace_matcher_t

__impl_matchers[ namespace_t.free_function ] = calldef_matcher_t
__impl_decl_types[ namespace_t.free_function ] = free_function_t

__impl_matchers[ namespace_t.free_operator ] = operator_matcher_t
__impl_decl_types[ namespace_t.free_operator ] = free_operator_t

