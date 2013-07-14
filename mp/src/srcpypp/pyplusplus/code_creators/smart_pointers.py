# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import declaration_based
from . import registration_based
from pygccxml import declarations

templates = declarations.templates

class held_type_t(object):
    """
    Helper class that can hold smart pointer name and create identifier for the
    held type from that given a creator.
    """
    def __init__( self, smart_ptr ):
        """
        :param smart_ptr: smart pointer type as string
        """
        object.__init__( self )
        self._smart_ptr = smart_ptr

    def _get_smart_ptr( self ):
        return self._smart_ptr
    def _set_smart_ptr( self, ptr ):
        self._smart_ptr = ptr
    smart_ptr = property( _get_smart_ptr, _set_smart_ptr )

    def create( self, creator):
        """ Return string of type to use for held type.
            Ex: `boost::shared_ptr` class
        """
        smart_ptr = algorithm.create_identifier( creator, self.smart_ptr )
        arg = algorithm.create_identifier( creator, creator.declaration.decl_string )
        return templates.join( smart_ptr, [ arg ] )

class smart_pointer_registrator_t( registration_based.registration_based_t
                                   , declaration_based.declaration_based_t ):
    """
    Converter for `boost::python::register_ptr_to_python`.

    Lets boost python know that it can use `smart_ptr` to hold a an object.
    See: http://www.boost.org/libs/python/doc/v2/register_ptr_to_python.html
    """
    def __init__( self, smart_ptr, class_creator ):
        """`smart_ptr`: string of pointer type.  Ex: `boost::shared_ptr`"""
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, class_creator.declaration )
        self._smart_ptr = smart_ptr
        self._class_creator = class_creator
        self.works_on_instance = False

    def _get_smart_ptr( self ):
        return self._smart_ptr
    def _set_smart_ptr( self, ptr ):
        self._smart_ptr = ptr
    smart_ptr = property( _get_smart_ptr, _set_smart_ptr )

    def _get_class_creator( self ):
        return self._class_creator
    def _set_class_creator( self, cc ):
        self._class_creator = cc
    class_creator = property( _get_class_creator, _set_class_creator )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''
        if self.class_creator \
           and self.class_creator.held_type \
           and isinstance( self.class_creator.held_type, held_type_t ) \
           and self.class_creator.held_type.smart_ptr == self.smart_ptr \
           and self.target_configuration.boost_python_has_wrapper_held_type \
           and not self.class_creator.declaration.require_self_reference:
            return '' #boost.python does it automaticly
        rptp = algorithm.create_identifier( self, '::boost::python::register_ptr_to_python' )
        held_type = held_type_t(self.smart_ptr).create( self )
        return templates.join( rptp, [ held_type ] ) + '();'

    def _get_system_files_impl( self ):
        return []

class smart_pointers_converter_t( registration_based.registration_based_t
                                  , declaration_based.declaration_based_t ):
    """ creator for boost::python::implicitly_convertible.
        This creates a statement that allows the usage of C++ implicit
        conversion from source to target.
        See: http://www.boost.org/libs/python/doc/v2/implicit.html
    """
    def __init__( self, smart_ptr, source, target ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, source )
        self._target = target
        self._smart_ptr = smart_ptr
        self.works_on_instance = False

    def _get_target(self):
        return self._target
    target = property( _get_target )

    def _get_source(self):
        return self.declaration
    source = property( _get_source )

    def _get_smart_ptr( self ):
        return self._smart_ptr
    def _set_smart_ptr( self, ptr ):
        self._smart_ptr = ptr
    smart_ptr = property( _get_smart_ptr, _set_smart_ptr )

    def _instantiate_smart_ptr( self, decl ):
        identifier = algorithm.create_identifier( self, decl.partial_decl_string )
        return templates.join( self.smart_ptr, [identifier] )

    def _create_impl(self):
        implicitly_convertible = algorithm.create_identifier( self, '::boost::python::implicitly_convertible' )
        from_arg = self._instantiate_smart_ptr( self.source )
        to_arg = self._instantiate_smart_ptr( self.target )
        return templates.join(implicitly_convertible, [ from_arg, to_arg ] ) + '();'

    def _get_system_files_impl( self ):
        return []
