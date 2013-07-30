# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator

class instruction_t(code_creator.code_creator_t):
    """
    This class is used as a base class for different instruction for code creators.    
    """
    #TODO: add silent option and make it default
    def __init__(self, silent=True):
        code_creator.code_creator_t.__init__(self)
        self._silent = silent
        
    def get_silent(self):
        return self._silent
    
    def set_silent(self, silent ):
        self._silent = silent
        
    silent = property( get_silent, set_silent
                       , doc="""silent property controls, whether instruction 
                         should be written within generated code or not. 
                         Default value is True - not written.""" )
        
    def _create_impl(self):
        if self.silent:
            return ''
        answer = []
        for line in self._generate_description().split( os.linesep ):
            answer.append( '// %s' % line )
        return os.linesep.join( answer )
    
    def _generate_description(self):
        raise NotImplementedError()
    
    def _get_system_files_impl( self ):
        return []
