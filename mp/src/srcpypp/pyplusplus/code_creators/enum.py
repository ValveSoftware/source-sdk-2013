# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import code_creator
from . import declaration_based
from . import registration_based

class enum_t( registration_based.registration_based_t
              , declaration_based.declaration_based_t ):
    """
    create code that exposes C++ enumeration
    """
    def __init__(self, enum ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=enum)
        self.works_on_instance = False

    def _get_value_aliases(self):
        return self.declaration.value_aliases
    def _set_value_aliases(self, value_aliases):
        self.declaration.value_aliases = value_aliases
    value_aliases = property( _get_value_aliases, _set_value_aliases )

    def _get_export_values(self):
        return self.declaration.export_values
    def _set_export_values(self, export_values):
        self.declaration.export_values = export_values
    export_values = property( _get_export_values, _set_export_values )

    def _generate_value_code(self, value_name):
        #in C++ you can't write namespace::enum::value, you should write namespace::value
        full_name = self.declaration.parent.decl_string
        return '.value("%(alias)s", %(name)s)' \
               % { 'alias' : self.value_aliases.get( value_name, value_name )
                    , 'name' : algorithm.create_identifier( self, full_name + '::' + value_name ) }

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''

        bpl_enum = '%(bpl::enum_)s< %(name)s>("%(alias)s")' \
                   % { 'bpl::enum_' : algorithm.create_identifier( self, '::boost::python::enum_' )
                       , 'name' : algorithm.create_identifier( self, self.declaration.decl_string )
                       , 'alias' : self.alias }

        values = []
        # Add the values that should be exported
        for value_name in self.declaration.export_values:
            values.append( self._generate_value_code( value_name ) )

        # Export the values
        if len(self.declaration.export_values)>0:
            values.append( '.export_values()' )

        # Add the values that should not be exported
        for name in self.declaration.no_export_values:
            values.append( self._generate_value_code( name ) )

        values.append( ';' )

        values = self.indent( os.linesep.join( values ) )
        return bpl_enum + os.linesep + values

    def _get_system_files_impl( self ):
        return []


class pyenum_t( code_creator.code_creator_t, declaration_based.declaration_based_t ):
    def __init__( self, enum ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=enum )

    def _create_impl( self ):
        result = [ 'class %(alias)s( ctypes_utils.Enumeration ):' % dict( alias=self.alias ) ]
        for name, value in self.declaration.values:
            result.append( self.indent( '%(name)s = %(value)s' % dict( name=name, value=value ) ) )
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []
