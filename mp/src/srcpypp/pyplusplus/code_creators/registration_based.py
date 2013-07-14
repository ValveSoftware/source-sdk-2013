# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import algorithm
from . import code_creator

class registration_based_t(code_creator.code_creator_t):
    """Code creator that is based on a declaration.
    """
    def __init__(self, associated_decl_creators=None ):
        code_creator.code_creator_t.__init__(self)
        if None is associated_decl_creators:
            associated_decl_creators = []
        self._associated_decl_creators = associated_decl_creators
        
    @property
    def associated_decl_creators( self ):
        """ references to global declaration code creators. """
        return self._associated_decl_creators
