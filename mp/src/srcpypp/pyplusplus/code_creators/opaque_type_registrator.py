# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator
from . import declaration_based

class opaque_type_registrator_t( code_creator.code_creator_t
                                 , declaration_based.declaration_based_t ):
    """
    This class creates code that register static sized array
    """
    def __init__( self, pointee ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, pointee )
        self.works_on_instance = False

    def _create_impl(self):
        return 'BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID( %s )' % self.decl_identifier
    
    def _get_system_files_impl( self ):
        return []
