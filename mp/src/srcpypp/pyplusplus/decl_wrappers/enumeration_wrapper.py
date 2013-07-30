# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class that configure enumeration declaration exposing"""

from . import decl_wrapper
from pyplusplus import messages
from pygccxml import declarations

class enumeration_t(decl_wrapper.decl_wrapper_t, declarations.enumeration_t):
    """defines a set of properties, that will instruct `Py++` how to expose the enumeration

    By default, `Py++` will export all enumeration values.
    """
    def __init__(self, *arguments, **keywords):
        declarations.enumeration_t.__init__(self, *arguments, **keywords )
        decl_wrapper.decl_wrapper_t.__init__( self )

        # A dict with new names for particular enumeration values
        # Key: Original name as it appears in the C++ source file
        # Value: New name as it should appear in the Python bindings
        self._value_aliases = {}

        # A list of enumeration names (C++ names, not aliases!) that should be
        # exported.
        # By default, export all values
        self._export_values = None

    def _get_value_aliases(self):
        return self._value_aliases
    def _set_value_aliases(self, value_aliases):
        self._value_aliases = value_aliases
    value_aliases = property( _get_value_aliases, _set_value_aliases, doc=
                              """A translation table from C++ enumeration value names to desired Python names.
                              @type: dict""")

    def _get_export_values(self):
        if self._export_values is None:
            return [x[0] for x in self.values]
        else:
            return self._export_values
    def _set_export_values(self, export_values):
        self._export_values = export_values
    export_values = property( _get_export_values, _set_export_values, doc=
                              """A list of (C++) enumeration names that should be exported.
                              @type: list""")

    def _get_no_export_values(self):
        all_values = [x[0] for x in self.values]
        export_values = self.export_values
        res = []
        for name in all_values:
            if name not in export_values:
                res.append(name)
        return res

    def _set_no_export_values(self, no_export_values):
        all_values = [x[0] for x in self.values]
        export_values = []
        for name in all_values:
            if name not in no_export_values:
                export_values.append(name)
        self.export_values = export_values

    no_export_values = property( _get_no_export_values, _set_export_values, doc=
                              """A list of (C++) enumeration names that should not be exported.
                              @type: list""")

    def _readme_impl( self ):
        msgs = []
        if self.name:
            name2value = self.get_name2value_dict()
            if len( set( name2value.keys() ) ) != len( set( name2value.values() ) ):
                msgs.append( messages.W1032 )
        return msgs

    def _exportable_impl( self ):
        if not self.parent.name:
            return messages.W1057 % str( self )
        return ''
