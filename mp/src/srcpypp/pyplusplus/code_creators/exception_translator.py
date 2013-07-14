# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import types
from . import algorithm 
from . import code_creator
from . import declaration_based
from . import registration_based
from pygccxml import declarations

class exception_translator_t( code_creator.code_creator_t
                              , declaration_based.declaration_based_t ):
    def __init__(self, exception_class ):        
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=exception_class  )

    @property
    def translator_name( self ):
        return 'translate_%(alias)s' % { 'alias' : self.declaration.alias }
    
    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''        
        return os.linesep.join([
              "void translate_%(alias)s( const %(cls_name)s& %(arg_name)s ){" \
            , self.indent( self.declaration.exception_translation_code )
            , "}"]) \
            % { 'alias' : self.declaration.alias
                 , 'cls_name' : self.decl_identifier
                 , 'arg_name' : self.declaration.exception_argument_name }

    def _get_system_files_impl( self ):
        return []


class exception_translator_register_t( registration_based.registration_based_t
                                       , declaration_based.declaration_based_t ):
    def __init__(self, exception_class, exception_translator):        
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=exception_class )
        self.works_on_instance = False
        self.translator = exception_translator
    
    def _create_impl( self ):
        if self.declaration.already_exposed:
            return ''

        return '%(register_exception_translator)s< %(cls)s >( &%(translator)s );' \
               % { 'register_exception_translator' : algorithm.create_identifier( self, 'boost::python::register_exception_translator' )
                   , 'cls'  : self.decl_identifier
                   , 'translator' : self.translator.translator_name }
        
    def _get_system_files_impl( self ):
        return []
        