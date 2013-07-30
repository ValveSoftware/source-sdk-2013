# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator
from . import ctypes_formatter
from . import declaration_based

class typedef_as_pyvar_t(code_creator.code_creator_t, declaration_based.declaration_based_t):
    def __init__( self, ns ):
        code_creator.code_creator_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, ns )

    def _create_impl(self):
        return "%(complete_py_name)s = %(type)s" \
                % dict( complete_py_name=self.complete_py_name
                        , type=ctypes_formatter.as_ctype( self.declaration.type, self.top_parent.treat_char_ptr_as_binary_data ) )

    def _get_system_files_impl( self ):
        return []
