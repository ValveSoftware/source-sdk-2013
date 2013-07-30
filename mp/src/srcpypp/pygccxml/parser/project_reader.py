# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import time
import types
from . import source_reader
from . import declarations_cache
import pygccxml.declarations
from pygccxml import utils

class COMPILATION_MODE:
    ALL_AT_ONCE = 'all at once'
    FILE_BY_FILE = 'file by file'


class file_configuration_t( object ):
    """
    source code location configuration.

    The class instance uses "variant" interface to represent the following data:

    1) path to a C++ source file

    2) path to GCC-XML generated XML file

    3) path to a C++ source file and path to GCC-XML generated file

        In this case, if XML file does not exists, it will be created. Next time
        you will ask to parse the source file, the XML file will be used instead.

        Small tip: you can setup your makefile to delete XML files every time,
        the relevant source file was changed.

    4) Python string, that contains valid C++ code


    There are few functions, that will help you to construct :class:`file_configuration_t`
    object:

    * :func:`create_source_fc`

    * :func:`create_gccxml_fc`

    * :func:`create_cached_source_fc`

    * :func:`create_text_fc`

    """
    class CONTENT_TYPE:
        STANDARD_SOURCE_FILE = 'standard source file'
        CACHED_SOURCE_FILE = 'cached source file'
        GCCXML_GENERATED_FILE = 'gccxml generated file'
        TEXT = 'text'

    def __init__( self
                  , data
                  , start_with_declarations=None
                  , content_type=CONTENT_TYPE.STANDARD_SOURCE_FILE
                  , cached_source_file=None ):
        object.__init__( self )
        self.__data = data
        if not start_with_declarations:
            start_with_declarations = []
        self.__start_with_declarations = start_with_declarations
        self.__content_type = content_type
        self.__cached_source_file = cached_source_file
        if not self.__cached_source_file \
           and self.__content_type == self.CONTENT_TYPE.CACHED_SOURCE_FILE:
            self.__cached_source_file = self.__data + '.xml'

    @property
    def data(self):
        return self.__data

    @property
    def start_with_declarations(self):
        return self.__start_with_declarations

    @property
    def content_type(self):
        return self.__content_type

    @property
    def cached_source_file(self):
        return self.__cached_source_file

def create_text_fc( text ):
    """
    Creates :class:`parser.file_configuration_t` instance, configured to contain Python string,
    that contains valid C++ code

    :param text: C++ code
    :type text: str

    :rtype: :class:`parser.file_configuration_t`
    """
    return file_configuration_t( data=text
                                 , content_type=file_configuration_t.CONTENT_TYPE.TEXT )

def create_source_fc( header ):
    """
    Creates :class:`parser.file_configuration_t` instance, configured to contain path to
    C++ source file

    :param header: path to C++ source file
    :type header: str

    :rtype: :class:`parser.file_configuration_t`
    """
    return file_configuration_t( data=header
                                 , content_type=file_configuration_t.CONTENT_TYPE.STANDARD_SOURCE_FILE )

def create_gccxml_fc( xml_file ):
    """
    Creates :class:`parser.file_configuration_t` instance, configured to contain path to
    GCC-XML generated XML file.

    :param xml_file: path to GCC-XML generated XML file
    :type xml_file: str

    :rtype: :class:`parser.file_configuration_t`
    """
    return file_configuration_t( data=xml_file
                                 , content_type=file_configuration_t.CONTENT_TYPE.GCCXML_GENERATED_FILE )

def create_cached_source_fc( header, cached_source_file ):
    """
    Creates :class:`parser.file_configuration_t` instance, configured to contain path to
    GCC-XML generated XML file and C++ source file. If XML file does not exists,
    it will be created and used for parsing. If XML file exists, it will be used
    for parsing.

    :param header: path to C++ source file
    :type header: str

    :param cached_source_file: path to GCC-XML generated XML file
    :type cached_source_file: str

    :rtype: :class:`parser.file_configuration_t`
    """
    return file_configuration_t( data=header
                                 , cached_source_file=cached_source_file
                                 , content_type=file_configuration_t.CONTENT_TYPE.CACHED_SOURCE_FILE )

class project_reader_t:
    """parses header files and returns the contained declarations"""

    def __init__( self, config, cache=None, decl_factory=None):
        """
        :param config: GCCXML configuration
        :type config: :class:config_t

        :param cache: declaration cache, by default a cache functionality will not be used
        :type cache: :class:`cache_base_t` instance or `str`

        :param decl_factory: declaration factory
        :type decl_factory: :class:`decl_factory_t`
        """
        self.__config = config
        self.__dcache = None
        if isinstance( cache, declarations_cache.cache_base_t ):
            self.__dcache = cache
        elif isinstance( cache, str ):
            self.__dcache = declarations_cache.file_cache_t(cache)
        else:
            self.__dcache = declarations_cache.dummy_cache_t()
        self.__decl_factory = decl_factory
        if not decl_factory:
            self.__decl_factory = pygccxml.declarations.decl_factory_t()

        self.logger = utils.loggers.gccxml

    @staticmethod
    def get_os_file_names( files ):
        """
        returns file names

        :param files: list of strings and\\or :class:`file_configuration_t` instances.
        :type files: list
        """
        fnames = []
        for f in files:
            if isinstance( f, str ):
                fnames.append( f )
            elif isinstance( f, file_configuration_t ):
                if f.content_type in ( file_configuration_t.CONTENT_TYPE.STANDARD_SOURCE_FILE
                                       , file_configuration_t.CONTENT_TYPE.CACHED_SOURCE_FILE ):
                    fnames.append( f.data )
            else:
                pass
        return fnames

    def read_files( self, files, compilation_mode=COMPILATION_MODE.FILE_BY_FILE):
        """
        parses a set of files

        :param files: list of strings and\\or :class:`file_configuration_t` instances.
        :type files: list

        :param compilation_mode: determines whether the files are parsed individually or as one single chunk
        :type compilation_mode: :class:`COMPILATION_MODE`
        :rtype: [:class:`declaration_t`]
        """
        if compilation_mode == COMPILATION_MODE.ALL_AT_ONCE \
           and len( files ) == len( self.get_os_file_names(files) ):
            return self.__parse_all_at_once(files)
        else:
            if compilation_mode == COMPILATION_MODE.ALL_AT_ONCE:
                msg = ''.join([
                    "Unable to parse files using ALL_AT_ONCE mode. "
                    , "There is some file configuration that is not file. "
                    , "pygccxml.parser.project_reader_t switches to FILE_BY_FILE mode." ])
                self.logger.warning( msg )
            return self.__parse_file_by_file(files)

    def __parse_file_by_file(self, files):
        namespaces = []
        config = self.__config.clone()
        self.logger.debug( "Reading project files: file by file" )
        for prj_file in files:
            reader = None
            header = None
            content_type = None
            if isinstance( prj_file, file_configuration_t ):
                del config.start_with_declarations[:]
                config.start_with_declarations.extend( prj_file.start_with_declarations )
                header = prj_file.data
                content_type = prj_file.content_type
            else:
                config = self.__config
                header = prj_file
                content_type = file_configuration_t.CONTENT_TYPE.STANDARD_SOURCE_FILE
            reader = source_reader.source_reader_t( config
                                                    , self.__dcache
                                                    , self.__decl_factory )
            decls = None
            if content_type == file_configuration_t.CONTENT_TYPE.STANDARD_SOURCE_FILE:
                self.logger.info( 'Parsing source file "%s" ... ' % header )
                decls = reader.read_file( header )
            elif content_type == file_configuration_t.CONTENT_TYPE.GCCXML_GENERATED_FILE:
                self.logger.info( 'Parsing xml file "%s" ... ' % header )
                decls = reader.read_xml_file( header )
            elif content_type == file_configuration_t.CONTENT_TYPE.CACHED_SOURCE_FILE:
                #TODO: raise error when header file does not exist
                if not os.path.exists( prj_file.cached_source_file ):
                    dir_ = os.path.split( prj_file.cached_source_file )[0]
                    if dir_ and not os.path.exists( dir_ ):
                        os.makedirs( dir_ )
                    self.logger.info( 'Creating xml file "%s" from source file "%s" ... '
                                      % ( prj_file.cached_source_file, header ) )
                    reader.create_xml_file( header, prj_file.cached_source_file )
                self.logger.info( 'Parsing xml file "%s" ... ' % prj_file.cached_source_file )
                decls = reader.read_xml_file( prj_file.cached_source_file )
            else:
                decls = reader.read_string( header )
            namespaces.append( decls )
        self.logger.debug( "Flushing cache... " )
        start_time = time.clock()
        self.__dcache.flush()
        self.logger.debug( "Cache has been flushed in %.1f secs" % ( time.clock() - start_time ) )
        answer = []
        self.logger.debug( "Joining namespaces ..." )
        for file_nss in namespaces:
            answer = self._join_top_namespaces( answer, file_nss )
        self.logger.debug( "Joining declarations ..." )
        for ns in answer:
            if isinstance( ns, pygccxml.declarations.namespace_t ):
                self._join_declarations( ns )
        leaved_classes = self._join_class_hierarchy( answer )
        types = self.__declarated_types(answer)
        self.logger.debug( "Relinking declared types ..." )
        self._relink_declarated_types( leaved_classes, types )
        source_reader.bind_aliases( pygccxml.declarations.make_flatten( answer ) )
        return answer

    def __parse_all_at_once(self, files):
        config = self.__config.clone()
        self.logger.debug( "Reading project files: all at once" )
        header_content = []
        for header in files:
            if isinstance( header, file_configuration_t ):
                del config.start_with_declarations[:]
                config.start_with_declarations.extend( header.start_with_declarations )
                header_content.append( '#include "%s" %s' % ( header.data, os.linesep ) )
            else:
                header_content.append( '#include "%s" %s' % ( header, os.linesep ) )
        return self.read_string( ''.join( header_content ) )

    def read_string(self, content):
        """Parse a string containing C/C++ source code.

        :param content: C/C++ source code.
        :type content: str
        :rtype: Declarations
        """
        reader = source_reader.source_reader_t( self.__config, None, self.__decl_factory )
        return reader.read_string( content )

    def read_xml( self, file_configuration ):
        """parses C++ code, defined on the file_configurations and returns GCCXML
        generated file content"""

        xml_file_path = None
        delete_xml_file = True
        fc = file_configuration
        reader = source_reader.source_reader_t( self.__config, None, self.__decl_factory )
        try:
            if fc.content_type == fc.CONTENT_TYPE.STANDARD_SOURCE_FILE:
                self.logger.info( 'Parsing source file "%s" ... ' % fc.data )
                xml_file_path = reader.create_xml_file( fc.data )
            elif fc.content_type == file_configuration_t.CONTENT_TYPE.GCCXML_GENERATED_FILE:
                self.logger.info( 'Parsing xml file "%s" ... ' % fc.data )
                xml_file_path = fc.data
                delete_xml_file = False
            elif fc.content_type == fc.CONTENT_TYPE.CACHED_SOURCE_FILE:
                #TODO: raise error when header file does not exist
                if not os.path.exists( fc.cached_source_file ):
                    dir_ = os.path.split( fc.cached_source_file )[0]
                    if dir_ and not os.path.exists( dir_ ):
                        os.makedirs( dir_ )
                    self.logger.info( 'Creating xml file "%s" from source file "%s" ... '
                                      % ( fc.cached_source_file, fc.data ) )
                    xml_file_path = reader.create_xml_file( fc.data, fc.cached_source_file )
                else:
                    xml_file_path = fc.cached_source_file
            else:
                xml_file_path = reader.create_xml_file_from_string( fc.data )
            xml_file = open( xml_file_path, 'r' )
            xml = xml_file.read()
            xml_file.close()
            utils.remove_file_no_raise( xml_file_path )
            return xml
        finally:
            if xml_file_path and delete_xml_file:
                utils.remove_file_no_raise( xml_file_path )

    def _join_top_namespaces(self, main_ns_list, other_ns_list):
        answer = main_ns_list[:]
        for other_ns in other_ns_list:
            main_ns = pygccxml.declarations.find_declaration( answer
                                                              , type=pygccxml.declarations.namespace_t
                                                              , name=other_ns._name
                                                              , recursive=False )
            if main_ns:
                main_ns.take_parenting( other_ns )
            else:
                answer.append( other_ns )
        return answer

    def _join_namespaces( self, nsref ):
        assert isinstance( nsref, pygccxml.declarations.namespace_t )
        ddhash = {} # decl.__class__ :  { decl.name : [decls] } double declaration hash
        decls = []

        for decl in nsref.declarations:
            if decl.__class__ not in ddhash:
                ddhash[ decl.__class__ ] = { decl._name : [ decl ] }
                decls.append( decl )
            else:
                joined_decls = ddhash[ decl.__class__ ]
                if decl._name not in joined_decls:
                    decls.append( decl )
                    joined_decls[decl._name] = [ decl ]
                else:
                    if isinstance( decl, pygccxml.declarations.calldef_t ):
                        if decl not in joined_decls[decl._name]:
                            #functions has overloading
                            decls.append( decl )
                            joined_decls[decl._name].append( decl )
                    elif isinstance( decl, pygccxml.declarations.enumeration_t ):
                        #unnamed enums
                        if not decl.name and decl not in joined_decls[decl._name]:
                            decls.append( decl )
                            joined_decls[decl._name].append( decl )
                    elif isinstance( decl, pygccxml.declarations.class_t ):
                        #unnamed classes
                        if not decl.name and decl not in joined_decls[decl._name]:
                            decls.append( decl )
                            joined_decls[decl._name].append( decl )
                    else:
                        assert 1 == len( joined_decls[ decl._name ] )
                        if isinstance( decl, pygccxml.declarations.namespace_t ):
                            joined_decls[ decl._name ][0].take_parenting( decl )

        class_t = pygccxml.declarations.class_t
        class_declaration_t = pygccxml.declarations.class_declaration_t
        if class_t in ddhash and class_declaration_t in ddhash:
            #if there is a class and its forward declaration - get rid of the
            #second one.
            class_names = set()
            for name, same_name_classes in ddhash[ class_t ].items():
                if not name:
                    continue
                class_names.add( same_name_classes[0].mangled )

            class_declarations = ddhash[ class_declaration_t ]
            for name, same_name_class_declarations in class_declarations.items():
                if not name:
                    continue
                for class_declaration in same_name_class_declarations :
                    if class_declaration.mangled and class_declaration.mangled in class_names:
                        decls.remove( class_declaration )

        nsref.declarations = decls

    def _join_class_hierarchy( self, namespaces ):
        create_key = lambda decl:( decl.location.as_tuple()
                                   , tuple( pygccxml.declarations.declaration_path( decl ) ) )
        classes = [decl for decl in pygccxml.declarations.make_flatten( namespaces ) if isinstance(decl, pygccxml.declarations.class_t )]
        leaved_classes = {}
        #selecting classes to leave
        for class_ in classes:
            key = create_key( class_ )
            if key not in leaved_classes:
                leaved_classes[ key ] = class_
        #replacing base and derived classes with those that should be leave
        #also this loop will add missing derived classes to the base
        for class_ in classes:
            leaved_class = leaved_classes[create_key( class_ )]
            for base_info in class_.bases:
                leaved_base = leaved_classes[ create_key( base_info.related_class ) ]
                #treating base class hierarchy of leaved_class
                leaved_base_info = pygccxml.declarations.hierarchy_info_t(
                    related_class=leaved_base
                    , access=base_info.access )
                if leaved_base_info not in leaved_class.bases:
                    leaved_class.bases.append( leaved_base_info )
                else:
                    index = leaved_class.bases.index( leaved_base_info )
                    leaved_class.bases[index].related_class = leaved_base_info.related_class
                #treating derived class hierarchy of leaved_base
                leaved_derived_for_base_info = pygccxml.declarations.hierarchy_info_t(
                    related_class=leaved_class
                    , access=base_info.access )
                if leaved_derived_for_base_info not in leaved_base.derived:
                    leaved_base.derived.append( leaved_derived_for_base_info )
                else:
                    index = leaved_base.derived.index( leaved_derived_for_base_info )
                    leaved_base.derived[index].related_class = leaved_derived_for_base_info.related_class
            for derived_info in class_.derived:
                leaved_derived = leaved_classes[ create_key( derived_info.related_class ) ]
                #treating derived class hierarchy of leaved_class
                leaved_derived_info = pygccxml.declarations.hierarchy_info_t(
                    related_class=leaved_derived
                    , access=derived_info.access )
                if leaved_derived_info not in leaved_class.derived:
                    leaved_class.derived.append( leaved_derived_info )
                #treating base class hierarchy of leaved_derived
                leaved_base_for_derived_info = pygccxml.declarations.hierarchy_info_t(
                    related_class=leaved_class
                    , access=derived_info.access )
                if leaved_base_for_derived_info not in leaved_derived.bases:
                    leaved_derived.bases.append( leaved_base_for_derived_info )
        #this loops remove instance we from parent.declarations
        for class_ in classes:
            key = create_key( class_ )
            if id( leaved_classes[key] ) == id( class_ ):
                continue
            else:
                declarations = None
                if class_.parent:
                    declarations = class_.parent.declarations
                else:
                    declarations = namespaces #yes, we are talking about global class that doesn't
                    #belong to any namespace. Usually is compiler generated top level classes
                declarations_ids = [ id(decl) for decl in declarations ]
                del declarations[ declarations_ids.index( id(class_) ) ]
        return leaved_classes

    def _relink_declarated_types(self, leaved_classes, declarated_types):
        create_key = lambda decl:( decl.location.as_tuple()
                                   , tuple( pygccxml.declarations.declaration_path( decl ) ) )
        create_mangled_key = lambda decl:( decl.location.as_tuple(), decl.mangled )

        mangled_leaved_classes = {}
        for cls in leaved_classes.values():
            mangled_leaved_classes[ create_mangled_key( cls ) ] = cls

        for decl_wrapper_type in declarated_types:
            #it is possible, that cache contains reference to dropped class
            #We need to clear it
            decl_wrapper_type.cache.reset()
            if isinstance( decl_wrapper_type.declaration, pygccxml.declarations.class_t ):
                key = create_key(decl_wrapper_type.declaration)
                if key in leaved_classes:
                    decl_wrapper_type.declaration = leaved_classes[ key ]
                else:
                    if decl_wrapper_type.declaration._name.startswith( '__vmi_class_type_info_pseudo' ):
                        continue
                    msg = []
                    msg.append( "Unable to find out actual class definition: '%s'." % decl_wrapper_type.declaration._name )
                    msg.append( "Class definition has been changed from one compilation to an other." )
                    msg.append( "Why did it happen to me? Here is a short list of reasons: " )
                    msg.append( "    1. There are different preprocessor definitions applied on same file during compilation" )
                    msg.append( "    2. Bug in pygccxml." )
                    self.logger.error( os.linesep.join(msg) )
            elif isinstance( decl_wrapper_type.declaration, pygccxml.declarations.class_declaration_t ):
                key = create_mangled_key(decl_wrapper_type.declaration)
                if key in mangled_leaved_classes:
                    decl_wrapper_type.declaration = mangled_leaved_classes[ key ]

    def _join_declarations( self, declref ):
        self._join_namespaces( declref )
        for ns in declref.declarations:
            if isinstance( ns, pygccxml.declarations.namespace_t ):
                self._join_declarations( ns )

    def __declarated_types(self, namespaces):
        def get_from_type(cpptype):
            if not cpptype:
                return []
            elif isinstance( cpptype, pygccxml.declarations.fundamental_t ):
                return []
            elif isinstance( cpptype, pygccxml.declarations.declarated_t ):
                return [ cpptype ]
            elif isinstance( cpptype, pygccxml.declarations.compound_t ):
                return get_from_type( cpptype.base )
            elif isinstance( cpptype, pygccxml.declarations.calldef_type_t ):
                types = get_from_type( cpptype.return_type )
                for arg in cpptype.arguments_types:
                    types.extend( get_from_type( arg ) )
                return types
            else:
                assert isinstance( cpptype, ( pygccxml.declarations.unknown_t
                                              , pygccxml.declarations.ellipsis_t ) )
                return []
        types = []
        for decl in pygccxml.declarations.make_flatten( namespaces ):
            if isinstance( decl, pygccxml.declarations.calldef_t ):
                types.extend( get_from_type( decl.function_type() ) )
            elif isinstance( decl, (pygccxml.declarations.typedef_t, pygccxml.declarations.variable_t) ):
                types.extend( get_from_type( decl.type ) )
        return types
