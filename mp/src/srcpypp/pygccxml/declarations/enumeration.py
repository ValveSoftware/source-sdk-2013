# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines class, that describes C++ `enum`
"""

import copy
import types
from . import compilers
from . import declaration

class enumeration_t( declaration.declaration_t ):
    """
    describes C++ `enum`
    """
    def __init__( self, name='', values=None ):
        """creates class that describes C++ `enum` declaration

        The items of the list 'values' may either be strings containing
        the enumeration value name or tuples (name, numeric value).

        :param name: `enum` name
        :type name: str
        :param parent: Parent declaration
        :type parent: declaration_t
        :param values: Enumeration values
        :type values: list
        """
        declaration.declaration_t.__init__( self, name )

        # A list of tuples (valname(str), valnum(int)). The order of the list should
        # be the same as the order in the C/C++ source file.
        self._values = []

        # Initialize values via property access
        self.values = values
        self._byte_size = 0
        self._byte_align = 0

    def __eq__(self, other):
        if not declaration.declaration_t.__eq__( self, other ):
            return False
        return self.values == other.values

    def __hash__(self): return super.__hash__(self)

    def _get__cmp__items( self ):
        """implementation details"""
        return [self.values]

    def _get_values(self):
        return copy.copy(self._values)
    def _set_values(self, values):
        self._values = []
        # None is treated like an empty list
        if (values==None):
            return
        # Check that we have indeed a list...
        if type(values)!=list:
            raise ValueError("'values' must be a list (got a %s instead)"%type(values).__name__)
        # Append the items individually. This has the effect that there's
        # some additional type checking and that a copy of 'values' is stored
        # and the caller cannot further manipulate the list via his own reference
        for item in values:
            if isinstance(item, str):
                self.append_value(item)
            elif type(item)==tuple:
                name,num = item
                self.append_value(name, num)
            else:
                raise ValueError("'values' contains an invalid item: %s"%item)
    values = property( _get_values, _set_values
                       , doc="""A list of tuples (valname(str), valnum(int)) that contain the enumeration values.
                       @type: list""")

    def append_value(self, valuename, valuenum=None):
        """Append another enumeration value to the `enum`.

        The numeric value may be None in which case it is automatically determined by
        increasing the value of the last item.

        When the 'values' attribute is accessed the resulting list will be in the same
        order as append_value() was called.

        :param valuename: The name of the value.
        :type valuename: str
        :param valuenum: The numeric value or None.
        :type valuenum: int
        """
        # No number given? Then use the previous one + 1
        if valuenum==None:
            if len(self._values)==0:
                valuenum = 0
            else:
                valuenum = self._values[-1][1]+1

        # Store the new value
        self._values.append((valuename, int(valuenum)))

    def has_value_name(self, name):
        """Check if this `enum` has a particular name among its values.

        :param name: Enumeration value name
        :type name: str
        :rtype: True if there is an enumeration value with the given name
        """
        for val,num in self._values:
            if val==name:
                return True
        return False

    def get_name2value_dict( self ):
        """returns a dictionary, that maps between `enum` name( key ) and `enum` value( value )"""
        x = {}
        for val, num in self._values:
            x[val] = num
        return x

    def i_depend_on_them( self, recursive=True ):
        return []

    def _get_byte_size(self):
        return self._byte_size
    def _set_byte_size( self, new_byte_size ):
        self._byte_size = new_byte_size
    byte_size = property( _get_byte_size, _set_byte_size
                          , doc="Size of this class in bytes @type: int")

    def _get_byte_align(self):
        return self._byte_align
    def _set_byte_align( self, new_byte_align ):
        self._byte_align = new_byte_align
    byte_align = property( _get_byte_align, _set_byte_align
                          , doc="Alignment of this class in bytes @type: int")

