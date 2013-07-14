# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import custom
from . import license
from . import include
from . import compound
from . import namespace
from . import algorithm
from . import module_body
from . import library_reference
from . import declaration_based
from . import include_directories
from pygccxml import utils



class module_t(compound.compound_t):
    """This class represents the source code for the entire extension module.

    The root of the code creator tree is always a module_t object.
    """
    def __init__(self, global_ns, code_generator_type):
        """Constructor.
        """
        compound.compound_t.__init__(self)
        self.__global_ns = global_ns
        self._code_generator = code_generator_type

    @property
    def global_ns(self):
        "reference to global_ns ( namespace_t ) declaration"
        return self.__global_ns

    def _get_license( self ):
        if isinstance( self.creators[0], license.license_t ):
            return self.creators[0]
        return None

    def _set_license( self, license_text ):
        if not isinstance( license_text, license.license_t ):
            license_inst = license.license_t( license_text )
        if isinstance( self.creators[0], license.license_t ):
            self.remove_creator( self.creators[0] )
        self.adopt_creator( license_inst, 0 )
    license = property( _get_license, _set_license,
                        doc="""License text.

                        The license text will always be the first children node.
                        @type: str or :class:`code_creators.license_t`""")

    def _get_system_files_impl( self ):
        return []

    @utils.cached
    def specially_exposed_decls(self):
        """list of exposed declarations, which were not ``included``, but still
        were exposed. For example, std containers.
        """
        decls = set()
        #select all declaration based code creators
        ccs = [cc for cc in algorithm.make_flatten_list( self ) if isinstance( cc, declaration_based.declaration_based_t )]
        #leave only "ignored"
        ccs = [cc for cc in ccs if cc.declaration.ignore == True]

        decls = [cc.declaration for cc in ccs]

        return set( decls )

    def update_documentation( self, doc_extractor ):
        if not doc_extractor:
            return
        visited = set()
        for cc in algorithm.make_flatten( self ):
            if not isinstance( cc, declaration_based.declaration_based_t ):
                continue
            if id( cc.declaration ) in visited:
                continue
            cc.declaration.documentation = doc_extractor( cc.declaration )
            visited.add( id( cc.declaration ) )

class bpmodule_t(module_t):
    """This class represents the source code for the entire extension module.

    The root of the code creator tree is always a module_t object.
    """
    def __init__(self, global_ns):
        """Constructor.
        """
        module_t.__init__(self, global_ns, bpmodule_t.CODE_GENERATOR_TYPES.BOOST_PYTHON)
        self.__body = None

    def _get_include_dirs(self):
        include_dirs = algorithm.creator_finder.find_by_class_instance(
            what=include_directories.include_directories_t
            , where=self.creators
            , recursive=False)
        if 0 == len( include_dirs ):
            include_dirs = include_directories.include_directories_t()
            if self.license:
                self.adopt_creator( include_dirs, 1 )
            else:
                self.adopt_creator( include_dirs, 0 )
            return include_dirs
        elif 1 == len( include_dirs ):
            return include_dirs[0]
        else:
            assert not "only single instance of include_directories_t should exist"

    def _get_std_directories(self):
        include_dirs = self._get_include_dirs()
        return include_dirs.std
    std_directories = property( _get_std_directories )

    def _get_user_defined_directories(self):
        include_dirs = self._get_include_dirs()
        return include_dirs.user_defined
    user_defined_directories = property( _get_user_defined_directories )

    @property
    def body(self):
        """Return reference to :class:`code_creators.module_body_t` code creator"""
        if None is self.__body:
            found = algorithm.creator_finder.find_by_class_instance( what=module_body.module_body_t
                                                                    , where=self.creators
                                                                    , recursive=False )
            if found:
                self.__body = found[0]
        return self.__body

    def last_include_index(self):
        """
        return the children index of the last :class:`code_creators.include_t` object.

        An exception is raised when there is no include_t object among
        the children creators.

        :rtype: int
        """
        for i in range( len(self.creators) - 1, -1, -1 ):
            if isinstance( self.creators[i], include.include_t ):
                return i
        else:
            return 0

    def replace_included_headers( self, headers, leave_system_headers=True ):
        to_be_removed = []
        for creator in self.creators:
            if isinstance( creator, include.include_t ):
                to_be_removed.append( creator )
            elif isinstance( creator, module_body.module_body_t ):
                break

        for creator in to_be_removed:
            if creator.is_system:
                if not leave_system_headers:
                    self.remove_creator( creator )
            elif creator.is_user_defined:
                pass
            else:
                self.remove_creator( creator )
        for header in headers:
            self.adopt_include( include.include_t( header=header ) )

    def adopt_include(self, include_creator):
        """Insert an :class:`code_creators.include_t` object.

        The include creator is inserted right after the last include file.

        :param include_creator: Include creator object
        :type include_creator: :class:`code_creators.include_t`
        """
        lii = self.last_include_index()
        if lii == 0:
            if not self.creators:
                lii = -1
            elif not isinstance( self.creators[0], include.include_t ):
                lii = -1
            else:
                pass
        self.adopt_creator( include_creator, lii + 1 )

    def do_include_dirs_optimization(self):
        include_dirs = self._get_include_dirs()
        includes = [creator for creator in self.creators if isinstance( creator, include.include_t )]
        for include_creator in includes:
            include_creator.include_dirs_optimization = include_dirs

    def _create_impl(self):
        self.do_include_dirs_optimization()
        index = 0
        code = []
        for index in range( len( self.creators ) ):
            if not isinstance( self.creators[index], include.include_t ):
                break
            else:
                code.append( self.creators[index].create() )
        if code:
            code.append( 2* os.linesep )
        code.append( self.create_internal_code( self.creators[index:], indent_code=False ))
        code.append( os.linesep )
        return os.linesep.join( code )

    def add_include( self, header, user_defined=True, system=False ):
        creator = include.include_t( header=header, user_defined=user_defined, system=system )
        self.adopt_include( creator )

    def add_namespace_usage( self, namespace_name ):
        self.adopt_creator( namespace.namespace_using_t( namespace_name )
                            , self.last_include_index() + 1 )

    def add_namespace_alias( self, alias, full_namespace_name ):
        self.adopt_creator( namespace.namespace_alias_t(
                                alias=alias
                                , full_namespace_name=full_namespace_name )
                            , self.last_include_index() + 1 )

    def adopt_declaration_creator( self, creator ):
        self.adopt_creator( creator, self.creators.index( self.body ) )

    def add_declaration_code( self, code, position ):
        self.adopt_declaration_creator( custom.custom_text_t( code ) )



class ctypes_module_t(module_t):
    """This class represents the source code for the entire extension module.

    The root of the code creator tree is always a module_t object.
    """
    def __init__(self, global_ns):
        """Constructor.
        """
        module_t.__init__(self, global_ns, ctypes_module_t.CODE_GENERATOR_TYPES.CTYPES)
        self.treat_char_ptr_as_binary_data = False
        
    def _create_impl(self):
        return self.create_internal_code( self.creators, indent_code=False )

    @utils.cached
    def library_var_name(self):
        for creator in self.creators:
            if isinstance( creator, library_reference.library_reference_t ):
                return creator.library_var_name
        else:
            raise RuntimeError( "Internal Error: library_reference_t creator was not created" )
