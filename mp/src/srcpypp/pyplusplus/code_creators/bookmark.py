# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import compound

class bookmark_t(compound.compound_t):
    def __init__( self, comment='' ):
        compound.compound_t.__init__(self )
        self.comment = ''

    def _create_impl(self):
        return compound.compound_t.create_internal_code( self.creators, indent_code=False )

    def _get_system_files_impl( self ):
        return []
