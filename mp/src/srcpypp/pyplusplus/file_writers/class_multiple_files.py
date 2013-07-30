# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines a class that writes :class:`code_creators.bpmodule_t` to multiple files,
the class also splits huge C++ classes to few source files.
"""

import os
from . import writer
from . import multiple_files
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators
from pyplusplus import utils as pypp_utils

class class_multiple_files_t(multiple_files.multiple_files_t):
    """
    This class will split code, generated for huge classes, to few files.
    The following strategy will be used:

      * New directory, named after a class alias, will be created.

      * `Py++` will generate

        * wrapper header - header that will contain code generated for class wrappers
        * classes h/cpp - will contain registration code for internal classes
        * `memfun` h/cpp - will contain registration code for member functions

    The main class registration function will be placed in "%(class alias)s_main"
    header and source files.
    """

    def __init__( self
                  , extmodule
                  , directory_path
                  , huge_classes
                  , num_of_functions_per_file=20
                  , files_sum_repository=None
                  , encoding='ascii'):
        multiple_files.multiple_files_t.__init__(self
                                                 , extmodule
                                                 , directory_path
                                                 , files_sum_repository=files_sum_repository
                                                 , encoding=encoding)
        self.huge_classes = huge_classes
        self.num_of_functions_per_file = num_of_functions_per_file
        self.internal_splitters = [
            self.split_internal_enums
            , self.split_internal_unnamed_enums
            , self.split_internal_classes
            , self.split_internal_memfuns
            , self.split_internal_v_memfuns
            , self.split_internal_pv_memfuns
            , self.split_internal_protected_memfuns
            #not supported yet
            #, self.split_internal_member_variables
        ]

    def create_base_fname( self, class_creator, pattern ):
        return "_%s__%s" % ( class_creator.alias, pattern )

    def wrapper_header( self, class_creator ):
        return self.create_base_fname( class_creator, 'wrapper' + self.HEADER_EXT )

    def write_wrapper( self, class_creator ):
        answer = []
        if self.extmodule.license:
            answer.append( self.extmodule.license.create() )

        creators = [class_creator]
        if class_creator.wrapper:
            creators.append( class_creator.wrapper )

        answer.append( self.create_include_code( creators ) )
        answer.append( '' )
        answer.append( self.create_namespaces_code( creators ) )

        if class_creator.wrapper:
            answer.append( class_creator.wrapper.create() )
            class_creator.wrapper.create = lambda: ''

        answer.append( '' )
        answer.append( class_creator.create_typedef_code() )

        code = os.linesep.join( answer )
        wrapper_code = self.create_header( self.create_base_fname(class_creator, 'wrapper'), code )
        header_file = os.path.join( self.directory_path, self.wrapper_header(class_creator) )
        self.write_file( header_file, wrapper_code )

    def split_internal_creators( self, class_creator, creators, pattern ):
        file_path = os.path.join( self.directory_path
                                  , self.create_base_fname( class_creator, pattern ) )

        function_name = 'register_%(cls_alias)s_%(pattern)s' \
                        % { 'cls_alias' : class_creator.alias, 'pattern' : pattern }

        function_decl = 'void %(fname)s( %(exposer_type)s& %(var_name)s )' \
                        % { 'fname' : function_name
                            , 'exposer_type' : class_creator.typedef_name
                            , 'var_name' : class_creator.class_var_name }

        #writting header file
        header_code = [ '#include "%s"' % self.wrapper_header( class_creator ) ]
        header_code.append( '' )
        header_code.append( function_decl + ';' )
        self.write_file( file_path + self.HEADER_EXT
                         , self.create_header( class_creator.alias + '_' + pattern
                                               , os.linesep.join(header_code) ) )

        #writting source file
        source_code = []
        if self.extmodule.license:
            source_code.append( self.extmodule.license.create() )

        #relevant header file
        head_headers = [ self.create_base_fname( class_creator, pattern + self.HEADER_EXT ) ]
        source_code.append( self.create_include_code( creators, tail_headers=head_headers ) )

        source_code.append( '' )
        source_code.append( self.create_namespaces_code( creators ) )

        for creator in creators:
            for decl_creator in self.associated_decl_creators( creator ):
                source_code.append( '' )
                source_code.append( decl_creator.create() )
                if not isinstance( decl_creator, self.ref_count_creators ):
                    decl_creator.create = lambda: ''

        # Write the register() function...
        source_code.append( '' )
        source_code.append( '%s{' % function_decl )
        source_code.append( '' )
        for index, creator in enumerate( creators ):
            source_code.append( code_creators.code_creator_t.indent( creator.create() ) )
            source_code.append( '' )
            if 0 == index:
                creator.create = lambda: function_name + '(%s);' % class_creator.class_var_name
            else:
                creator.create = lambda: ''
        source_code.append( '}' )
        self.write_file( file_path + self.SOURCE_EXT, os.linesep.join( source_code ) )

    def split_internal_enums( self, class_creator ):
        """Write all enumerations into a separate .h/.cpp file.
        """
        enums_creators = [x for x in class_creator.creators if isinstance(x, code_creators.enum_t )]
        self.split_internal_creators( class_creator, enums_creators, 'enums' )
        return 'enums'

    def split_internal_unnamed_enums( self, class_creator ):
        creators = [x for x in class_creator.creators if isinstance(x, code_creators.unnamed_enum_t )]
        self.split_internal_creators( class_creator, creators, 'unnamed_enums' )
        return 'unnamed_enums'

    def split_internal_calldefs( self, class_creator, calldef_types, pattern ):
        creators = [x for x in class_creator.creators if isinstance(x, calldef_types )]
        grouped_creators = pypp_utils.split_sequence( creators, self.num_of_functions_per_file )
        if len( grouped_creators ) == 1:
            for creator in creators:
                creator.works_on_instance = False
            self.split_internal_creators( class_creator, creators, pattern )
            return pattern
        else:
            patterns = []
            for index, group in enumerate( grouped_creators ):
                pattern_tmp = pattern + str( index )
                patterns.append( pattern_tmp )
                for creator in group:
                    creator.works_on_instance = False
                self.split_internal_creators( class_creator, group, pattern_tmp )
            return patterns

    def split_internal_memfuns( self, class_creator ):
        calldef_types = ( code_creators.mem_fun_t, code_creators.mem_fun_overloads_t )
        return self.split_internal_calldefs( class_creator, calldef_types, 'memfuns' )

    def split_internal_v_memfuns( self, class_creator ):
        calldef_types = ( code_creators.mem_fun_v_t )
        return self.split_internal_calldefs( class_creator, calldef_types, 'memfuns_virtual' )

    def split_internal_pv_memfuns( self, class_creator ):
        calldef_types = ( code_creators.mem_fun_pv_t )
        return self.split_internal_calldefs( class_creator, calldef_types, 'memfuns_pvirtual' )

    def split_internal_protected_memfuns( self, class_creator ):
        calldef_types = (
            code_creators.mem_fun_protected_t
            , code_creators.mem_fun_protected_s_t
            , code_creators.mem_fun_protected_v_t
            , code_creators.mem_fun_protected_pv_t )
        return self.split_internal_calldefs( class_creator, calldef_types, 'protected_memfuns' )

    def split_internal_classes( self, class_creator ):
        class_types = ( code_creators.class_t, code_creators.class_declaration_t )
        creators = [x for x in class_creator.creators if isinstance(x, class_types )]
        self.split_internal_creators( class_creator, creators, 'classes' )
        return 'classes'

    def split_internal_member_variables( self, class_creator ):
        creators = [x for x in class_creator.creators if isinstance(x, code_creators.member_variable_base_t)]
        self.split_internal_creators( class_creator, creators, 'memvars' )
        return 'memvars'

    def split_class_impl( self, class_creator):
        if not class_creator.declaration in self.huge_classes:
            return super( class_multiple_files_t, self ).split_class_impl( class_creator )

        class_creator.declaration.always_expose_using_scope = True

        function_name = 'register_%s_class' % class_creator.alias
        file_path = os.path.join( self.directory_path, class_creator.alias )
        # Write the .h file...
        header_name = file_path + self.HEADER_EXT
        self.write_file( header_name
                         , self.create_header( class_creator.alias
                                               , self.create_function_code( function_name ) ) )

        self.write_wrapper( class_creator )

        tail_headers = []
        for splitter in self.internal_splitters:
            pattern = splitter( class_creator )
            if not pattern:
                continue
            if isinstance( pattern, str ):
                tail_headers.append( self.create_base_fname( class_creator, pattern + self.HEADER_EXT ) )
            else:
                assert( isinstance( pattern, list ) )
                for p in pattern:
                    tail_headers.append( self.create_base_fname( class_creator, p + self.HEADER_EXT ) )
        #writting source file
        source_code = []
        if self.extmodule.license:
            source_code.append( self.extmodule.license.create() )

        source_code.append( self.create_include_code( [class_creator], tail_headers=tail_headers ) )

        source_code.append( '' )
        source_code.append( self.create_namespaces_code( [class_creator] ) )

        for creator in class_creator.associated_decl_creators:
            source_code.append( '' )
            source_code.append( creator.create() )
            if not isinstance( creator, self.ref_count_creators ):
                creator.create = lambda: ''

        # Write the register() function...
        source_code.append( '' )
        source_code.append( 'void %s(){' % function_name )
        source_code.append( '' )
        source_code.append( class_creator.create() )
        source_code.append( '' )
        source_code.append( '}' )
        self.write_file( file_path + self.SOURCE_EXT, os.linesep.join( source_code ) )

        # Replace the create() method so that only the register() method is called
        # (this is called later for the main source file).
        class_creator.create = lambda: function_name +'();'
        self.include_creators.append( code_creators.include_t( header_name ) )
        self.split_header_names.append(header_name)
        self.split_method_names.append(function_name)
