# Copyright 2006 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines :class:function_transformation_t class"""

import hashlib
from . import controllers
from pygccxml import declarations
from pyplusplus import code_repository

class function_transformation_t:
    """the class holds function transformation definition - all transformations that should be applied"""
    def __init__(self, function, transformer_creator, **keywd):
        self.__function = function
        self.__controller = None
        if isinstance( function.parent, declarations.class_t ):
            if declarations.VIRTUALITY_TYPES.NOT_VIRTUAL == function.virtuality:
                self.__controller = controllers.mem_fun_controller_t( function )
            elif declarations.VIRTUALITY_TYPES.PURE_VIRTUAL == function.virtuality:
                self.__controller = controllers.pure_virtual_mem_fun_controller_t( function )
            else:
                self.__controller = controllers.virtual_mem_fun_controller_t( function )
        else:
            self.__controller = controllers.free_fun_controller_t( function )
        self.__transformers = [tr_creator( function ) for tr_creator in transformer_creator]
        self.__thread_safe = keywd.get( 'thread_safe', False )
        self.__controller.apply( self.__transformers )
        self.__unique_name = None
        self.__alias = keywd.get( 'alias', None )

    @property
    def unique_name( self ):
        if None is self.__unique_name:
            obj = hashlib.md5()
            if self.__function.mangled: # free functions don't have a mangled value
                obj.update( self.__function.mangled.encode() )
            else:
                obj.update( self.__function.decl_string.encode() )
                obj.update( self.__function.location.file_name.encode() )
                obj.update( str( self.__function.location.line ).encode() )
            self.__unique_name = self.__function.name + '_' + obj.hexdigest ()
        return self.__unique_name

    @property
    def alias( self ):
        if None is self.__alias:
            if self.__function.overloads:
                self.__alias = self.unique_name
            else:
                self.__alias = self.__function.alias
        return self.__alias

    @property
    def transformers( self ):
        return self.__transformers

    @property
    def controller( self ):
        return self.__controller

    def required_headers( self ):
        headers = []
        for transformer in self.transformers:
            headers.extend( transformer.required_headers() )
        if self.__function.call_policies:
            headers.append( code_repository.call_policies.file_name )
        return headers

    @property
    def thread_safe( self ):
        return self.__thread_safe
