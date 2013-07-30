# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import code_creator

class custom_t(code_creator.code_creator_t):
    def __init__(self, works_on_instance=True):
        """ Base class for custom code.
            works_on_instance: If true, the custom code can be applied directly to obj inst.
            Ex: ObjInst."CustomCode"
        """
        code_creator.code_creator_t.__init__(self)
        self.works_on_instance = works_on_instance
    
    def _create_impl(self):
        raise NotImplementedError()
    
    def _get_system_files_impl( self ):
        return []

class custom_text_t(custom_t):
    def __init__(self, text, works_on_instance=True):
        custom_t.__init__(self, works_on_instance)
        self._text = text

    def _get_text(self):
        return self._text
    def _set_text(self, new_text):
        self._text = new_text
    text = property( _get_text, _set_text )

    def _create_impl(self):
        return self.text
    
    def _get_system_files_impl( self ):
        return []
