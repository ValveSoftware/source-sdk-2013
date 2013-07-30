# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import sort_algorithms
from . import dependencies_manager
from pygccxml import binary_parsers
from pyplusplus import _logging_
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators
from pyplusplus import code_repository

ACCESS_TYPES = declarations.ACCESS_TYPES
VIRTUALITY_TYPES = declarations.VIRTUALITY_TYPES

class ctypes_creator_t( declarations.decl_visitor_t ):
    def __init__( self
                  , global_ns
                  , library_path
                  , exported_symbols ):
        declarations.decl_visitor_t.__init__(self)
        self.logger = _logging_.loggers.module_builder
        self.decl_logger = _logging_.loggers.declarations

        self.global_ns = global_ns

        self.__library_path = library_path
        self.__exported_symbols = exported_symbols
        self.__exported_decls = set( exported_symbols.values() )
        self.module = code_creators.ctypes_module_t( global_ns )
        self.__dependencies_manager = dependencies_manager.manager_t(self.decl_logger)

        #bookmark for class introductions
        self.__class_ccs = code_creators.bookmark_t()
        #bookmark for class deinitions
        self.__class_defs_ccs = code_creators.bookmark_t()
        #bookmark for typedef definitions
        self.__typedefs_ccs =  code_creators.bookmark_t()
        self.curr_decl = global_ns
        self.curr_code_creator = self.module
        #mapping between class declaration and class introduction code creator
        self.__class2introduction = {}
        #mapping between namespace and its code creator
        self.__namespace2pyclass = {}

    def __print_readme( self, decl ):
        readme = decl.readme()
        if not readme:
            return

        if not decl.exportable:
            reason = readme[0]
            readme = readme[1:]
            self.decl_logger.warn( "%s;%s" % ( decl, reason ) )

        for msg in readme:
            self.decl_logger.warn( "%s;%s" % ( decl, msg ) )

    def __should_generate_code( self, decl ):
        if decl.ignore or decl.already_exposed:
            return False
        if isinstance( decl, declarations.calldef_t ):
            return decl in self.__exported_decls
        if isinstance( decl, declarations.variable_t ):
            if isinstance( decl.parent, declarations.namespace_t ):
                return decl in self.__exported_decls
        return True

    def __contains_exported( self, decl ):
        return bool( decl.decls( self.__should_generate_code, recursive=True, allow_empty=True ) )

    # - implement better 0(n) algorithm
    def __add_class_introductions( self, cc, class_ ):
        if not self.__should_generate_code( class_ ):
            return
        ci_creator = code_creators.class_introduction_t( class_ )
        self.__class2introduction[ class_ ] = ci_creator
        cc.adopt_creator( ci_creator )
        classes = class_.classes( recursive=False, allow_empty=True)
        classes = sort_algorithms.sort_classes( classes, include_vars=True )
        for internal_class in classes:
            self.__add_class_introductions( ci_creator, internal_class )

        if not class_.opaque:
            self.__class_defs_ccs.adopt_creator( code_creators.fields_definition_t( class_ ) )
            
    def create(self ):
        """
        create and return the module for the extension - code creators tree root

        :rtype: :class:`code_creators.module_t`
        """
        # Invoke the appropriate visit_*() method on all decls
        ccc = self.curr_code_creator
        ccc.adopt_creator( code_creators.import_t( 'ctypes' ) )
        ccc.adopt_creator( code_creators.import_t( code_repository.ctypes_utils.file_name  ) )

        ccc.adopt_creator( code_creators.separator_t() )

        ccc.adopt_creator( code_creators.library_reference_t( self.__library_path ) )
        ccc.adopt_creator( code_creators.name_mappings_t( self.__exported_symbols ) )

        ccc.adopt_creator( code_creators.separator_t() )
        #adding namespaces
        global_ns_cc = code_creators.bookmark_t()
        ccc.adopt_creator( global_ns_cc )
        ccc.adopt_creator( self.__class_ccs )
        self.__namespace2pyclass[ self.global_ns ] = global_ns_cc
        #adding class introductions - special case because of hierarchy
        f = lambda cls: self.__should_generate_code( cls ) \
                        and isinstance( cls.parent, declarations.namespace_t )
        ns_classes = self.global_ns.classes( f, recursive=True, allow_empty=True)
        ns_classes = sort_algorithms.sort_classes( ns_classes, include_vars=True )
        for class_ in ns_classes:
            self.__add_class_introductions( self.__class_ccs, class_ )

        ccc.adopt_creator( self.__class_defs_ccs )

        ccc.adopt_creator( code_creators.separator_t() )

        ccc.adopt_creator( self.__typedefs_ccs )

        declarations.apply_visitor( self, self.curr_decl )

        self.__dependencies_manager.inform_user()

        return self.module

    def visit_member_function( self ):
        pass #c code doesn't have member functions
    def visit_constructor( self ):
        pass #c code doesn't have member functions    
    def visit_destructor( self ):
        pass #c code doesn't have member functions
    def visit_member_operator( self ):
        pass #c code doesn't have member functions
    def visit_casting_operator( self ):
        pass #c code doesn't have member functions
    def visit_free_function( self ):
        self.__dependencies_manager.add_exported( self.curr_decl )
        self.curr_code_creator.adopt_creator( code_creators.function_definition_t( self.curr_decl ) )

    def visit_free_operator( self ):
        self.__dependencies_manager.add_exported( self.curr_decl )

    def visit_class_declaration(self ):
        self.__dependencies_manager.add_exported( self.curr_decl )
        ci_creator = code_creators.class_declaration_introduction_t( self.curr_decl )
        self.__class_ccs.adopt_creator( ci_creator )

    def visit_class(self):
        self.__dependencies_manager.add_exported( self.curr_decl )
        if self.curr_decl.opaque:
            cls_intro_cc = self.__class2introduction[ self.curr_decl ]
            cls_intro_cc.adopt_creator( code_creators.opaque_init_introduction_t( self.curr_decl ) )
        else:
            class_ = self.curr_decl
            for decl in self.curr_decl.decls( recursive=False, allow_empty=True ):
                if isinstance( decl, declarations.variable_t ):
                    continue #fields_definition_t class treats them
                if self.__should_generate_code( decl ):
                    self.curr_decl = decl
                    declarations.apply_visitor( self, decl )
            self.curr_decl = class_

    def visit_enumeration(self):
        self.__dependencies_manager.add_exported( self.curr_decl )
        paretn_cc = None
        if isinstance( self.curr_decl.parent, declarations.class_t ):
            paretn_cc = self.__class2introduction[ self.curr_decl.parent ]
        else:
            paretn_cc = self.__namespace2pyclass[ self.curr_decl.parent ]
        paretn_cc.adopt_creator( code_creators.pyenum_t( self.curr_decl ) )

    def visit_typedef(self):
        self.__dependencies_manager.add_exported( self.curr_decl )
        self.__typedefs_ccs.adopt_creator( code_creators.typedef_as_pyvar_t( self.curr_decl ) )

    def visit_variable(self):
        self.__dependencies_manager.add_exported( self.curr_decl )
        self.curr_code_creator.adopt_creator( code_creators.global_variable_reference_t( self.curr_decl ) )

    def visit_namespace(self ):
        if not self.__contains_exported( self.curr_decl ):
            return
        if self.global_ns is not self.curr_decl:
            ns_creator = code_creators.namespace_as_pyclass_t( self.curr_decl )
            self.__namespace2pyclass[ self.curr_decl ] = ns_creator
            self.__namespace2pyclass[ self.curr_decl.parent ].adopt_creator( ns_creator )

        ns = self.curr_decl
        for decl in self.curr_decl.decls( recursive=False, allow_empty=True ):
            if isinstance( decl, declarations.namespace_t) or self.__should_generate_code( decl ):
                self.curr_decl = decl
                declarations.apply_visitor( self, decl )
        self.curr_decl = ns


