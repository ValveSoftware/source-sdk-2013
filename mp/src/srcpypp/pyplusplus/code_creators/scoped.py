# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import compound
from . import declaration_based

class scoped_t(declaration_based.declaration_based_t, compound.compound_t):
    def __init__( self, declaration ):
        declaration_based.declaration_based_t.__init__( self, declaration=declaration)
        compound.compound_t.__init__( self )
        
    def _create_impl(self):
        #template method pattern should be used.
        raise NotImplementedError()
    
