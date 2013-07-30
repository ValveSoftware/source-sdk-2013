# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines a class that writes :class:`code_creators.bpmodule_t` to multiple files"""

import os
from . import writer
from pyplusplus import messages
from pyplusplus import _logging_
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators

#TODO: to add namespace_alias_t classes
class multiple_files_t(writer.writer_t):
    """
    This class implements classic strategy of dividing classes to files
    one class in one header + source files.
    """
    HEADER_EXT = '_pypp.hpp'
    SOURCE_EXT = '_pypp.cpp'

    def __init__(self, extmodule, directory_path, write_main=True, files_sum_repository=None, encoding='ascii'):
        """
        :param extmodule: code creators tree root
        :type extmodule: :class:`code_creators.bpmodule_t`
        :param directory_path: generated source files directory
        :type directory_path: str
        :param write_main:  if True, the class will write out a main file that calls all the registration methods.
        :type write_main: boolean
        """
        writer.writer_t.__init__( self, extmodule, files_sum_repository, encoding=encoding )
        self.__directory_path = directory_path
        self.create_dir( directory_path )
        self.include_creators = []  # List of include_t creators that contain the generated headers
        self.split_header_names = []  # List of include file names for split files
        self.split_method_names = []  # List of methods from the split files
        self.write_main = write_main
        self.written_files = []
        self.ref_count_creators = ( code_creators.opaque_type_registrator_t, )
        self.__predefined_include_creators \
            = [creator for creator in self.extmodule.creators if isinstance( creator, code_creators.include_t )]
        self.__value_traits = [x for x in self.extmodule.creators if isinstance(x, code_creators.value_traits_t)]


    def write_file( self, fpath, content ):
        if fpath in self.written_files:
            msg = ['`Py++` is going to write different content to the same file(%s).' % fpath]
            msg.append('The following is a short list of possible explanations for this behaviour:' )
            msg.append('* `Py++` bug, in this case, please report it' )
            msg.append('* module_builder_t contains two or more classes with the same alias')
            msg.append('* module_builder_t contains two or more classes with the same wrapper alias')
            msg.append('Please carefully review `Py++` warning messages. It should contain an additional information.')
            raise RuntimeError( os.linesep.join(msg) )

        self.written_files.append( fpath )
        writer.writer_t.write_file( fpath, content, self.files_sum_repository, self.encoding )

    def create_dir( self, directory_path ):
        """Create the output directory if it doesn't already exist.
        """
        if os.path.exists( directory_path ) and not os.path.isdir(directory_path):
            raise RuntimeError( 'directory_path should contain path to directory.' )
        if not os.path.exists( directory_path ):
            os.makedirs( directory_path )

    def _get_directory_path(self):
        return self.__directory_path
    directory_path = property( _get_directory_path,
                               doc="""The name of the output directory.
                               @type: str
                               """ )

    def get_unique_creators( self, creators ):
        unique_creators = []
        unique_creator_ids = set()
        for creator in creators:
            if not id( creator ) in unique_creator_ids:
                unique_creator_ids.add( id( creator ) )
                unique_creators.append( creator )
        return unique_creators

    def associated_decl_creators( self, creator ):
        """ references to all class declaration code creators. """
        if not isinstance( creator, code_creators.registration_based_t ):
            return []

        associated_creators = creator.associated_decl_creators[:]

        internal_creators = []
        if isinstance( creator, code_creators.compound_t ):
            internal_creators.extend(
                [creator for creator in code_creators.make_flatten( creator.creators ) if isinstance( creator, code_creators.registration_based_t )] )

        for internal_creator in internal_creators:
            associated_creators.extend( internal_creator.associated_decl_creators )
        #now associated_creators contains all code creators associated with the creator
        #We should leave only creators, defined in the global namespace
        associated_creators = [associated_creator for associated_creator in associated_creators if associated_creator.parent is self.extmodule]
        return associated_creators

    def create_function_code( self, function_name ):
        return "void %s();" % function_name

    def create_header( self, file_name, code ):
        """
        return the content of a header file.

        :param file_name: A string that uniquely identifies the file name
        :type file_name: str
        :rtype: str
        """
        tmpl = os.linesep.join([
                    "#ifndef %(file_name)s_hpp__pyplusplus_wrapper"
                  , "#define %(file_name)s_hpp__pyplusplus_wrapper"
                  , ''
                  , "%(code)s"
                  , ''
                  , "#endif//%(file_name)s_hpp__pyplusplus_wrapper" ])

        content = ''
        if self.extmodule.license:
            content = self.extmodule.license.create() + os.linesep
        content = content + tmpl % { 'file_name' : file_name, 'code' : code }
        return content

    def find_out_value_traits_header( self, code_creator ):
        if not isinstance( code_creator, ( code_creators.class_t, code_creators.class_declaration_t ) ):
            return None
        if None is code_creator.declaration.indexing_suite:
            return None
        if not isinstance( code_creator.declaration.indexing_suite, decl_wrappers.indexing_suite2_t ):
            return None

        #sometimes, for some reason I expose containers as regular classes ( hash_map )
        #and in this case I do generate include to
        classes = ( code_creators.indexing_suite1_t, code_creators.indexing_suite2_t )
        for cont_code_creator in code_creator.creators:
            if isinstance( cont_code_creator, classes ):
                break
        else:
            return None

        try:
            element_type = code_creator.declaration.indexing_suite.element_type
            class_traits = declarations.class_traits
            if not class_traits.is_my_case( element_type ):
                return None
            value_class = class_traits.get_declaration( element_type )
            if value_class.less_than_comparable and value_class.equality_comparable:
                return None #`Py++` doesn't create value traits for class that has
                            # = and < operators available
            return self.create_value_traits_header_name( value_class )
        except RuntimeError as error:
            decls_logger = _logging_.loggers.declarations
            if not messages.filter_disabled_msgs([messages.W1042], code_creator.declaration.disabled_messages ):
                return #user disabled property warning
            decls_logger.warn( "%s;%s" % ( code_creator.declaration, messages.W1042 ) )

    def create_include_code( self, creators, head_headers=None, tail_headers=None ):
        answer = []
        normalize = code_creators.include_directories_t.normalize
        unique_headers = code_creators.code_creator_t.unique_headers

        if head_headers:
            answer.extend( ['#include "%s"' % normalize( header ) for header in head_headers] )

        dependend_on_headers = []
        for creator in creators:
            dependend_on_headers.extend( creator.get_system_files( recursive=True, language='C++' ) )

        dependend_on_headers = unique_headers( list(map( normalize, dependend_on_headers )) )

        for include_cc in self.__predefined_include_creators:
            if include_cc.is_system:
                if include_cc.header in dependend_on_headers:
                    answer.append( include_cc.create() )
            else:# user header file - always include
                answer.append( include_cc.create() )

        for used_header in self.get_user_headers( creators ):
            answer.append( '#include "%s"' % user_header )

        for creator in creators:
            header = self.find_out_value_traits_header( creator )
            if header:
                answer.append( '#include "%s"' % header )

        if tail_headers:
            answer.extend( ['#include "%s"' % normalize( header ) for header in tail_headers] )

        return os.linesep.join( answer )

    def create_namespaces_code( self, creators ):
        # Write all 'global' namespace_alias_t and namespace_using_t creators first...
        ns_types = ( code_creators.namespace_alias_t, code_creators.namespace_using_t )
        ns_creators = [x for x in self.extmodule.creators if isinstance( x, ns_types )]

        ns_creators.extend( [x for x in self.extmodule.body.creators if isinstance( x, ns_types )] )
        if not ns_creators:
            return ''
        else:
            return os.linesep.join( [creator.create() for creator in ns_creators] )

    def create_source( self, file_name, function_name, registration_creators ):
        """
        return the content of a cpp file.

        :param file_name: The base name of the corresponding include file (without extension)
        :type file_name: str

        :param function_name: "register" function name
        :type function_name: str

        :param creators: "register" function code creators
        :type creators: list of :class:`code_creators.code_creator_t`
        :rtype: str
        """
        declaration_creators = []
        for rc in registration_creators:
            declaration_creators.extend( self.associated_decl_creators( rc ) )
        declaration_creators = self.get_unique_creators( declaration_creators )

        creators = registration_creators + declaration_creators

        answer = []
        if self.extmodule.license:
            answer.append( self.extmodule.license.create() )

        head_headers = [ file_name + self.HEADER_EXT ]
        answer.append( self.create_include_code( creators, tail_headers=head_headers ) )

        answer.append( '' )
        answer.append( self.create_namespaces_code( creators ) )

        # Write wrapper classes...
        for creator in declaration_creators:
            answer.append( '' )
            answer.append( creator.create() )
            if not isinstance( creator, self.ref_count_creators ):
                creator.create = lambda: ''

        # Write the register() function...
        answer.append( '' )
        answer.append( 'void %s(){' % function_name )
        answer.append( '' )
        for creator in registration_creators:
            answer.append( code_creators.code_creator_t.indent( creator.create() ) )
            answer.append( '' )
        answer.append( '}' )
        return os.linesep.join( answer )

    def split_class_impl( self, class_creator):
        function_name = 'register_%s_class' % class_creator.alias
        file_path = os.path.join( self.directory_path, class_creator.alias )
        # Write the .h file...
        header_name = file_path + self.HEADER_EXT
        self.write_file( header_name
                         , self.create_header( class_creator.alias
                                               , self.create_function_code( function_name ) ) )

        # Write the .cpp file...
        cpp_code = self.create_source( class_creator.alias, function_name, [class_creator] )

        self.write_file( file_path + self.SOURCE_EXT, cpp_code )

        # Replace the create() method so that only the register() method is called
        # (this is called later for the main source file).
        class_creator.create = lambda: function_name +'();'
        self.include_creators.append( code_creators.include_t( header_name ) )
        self.split_header_names.append(header_name)
        self.split_method_names.append(function_name)

    def split_class( self, class_creator):
        """Write the .h/.cpp file for one class.

        Writes a .h/.cpp file for the given class. The files use the class name
        as base file name.

        :param class_creator: The class creator for one particular class
        :type class_creator: class_t
        """
        try:
            if class_creator.declaration.already_exposed:
                return
            self.split_class_impl( class_creator )
        except IOError as error:
            msg = [ 'Failed to write code for class "%s" into file.;' % class_creator.declaration.name ]
            msg.append( "May be the class name is too long?." )
            msg.append( "Error: %s'" % str(error) )
            self.logger.error( os.linesep.join( msg ) )
            raise

    def split_classes( self ):
        # Obtain a list of all class creators...
        class_creators = [x for x in self.extmodule.body.creators if isinstance(x, ( code_creators.class_t, code_creators.class_declaration_t ) )]
        # ...and write a .h/.cpp file for each class
        for cls in class_creators: self.split_class(cls)

    def create_value_traits_header_name( self, value_class ):
        return "_" + value_class.alias + "__value_traits" + self.HEADER_EXT

    def split_value_traits( self, value_traits ):
        """
        Write the value_traits class to header file, that will be included
        from files, that uses indexing suite 2
        """
        if value_traits.declaration.already_exposed:
            return

        header_name = self.create_value_traits_header_name( value_traits.declaration )
        file_path = os.path.join( self.directory_path, header_name )
        self.write_file( file_path
                        , self.create_header( header_name.replace( '.', '_' )
                                              , value_traits.create() ) )
        value_traits.create = lambda: ''

    def split_values_traits( self ):
        for trait in self.__value_traits: self.split_value_traits(trait)

    def split_creators( self, creators, pattern, function_name, registrator_pos ):
        """Write non-class creators into a particular .h/.cpp file.

        :param creators: The code creators that should be written
        :type creators: list of :class:`code_creators.code_creator_t`

        :param pattern: Name pattern that is used for constructing the final output file name
        :type pattern: str

        :param function_name: "register" function name
        :type function_name: str

        :param registrator_pos: The position of the code creator that creates the code to invoke the "register" function.
        :type registrator_pos: int
        """
        if not creators:
            return
        file_pattern = self.extmodule.body.name + pattern
        file_path = os.path.join( self.directory_path, file_pattern )
        header_name = file_path + self.HEADER_EXT
        self.write_file( header_name
                         , self.create_header( file_pattern, self.create_function_code( function_name ) ) )
        self.write_file( file_path + self.SOURCE_EXT
                         , self.create_source( file_pattern, function_name, creators ))

        for creator in creators:
            creator.create = lambda: ''
        self.extmodule.body.adopt_creator(
            code_creators.custom_text_t( function_name + '();' )
            , registrator_pos)
        self.include_creators.append( code_creators.include_t( header_name ) )
        self.split_header_names.append(header_name)
        self.split_method_names.append(function_name)

    def split_enums( self ):
        """Write all enumerations into a separate .h/.cpp file.
        """
        enums_creators = [x for x in self.extmodule.body.creators if isinstance(x, code_creators.enum_t )]

        self.split_creators( enums_creators, '_enumerations', 'register_enumerations', 0 )

    def split_global_variables( self ):
        """Write all global variables into a separate .h/.cpp file.
        """
        creators = [x for x in self.extmodule.body.creators if isinstance(x, code_creators.global_variable_t )]
        creators.extend( [x for x in self.extmodule.body.creators if isinstance(x, code_creators.unnamed_enum_t )] )
        self.split_creators( creators, '_global_variables', 'register_global_variables', -1 )

    def split_free_functions( self ):
        """Write all free functions into a separate .h/.cpp file.
        """
        free_functions = ( code_creators.free_function_t, code_creators.free_fun_overloads_t )
        creators = [x for x in self.extmodule.body.creators if isinstance(x, free_functions )]
        self.split_creators( creators, '_free_functions', 'register_free_functions', -1 )

    def write(self):
        """
        writes out the module.

        Creates a separate source/header combo for each class and for enumerations,
        globals and free functions.
        If :meth:`write_main` is True it writes out a main file that calls all the registration methods.
        After this call :meth:`split_header_names` and :meth:`split_method_names`
        will contain all the header files and registration methods used.  This
        can be used by user code to create custom registration methods if main is not written.
        """

        self.write_code_repository( self.__directory_path )
        self.save_exposed_decls_db( self.__directory_path )

        self.extmodule.do_include_dirs_optimization()

        self.split_values_traits()
        self.split_classes()
        self.split_enums()
        self.split_global_variables()
        self.split_free_functions()

        if self.write_main:
            self.include_creators.sort( key=lambda ic: ic.header )
            for creator in self.include_creators:
                self.extmodule.adopt_include( creator )
            main_cpp = os.path.join( self.directory_path, self.extmodule.body.name + '_main.cpp' )
            self.write_file( main_cpp, self.extmodule.create() + os.linesep )
        self.files_sum_repository.save_values()
