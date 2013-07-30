# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator
from . import include_directories

class include_t(code_creator.code_creator_t):
    """
    Creates C++ code for include directive
    """
    def __init__( self, header, user_defined=False, system=False ):
        code_creator.code_creator_t.__init__(self)
        self._header = include_directories.include_directories_t.normalize( header )
        self._include_dirs_optimization = None #This parameter will be set from bpmodule_t.create function
        self._user_defined = user_defined
        self._system = system
        self.__created_code = None

    @property
    def is_user_defined(self):
        return self._user_defined

    @property
    def is_system(self):
        """Return True if header file is system( Boost.Python or `Py++` ) header file"""
        return self._system

    def _get_header(self):
        return self._header
    def _set_header(self, header):
        self._header = include_directories.include_directories_t.normalize( header )
    header = property( _get_header, _set_header )

    def _get_include_dirs_optimization(self):
        return self._include_dirs_optimization
    def _set_include_dirs_optimization(self, include_dirs):
        self._include_dirs_optimization = include_dirs
    include_dirs_optimization = property( _get_include_dirs_optimization, _set_include_dirs_optimization )

    def _create_include_directive_code(self):
        header = self.header.strip()
        if header.startswith( '"' ) or header.startswith( '<' ):
            return '#include %s' % self.header

        if not self.include_dirs_optimization:
            return '#include "%s"' % self.header
        else:
            normalize_header = self.include_dirs_optimization.normalize_header( self.header )
            if self.include_dirs_optimization.is_std(self.header):
                return '#include <%s>' % normalize_header
            else:
                return '#include "%s"' % normalize_header

    def _create_impl(self):
        if not self.__created_code:
            self.__created_code = self._create_include_directive_code()
        return self.__created_code

    def _get_system_files_impl( self ):
        return []


class import_t(code_creator.code_creator_t):
    """Creates Python import directive"""
    def __init__( self, module_name ):
        code_creator.code_creator_t.__init__(self)
        self._module_name = module_name

    def _create_impl(self):
        return 'import %(module)s' % dict( module=os.path.splitext(self._module_name)[0] )

    def _get_system_files_impl( self ):
        return []
