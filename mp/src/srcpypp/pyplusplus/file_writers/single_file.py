# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines a class that writes :class:`code_creators.module_t` to single file"""

import os
from . import writer

class single_file_t(writer.writer_t):
    """generates all code into single cpp file"""

    def __init__(self, extmodule, file_name, encoding='ascii'):
        writer.writer_t.__init__(self, extmodule, encoding=encoding)
        self.__fname = file_name

    @property
    def file_name(self):
        return self.__fname
    
    def write(self):        
        target_dir = os.path.dirname( self.file_name )
        if not target_dir:
            target_dir = os.getcwd()
        if not os.path.exists( target_dir ):
            os.makedirs( target_dir )
        headers = self.get_user_headers( [self.extmodule] )
        for header in headers:
            self.extmodule.add_include( header )
        self.write_code_repository( target_dir )
        self.write_file( self.file_name, self.extmodule.create(), encoding=self.encoding )
        self.save_exposed_decls_db( target_dir )
