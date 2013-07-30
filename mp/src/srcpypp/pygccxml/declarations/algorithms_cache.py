# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines class that will keep results of different calculations.
"""


class declaration_algs_cache_t( object ):
    def __init__( self ):
        object.__init__( self )
        self._enabled = True
        self._full_name = None
        self._full_partial_name = None
        self._access_type = None
        self._demangled_name = None
        self._declaration_path = None
        self._partial_declaration_path = None
        self._container_key_type = None
        self._container_element_type = None

    def disable( self ):
        self._enabled = False

    def enable( self ):
        self._enabled = True

    @property
    def enabled( self ):
        return self._enabled

    def _get_full_name( self ):
        return self._full_name
    def _set_full_name( self, fname ):
        if not self.enabled:
            fname = None
        self._full_name = fname
    full_name = property( _get_full_name, _set_full_name )

    def _get_full_partial_name( self ):
        return self._full_partial_name
    def _set_full_partial_name( self, fname ):
        if not self.enabled:
            fname = None
        self._full_partial_name = fname
    full_partial_name = property( _get_full_partial_name, _set_full_partial_name )

    def _get_access_type( self ):
        return self._access_type
    def _set_access_type( self, access_type ):
        if not self.enabled:
            access_type = None
        self._access_type = access_type
    access_type = property( _get_access_type, _set_access_type )

    def _get_demangled_name( self ):
        return self._demangled_name
    def _set_demangled_name( self, demangled_name ):
        if not self.enabled:
            demangled_name = None
        self._demangled_name = demangled_name
    demangled_name = property( _get_demangled_name, _set_demangled_name )

    def _get_declaration_path( self ):
        return self._declaration_path
    def _set_declaration_path( self, declaration_path ):
        if not self.enabled:
            declaration_path = None
        self._declaration_path = declaration_path
    declaration_path = property( _get_declaration_path, _set_declaration_path )

    def _get_partial_declaration_path( self ):
        return self._partial_declaration_path
    def _set_partial_declaration_path( self, partial_declaration_path ):
        if not self.enabled:
            partial_declaration_path = None
        self._partial_declaration_path = partial_declaration_path
    partial_declaration_path = property( _get_partial_declaration_path
                                             , _set_partial_declaration_path )

    def _get_container_element_type( self ):
        return self._container_element_type
    def _set_container_element_type( self, etype ):
        if not self.enabled:
            etype = None
        self._container_element_type = etype
    container_element_type = property( _get_container_element_type, _set_container_element_type )

    def _get_container_key_type( self ):
        return self._container_key_type
    def _set_container_key_type( self, ktype ):
        if not self.enabled:
            ktype = None
        self._container_key_type = ktype
    container_key_type = property( _get_container_key_type, _set_container_key_type )

    def reset( self ):
        self.full_name = None
        self.full_partial_name = None
        self.access_type = None
        self.demangled_name = None
        self.declaration_path = None
        self.partial_declaration_path = None
        self.container_key_type = None
        self.container_element_type = None

    def reset_name_based( self ):
        self.full_name = None
        self.full_partial_name = None
        self.demangled_name = None
        self.declaration_path = None
        self.partial_declaration_path = None
        self.container_key_type = None
        self.container_element_type = None

    def reset_access_type( self ):
        self.access_type = None

class type_algs_cache_t( object ):
    enabled = True

    @staticmethod
    def disable():
        type_algs_cache_t.enabled = False

    @staticmethod
    def enable():
        type_algs_cache_t.enabled = True

    def __init__( self ):
        object.__init__( self )
        self._remove_alias = None

    def _get_remove_alias( self ):
        return self._remove_alias
    def _set_remove_alias( self, remove_alias ):
        if not type_algs_cache_t.enabled:
            remove_alias = None
        self._remove_alias = remove_alias

    remove_alias = property( _get_remove_alias, _set_remove_alias )

    def reset(self):
        self.remove_alias = None

