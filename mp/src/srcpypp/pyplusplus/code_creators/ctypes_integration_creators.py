# Copyright 2004-2008 Roman Yakovenko
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import code_creator
from . import declaration_based
from . import registration_based
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_repository

class  expose_this_t( registration_based.registration_based_t
                      , declaration_based.declaration_based_t ):
    """
    creates code that expose address of the object to Python
    """

    def __init__(self, class_ ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=class_)

    def _create_impl(self):
        answer = [ 'add_property' ]
        answer.append( '( ' )
        answer.append('"this"')
        answer.append( self.PARAM_SEPARATOR )
        answer.append( 'pyplus_conv::make_addressof_inst_getter< %s >()' % self.decl_identifier )
        if self.documentation:
            answer.append( self.PARAM_SEPARATOR )
            answer.append( self.documentation )
        answer.append( ' ) ' )

        return ''.join( answer )

    def _get_system_files_impl( self ):
        return [code_repository.ctypes_integration.file_name]

class  expose_sizeof_t( registration_based.registration_based_t
                        , declaration_based.declaration_based_t ):
    """
    creates code that expose address of the object to Python
    """

    def __init__(self, class_ ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=class_)

    def _create_impl(self):
        return 'def( pyplus_conv::register_sizeof( boost::type< %s >() ) )' % self.decl_identifier

    def _get_system_files_impl( self ):
        return [code_repository.ctypes_integration.file_name]
