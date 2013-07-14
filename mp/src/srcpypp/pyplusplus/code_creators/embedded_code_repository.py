# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import code_creator

class embedded_code_repository_t(code_creator.code_creator_t):
    """Creates Python import directive"""
    def __init__( self, code_repository_module ):
        code_creator.code_creator_t.__init__(self)
        self.__code = code_repository_module.code

    def _create_impl(self):
        return self.__code

    def _get_system_files_impl( self ):
        return []
