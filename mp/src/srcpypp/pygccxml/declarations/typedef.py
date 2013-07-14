# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines class that describes C++ typedef declaration
"""

from . import declaration
from . import dependencies

class typedef_t( declaration.declaration_t ):
    """describes C++ typedef declaration"""

    def __init__( self, name='', type=None ):
        """creates class that describes C++ typedef"""
        declaration.declaration_t.__init__( self, name )
        self._type = type

    def _get__cmp__items( self ):
        """implementation details"""
        return [self.type]

    def __eq__(self, other):
        if not declaration.declaration_t.__eq__( self, other ):
            return False
        return self.type == other.type

    def __hash__(self):
        return super.__hash__(self)

    def _get_type(self):
        return self._type
    def _set_type(self, type):
        self._type = type
    type = property( _get_type, _set_type
                     , doc="reference to the original :class:`type <type_t>`"    )

    def i_depend_on_them( self, recursive=True ):
        return [ dependencies.dependency_info_t( self, self.type ) ]

    @property
    def byte_size (self):
        "Size of this type in bytes @type: int"
        return self._type.byte_size

    @property
    def byte_align (self):
        "alignment of this type in bytes @type: int"
        return self._type.byte_align
