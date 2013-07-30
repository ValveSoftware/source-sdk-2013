# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import code_creator

class license_t(code_creator.code_creator_t):
    """
    This class allows user to put his license on the top of every generated file.
    License text will be generated as is.
    """
    def __init__(self, text ):
        code_creator.code_creator_t.__init__(self)
        self._text = text

    def _get_text(self):
        return self._text
    def _set_text(self, new_text):
        self._text = new_text
    text = property( _get_text, _set_text )

    def _get_system_files_impl( self ):
        return []

    def _create_impl(self):
        return self.text
    
    def beautify( self, code ):
        return code