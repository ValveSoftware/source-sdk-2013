# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator


class name_mappings_t(code_creator.code_creator_t):
    """creates dictionary { [un]decorated name : [un]decorated name }"""

    def __init__( self, exported_symbols ):
        code_creator.code_creator_t.__init__(self)
        self._exported_symbols = exported_symbols

    def _create_impl(self):
        tmpl = '"%s" : "%s", '
        items_decorated = []
        items_undecorated = []
        for blob, decl in self._exported_symbols.items():
            items_decorated.append( tmpl % ( blob, str(decl) ) )
            items_undecorated.append( tmpl % ( str(decl), blob ) )

        result = []
        result.append( '%s.undecorated_names = {#mapping between decorated and undecorated names'
                       % self.top_parent.library_var_name )
        for s in items_undecorated:
            result.append( self.indent( s ) )
        for s in items_decorated:
            result.append( self.indent( s ) )
        result.append( '}' )
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []


if __name__ == '__main__':
    data = { 'a' : 'AA', 'b' : 'BB' }
    nm = name_mapping_t( 'name_mapping', data )
    print(nm.create())
