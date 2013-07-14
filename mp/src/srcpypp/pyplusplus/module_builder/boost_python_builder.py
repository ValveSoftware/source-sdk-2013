# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import sys
import time
import types
import warnings
from . import module_builder

from pygccxml import parser
from pygccxml import utils as pygccxml_utils
from pygccxml import declarations as decls_package

from pyplusplus import utils
from pyplusplus import _logging_
from pyplusplus import decl_wrappers
from pyplusplus import file_writers
from pyplusplus import code_creators
from pyplusplus import creators_factory

class builder_t(module_builder.module_builder_t):
    """
    This class provides users with simple and intuitive interface to `Py++`
    and/or pygccxml functionality. If this is your first attempt to use `Py++`
    consider to read tutorials. 
    """

    def __init__( self
                  , files
                  , gccxml_path=''
                  , working_directory='.'
                  , include_paths=None
                  , define_symbols=None
                  , undefine_symbols=None
                  , start_with_declarations=None
                  , compilation_mode=None
                  , cache=None
                  , optimize_queries=True
                  , ignore_gccxml_output=False
                  , indexing_suite_version=1
                  , cflags=""
                  , encoding='ascii'
                  , compiler=None
                  , gccxml_config=None):
        """
        :param files: list of files, declarations from them you want to export
        :type files: list of strings or :class:`parser.file_configuration_t` instances

        :param gccxml_path: path to gccxml binary. If you don't pass this argument,
                            pygccxml parser will try to locate it using you environment PATH variable
        :type gccxml_path: str

        :param include_paths: additional header files location. You don't have to
                              specify system and standard directories.
        :type include_paths: list of strings

        :param define_symbols: list of symbols to be defined for preprocessor.
        :param define_symbols: list of strings

        :param undefine_symbols: list of symbols to be undefined for preprocessor.
        :param undefine_symbols: list of strings

        :param cflags: Raw string to be added to gccxml command line.

        :param gccxml_config: instance of pygccxml.parser.config_t class, holds
                              gccxml( compiler ) configuration. You can use this
                              argument instead of passing the compiler configuration separately.
        """
        module_builder.module_builder_t.__init__( self, global_ns=None, encoding=encoding )

        if not gccxml_config:
            gccxml_config = parser.config_t( gccxml_path=gccxml_path
                                             , working_directory=working_directory
                                             , include_paths=include_paths
                                             , define_symbols=define_symbols
                                             , undefine_symbols=undefine_symbols
                                             , start_with_declarations=start_with_declarations
                                             , ignore_gccxml_output=ignore_gccxml_output
                                             , cflags=cflags
                                             , compiler=compiler)

        #may be in future I will add those directories to user_defined_directories to self.__code_creator.
        self.__parsed_files = list(map( pygccxml_utils.normalize_path
                                   , parser.project_reader_t.get_os_file_names( files ) ))
        tmp = [os.path.split( file_ )[0] for file_ in self.__parsed_files]
        self.__parsed_dirs = [_f for _f in tmp if _f]

        self.global_ns = self.__parse_declarations( files
                                                    , gccxml_config
                                                    , compilation_mode
                                                    , cache
                                                    , indexing_suite_version)
        self.global_ns.decls(recursive=True, allow_empty=True)._code_generator = decl_wrappers.CODE_GENERATOR_TYPES.CTYPES

        self.__code_creator = None
        if optimize_queries:
            self.run_query_optimizer()

        self.__declarations_code_head = []
        self.__declarations_code_tail = []

        self.__registrations_code_head = []
        self.__registrations_code_tail = []



    def register_module_dependency( self, other_module_generated_code_dir ):
        """
        `already_exposed` solution is pretty good when you mix hand-written
        modules with `Py++` generated. It doesn't work/scale for "true"
        multi-module development. This is exactly the reason why `Py++`
        offers "semi automatic" solution.

        For every exposed module, `Py++` generates `exposed_decl.pypp.txt` file.
        This file contains the list of all parsed declarations and whether they
        were included or excluded. Later, when you work on another module, you
        can tell `Py++` that the current module depends on the previously
        generated one. `Py++` will load `exposed_decl.pypp.txt` file and update
        the declarations.
        """

        db = utils.exposed_decls_db_t()
        db.load( other_module_generated_code_dir )
        db.update_decls( self.global_ns )


    def __parse_declarations( self, files, gccxml_config, compilation_mode, cache, indexing_suite_version ):
        if None is gccxml_config:
            gccxml_config = parser.config_t()
        if None is compilation_mode:
            compilation_mode = parser.COMPILATION_MODE.FILE_BY_FILE
        start_time = time.clock()
        self.logger.debug( 'parsing files - started' )
        reader = parser.project_reader_t( gccxml_config, cache, decl_wrappers.dwfactory_t() )
        decls = reader.read_files( files, compilation_mode )

        self.logger.debug( 'parsing files - done( %f seconds )' % ( time.clock() - start_time ) )
        self.logger.debug( 'settings declarations defaults - started' )

        global_ns = decls_package.matcher.get_single(
                decls_package.namespace_matcher_t( name='::' )
                , decls )
        if indexing_suite_version != 1:
            for cls in global_ns.classes():
                cls.indexing_suite_version = indexing_suite_version
            for cls in global_ns.decls(decl_type=decls_package.class_declaration_t):
                cls.indexing_suite_version = indexing_suite_version

        start_time = time.clock()
        self.__apply_decls_defaults(decls)
        self.logger.debug( 'settings declarations defaults - done( %f seconds )'
                           % ( time.clock() - start_time ) )
        return global_ns

    def __filter_by_location( self, flatten_decls ):
        for decl in flatten_decls:
            if not decl.location:
                continue
            fpath = pygccxml_utils.normalize_path( decl.location.file_name )
            if pygccxml_utils.contains_parent_dir( fpath, self.__parsed_dirs ):
                continue
            if fpath in self.__parsed_files:
                continue
            found = False
            for pfile in self.__parsed_files:
                if fpath.endswith( pfile ):
                    found = True
                    break
            if not found:
                decl.exclude()

    def __apply_decls_defaults(self, decls):
        flatten_decls = decls_package.make_flatten( decls )
        self.__filter_by_location( flatten_decls )
        call_policies_resolver = creators_factory.built_in_resolver_t()
        calldefs = [decl for decl in flatten_decls if isinstance( decl, decls_package.calldef_t )]
        for calldef in calldefs:
            calldef.set_call_policies( call_policies_resolver( calldef ) )
        mem_vars = [decl for decl in flatten_decls if isinstance( decl, decls_package.variable_t )
                                        and isinstance( decl.parent, decls_package.class_t )]
        for mem_var in mem_vars:
            mem_var.set_getter_call_policies( call_policies_resolver( mem_var, 'get' ) )
        for mem_var in mem_vars:
            mem_var.set_setter_call_policies( call_policies_resolver( mem_var, 'set' ) )

    @property
    def declarations_code_head( self ):
        "A list of the user code, which will be added to the head of the declarations section."
        return self.__declarations_code_head

    @property
    def declarations_code_tail( self ):
        "A list of the user code, which will be added to the tail of the declarations section."
        return self.__declarations_code_tail

    @property
    def registrations_code_head( self ):
        "A list of the user code, which will be added to the head of the registrations section."
        return self.__registrations_code_head

    @property
    def registrations_code_tail( self ):
        "A list of the user code, which will be added to the tail of the registrations section."
        return self.__registrations_code_tail

    def build_code_creator( self
                       , module_name
                       , boost_python_ns_name='bp'
                       , call_policies_resolver_=None
                       , types_db=None
                       , target_configuration=None
                       , enable_indexing_suite=True
                       , doc_extractor=None):
        """
        Creates :class:`code_creators.bpmodule_t` code creator.

        :param module_name: module name
        :type module_name: str

        :param boost_python_ns_name: boost::python namespace alias, by default it is `bp`
        :type boost_python_ns_name: str

        :param call_policies_resolver_: callable, that will be invoked on every calldef object. It should return call policies.
        :type call_policies_resolver_: callable

        :param doc_extractor: callable, that takes as argument reference to declaration and returns documentation string
        :type doc_extractor: callable or None
        """

        creator = creators_factory.bpcreator_t( self.global_ns
                                                , module_name
                                                , boost_python_ns_name
                                                , call_policies_resolver_
                                                , types_db
                                                , target_configuration
                                                , enable_indexing_suite )
        self.__code_creator = creator.create()
        self.__code_creator.replace_included_headers(self.__parsed_files)
        self.__code_creator.update_documentation( doc_extractor )
        return self.__code_creator

    @property
    def code_creator( self ):
        "reference to :class:`code_creators.bpmodule_t` instance"
        if not self.__code_creator:
            raise RuntimeError( "self.module is equal to None. Did you forget to call build_code_creator function?" )
        return self.__code_creator

    def has_code_creator( self ):
        """
        Function, that will return True if build_code_creator function has been
        called and False otherwise
        """
        return not ( None is self.__code_creator )

    def add_declaration_code( self, code, tail=True ):
        """adds the user code to the generated one"""
        if tail:
            self.__declarations_code_tail.append( code )
        else:
            self.__declarations_code_head.append( code )

    def add_registration_code( self, code, tail=True ):
        """adds the user code to the generated one"""
        if tail:
            self.__registrations_code_tail.append( code )
        else:
            self.__registrations_code_head.append( code )

    def add_constants( self, **keywds ):
        """
        adds code that exposes some constants to Python.

        For example:
        .. code-block:: python

           mb.add_constants( version='"1.2.3"' )
           # or
           constants = dict( version:'"1.2.3"' )
           mb.add_constants( \\*\\*constants )

        will generate the following code:

        .. code-block:: c++

           boost::python::scope().attr("version") = "1.2.3";

        """
        tmpl = 'boost::python::scope().attr("%(name)s") = %(value)s;'
        for name, value in list(keywds.items()):
            if not isinstance( value, str ):
                value = str( value )
            self.add_registration_code( tmpl % dict( name=name, value=value) )


    def __merge_user_code( self ):
        for code in self.__declarations_code_tail:
            self.code_creator.add_declaration_code( code, -1 )

        for code in self.__declarations_code_head:
            self.code_creator.add_declaration_code( code, 0 )

        body = self.code_creator.body

        for code in self.__registrations_code_tail:
            body.adopt_creator( code_creators.custom_text_t( code ), -1 )

        for code in self.__registrations_code_head:
            body.adopt_creator( code_creators.custom_text_t( code ), 0 )


    def write_module( self, file_name ):
        """
        Writes module to a single file

        :param file_name: file name
        :type file_name: string

        """
        self.__merge_user_code()
        file_writers.write_file( self.code_creator, file_name, encoding=self.encoding )
        
    def get_module( self ):
        self.__merge_user_code()
        return self.code_creator.create()
        
    def merge_user_code(self):
        self.__merge_user_code()
        
    def __work_on_unused_files( self, dir_name, written_files, on_unused_file_found ):
        all_files = os.listdir( dir_name )
        all_files = [os.path.join( dir_name, fname ) for fname in all_files]
        all_files = list(filter( file_writers.has_pypp_extenstion, all_files ))

        unused_files = set( all_files ).difference( set( written_files ) )
        for fpath in unused_files:
            try:
                if on_unused_file_found is os.remove:
                    self.logger.info( 'removing file "%s"' % fpath )
                on_unused_file_found( fpath )
            except Exception as error:
                self.logger.exception( "Exception was catched, while executing 'on_unused_file_found' function."  )

    def split_module( self
                      , dir_name
                      , huge_classes=None
                      , on_unused_file_found=os.remove
                      , use_files_sum_repository=False):
        """
        writes module to multiple files

        :param dir_name: directory name
        :type dir_name: str

        :param huge_classes: list that contains reference to classes, that should be split

        :param on_unused_file_found: callable object that represents the action that should be taken on
                                     file, which is no more in use

        :param use_files_sum_repository: `Py++` can generate file, which will contain `md5` sum of every generated file.
                                          Next time you generate code, md5sum will be loaded from the file and compared.
                                          This could speed-up code generation process by 10-15%.
        """
        self.__merge_user_code()

        files_sum_repository = None
        if use_files_sum_repository:
            cache_file = os.path.join( dir_name, self.code_creator.body.name + '.md5.sum' )
            files_sum_repository = file_writers.cached_repository_t( cache_file )

        written_files = []
        if None is huge_classes:
            written_files = file_writers.write_multiple_files(
                                self.code_creator
                                , dir_name
                                , files_sum_repository=files_sum_repository
                                , encoding=self.encoding)
        else:
            written_files = file_writers.write_class_multiple_files(
                                self.code_creator
                                , dir_name
                                , huge_classes
                                , files_sum_repository=files_sum_repository
                                , encoding=self.encoding)
        self.__work_on_unused_files( dir_name, written_files, on_unused_file_found )

        return written_files

    def balanced_split_module( self
                               , dir_name
                               , number_of_files
                               , on_unused_file_found=os.remove
                               , use_files_sum_repository=False):
        """
        Writes module to fixed number of multiple cpp files

        :param number_of_files: the desired number of generated cpp files
        :type number_of_files: int

        :param dir_name: directory name
        :type dir_name: string

        :param on_unused_file_found: callable object that represents the action that should be taken on
                                     file, which is no more in use

        :param use_files_sum_repository: `Py++` can generate file, which will contain md5 sum of every generated file.
                                          Next time you generate code, md5sum will be loaded from the file and compared.
                                          This could speed-up code generation process by 10-15%.
        """
        self.__merge_user_code()

        files_sum_repository = None
        if use_files_sum_repository:
            cache_file = os.path.join( dir_name, self.code_creator.body.name + '.md5.sum' )
            files_sum_repository = file_writers.cached_repository_t( cache_file )

        written_files = file_writers.write_balanced_files( self.code_creator
                                                           , dir_name
                                                           , number_of_buckets=number_of_files
                                                           , files_sum_repository=files_sum_repository
                                                           , encoding=self.encoding)

        self.__work_on_unused_files( dir_name, written_files, on_unused_file_found )

        return written_files

    def _get_BOOST_PYTHON_MAX_ARITY( self ):
        return decl_wrappers.calldef_t.BOOST_PYTHON_MAX_ARITY
    def _set_BOOST_PYTHON_MAX_ARITY( self, value ):
        decl_wrappers.calldef_t.BOOST_PYTHON_MAX_ARITY = value
    BOOST_PYTHON_MAX_ARITY = property( _get_BOOST_PYTHON_MAX_ARITY, _set_BOOST_PYTHON_MAX_ARITY )
