# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import compound
from . import code_creator
from . import declaration_based

class namespace_alias_t(code_creator.code_creator_t):
    def __init__( self, alias, full_namespace_name ):
        code_creator.code_creator_t.__init__( self )
        self._alias = alias
        self._full_namespace_name = full_namespace_name
        self.__created_code = None

    def _get_alias( self ):
        return self._alias
    def _set_alias( self, new_alias ):
        self._alias = new_alias
    alias = property( _get_alias, _set_alias )

    def _get_full_namespace_name( self ):
        return self._full_namespace_name.lstrip( '::' )
    def _set_full_namespace_name( self, new_full_namespace_name ):
        self._full_namespace_name = new_full_namespace_name
    full_namespace_name = property( _get_full_namespace_name, _set_full_namespace_name )

    def _create_impl(self):
        if self.__created_code:
            return self.__created_code
        else:
            self.__created_code = 'namespace %s = %s;' % ( self.alias, self.full_namespace_name )
            return self.__created_code

    def _get_system_files_impl( self ):
        return []

class namespace_using_t(code_creator.code_creator_t):
    def __init__( self, namespace_name ):
        code_creator.code_creator_t.__init__(self )
        self._namespace_name = namespace_name
        self.__created_code = None

    def _get_namespace_name( self ):
        return self._namespace_name.lstrip( '::' )
    def _set_namespace_name( self, new_namespace_name ):
        self._namespace_name = new_namespace_name
    namespace_name = property( _get_namespace_name, _set_namespace_name )

    def _create_impl(self):
        if self.__created_code:
            return self.__created_code
        else:
            self.__created_code = 'using namespace %s;' % self.namespace_name
            return self.__created_code

    def _get_system_files_impl( self ):
        return []


class namespace_as_pyclass_t(compound.compound_t, declaration_based.declaration_based_t):
    def __init__( self, ns ):
        compound.compound_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, ns )

    def _create_impl(self):
        result = []
        result.append( "class %s:" % self.alias )
        result.append( self.indent( '"""namespace %s"""' % self.decl_identifier ) )
        if self.creators:
            result.append( self.indent( "" ) )
        result.append( compound.compound_t.create_internal_code( self.creators ) )
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []

