# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator
from pyplusplus import decl_wrappers

#TODO: add different dlls

#TODO: another idea is a small refactoring in ctypes:
#the idea is to introduce single "load shared library function", which servers
#as a factory for function defined in the library:
# class shared_library:
#     def function( self, name_or_ordinal, calling convention, restype, argtypes )

#TODO: why I need to preload "C" runtime ( other libraries )?

#TODO: how I can find out the full path of the loaded dll

class library_reference_t(code_creator.code_creator_t):
    """Creates reference to a library"""

    def __init__( self, library_path, library_var_name=None, is_cpp_library=True ):
        code_creator.code_creator_t.__init__(self)
        self._library_path = library_path
        self._is_cpp_library = is_cpp_library
        self._library_var_name = self.__create_library_var_name( library_path, library_var_name )

    @property
    def library_var_name(self):
        return self._library_var_name

    def __create_library_var_name( self, library_path, library_var_name ):
        if library_var_name:
            return library_var_name
        else:
            basename = os.path.splitext( os.path.basename( library_path ) )[0]
            name = basename.split( '.' )[0]
            return decl_wrappers.algorithm.create_valid_name( name ) + '_lib'

    def _create_impl(self):
        return '%(var)s = ctypes.%(loader)s( r"%(path)s" )' \
               % dict( var=self.library_var_name
                       , loader='CDLL'
                       , path=self._library_path )

    def _get_system_files_impl( self ):
        return []

if __name__ == '__main__':
    lr = library_reference_t( 'library', r'c:\temp\x1.dll', False )
    print(lr.create())
    lr = library_reference_t( 'library', r'c:\temp\x1.dll', True )
    print(lr.create())
