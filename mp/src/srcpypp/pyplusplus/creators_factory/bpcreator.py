# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import types_database
from . import creators_wizard
from . import sort_algorithms
from . import dependencies_manager
from . import opaque_types_manager
from . import call_policies_resolver
from . import fake_constructors_manager

from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators
from pyplusplus import code_repository
from pyplusplus import _logging_

ACCESS_TYPES = declarations.ACCESS_TYPES
VIRTUALITY_TYPES = declarations.VIRTUALITY_TYPES

class bpcreator_t( declarations.decl_visitor_t ):
    """
    code creators factory for Boost.Python library

    This class takes a set of declarations as input and creates a code
    creators tree that contains the Boost.Python C++ source code for the
    final extension module. Each node in the code creators tree represents
    a block of text (C++ source code).
    """

    def __init__( self
                  , decls
                  , module_name
                  , boost_python_ns_name='bp'
                  , call_policies_resolver_=None
                  , types_db=None
                  , target_configuration=None
                  , enable_indexing_suite=True ):
        """Constructor.

        :param decls: Declarations that should be exposed in the final module.
        :param module_name: The name of the final module.
        :param boost_python_ns_name: The alias for the boost::python namespace.
        :param call_policies_resolver_: Callable that takes one declaration (calldef_t) as input and returns a call policy object which should be used for this declaration.
        :param types_db: ...todo...
        :param target_configuration: A target configuration object can be used to customize the generated source code to a particular compiler or a particular version of Boost.Python.
        :param already_exposed_dbs: list of files/directories other modules, this module depends on, generated their code too
        :type decls: list of declaration_t
        :type module_name: str
        :type boost_python_ns_name: str
        :type call_policies_resolver_: callable
        :type types_db: L:class:`types_database.types_database_t`
        :type target_configuration: :class:`code_creators.target_configuration_t`
        :type already_exposed_dbs: list of strings
        """
        declarations.decl_visitor_t.__init__(self)
        self.logger = _logging_.loggers.module_builder
        self.decl_logger = _logging_.loggers.declarations

        self.__enable_indexing_suite = enable_indexing_suite
        self.__target_configuration = target_configuration
        if not self.__target_configuration:
            self.__target_configuration = code_creators.target_configuration_t()

        self.__call_policies_resolver = call_policies_resolver_
        if not self.__call_policies_resolver:
            self.__call_policies_resolver \
                = call_policies_resolver.built_in_resolver_t(self.__target_configuration)

        self.__types_db = types_db
        if not self.__types_db:
            self.__types_db = types_database.types_database_t()

        global_ns = declarations.get_global_namespace(decls)

        self.__extmodule = code_creators.bpmodule_t( global_ns )
        if boost_python_ns_name:
            bp_ns_alias = code_creators.namespace_alias_t( alias=boost_python_ns_name
                                                           , full_namespace_name='::boost::python' )
            self.__extmodule.adopt_creator( bp_ns_alias )

        self.__module_body = code_creators.module_body_t( name=module_name )

        self.__extmodule.adopt_creator( self.__module_body )

        self.__opaque_types_manager = opaque_types_manager.manager_t( self.__extmodule )
        self.__dependencies_manager = dependencies_manager.manager_t(self.decl_logger)

        prepared_decls = self._prepare_decls( decls )
        self.__decls = sort_algorithms.sort( prepared_decls )

        self.curr_code_creator = self.__module_body
        self.curr_decl = None
        self.__array_1_registered = set() #(type.decl_string,size)
        self.__free_operators = []
        self.__std_containers_free_operators = {}
        self.__exposed_free_fun_overloads = set()
        self.__fc_manager = fake_constructors_manager.manager_t( global_ns )

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

    def _prepare_decls( self, decls ):
        to_be_exposed = []
        for decl in declarations.make_flatten( decls ):
            if decl.ignore:
                continue

            if isinstance( decl, declarations.namespace_t ):
                continue

            if isinstance( decl, declarations.class_types ):
                if decl.opaque:
                    continue

            if not decl.exportable:
                #leave only decls that user wants to export and that could be exported
                self.__print_readme( decl )
                continue

            if decl.already_exposed:
                #check wether this is already exposed in other module
                continue

            if isinstance( decl.parent, declarations.namespace_t ):
                #leave only declarations defined under namespace, but remove namespaces
                to_be_exposed.append( decl )

            #Right now this functionality introduce a bug: declarations that should
            #not be exported for some reason are not marked as such. I will need to
            #find out.
            #if isinstance( decl, declarations.calldef_t ) and not isinstance( decl, declarations.destructor_t ):
                #self.__types_db.update( decl )
                #if None is decl.call_policies:
                    #decl.call_policies = self.__call_policies_resolver( decl )

            #if isinstance( decl, declarations.variable_t ):
                #self.__types_db.update( decl )

            self.__print_readme( decl )

        return to_be_exposed

    def _adopt_free_operator( self, operator ):
        def adopt_operator_impl( operator, found_creators ):
            creator = [creator for creator in found_creators if isinstance( creator, code_creators.class_t )]
            if len(creator) == 1:
                creator = creator[0]
                #I think I don't need this condition any more
                if not find( lambda creator: isinstance( creator, code_creators.declaration_based_t )
                                             and operator is creator.declaration
                             , creator.creators ):
                    #expose operator only once
                    self.__dependencies_manager.add_exported( operator )
                    creator.adopt_creator( code_creators.operator_t( operator=operator ) )
            elif not creator:
                pass
            else:
                assert not "Found %d class code creators" % len(creator)
        find = code_creators.creator_finder.find_by_declaration
        if operator.target_class and operator.target_class.ignore == False:
            found = find( lambda decl: operator.target_class is decl
                          , self.__extmodule.body.creators )
            adopt_operator_impl( operator, found )

    def _is_registered_smart_pointer_creator( self, creator, db ):
        for registered in db:
            if not isinstance( creator, registered.__class__ ):
                continue
            elif registered.smart_ptr != creator.smart_ptr:
                continue
            elif isinstance( creator, code_creators.smart_pointer_registrator_t ):
                if creator.declaration is registered.declaration:
                    return True
            elif isinstance( creator, code_creators.smart_pointers_converter_t ):
                if ( creator.source is registered.source ) \
                   and ( creator.target is registered.target ):
                    return True
            else:
                assert not "unknown instace of registrator: " % str( registered )

    def _treat_smart_pointers( self ):
        """
        goes over all class creators and apply held_type and registers smart pointers
        classes as needed
        """
        find_classes = code_creators.creator_finder.find_by_class_instance
        class_creators = find_classes( what=code_creators.class_t
                                       , where=self.__extmodule.body.creators
                                       , recursive=True )
        registrators_db = []
        for creator in class_creators:
            if None is creator.held_type:
                if not creator.declaration.is_abstract:
                    creator.held_type = self.__types_db.create_holder( creator.declaration )
            registrators = self.__types_db.create_registrators( creator )
            for r in registrators:
                if not self._is_registered_smart_pointer_creator( r, registrators_db ):
                    creator.adopt_creator(r)
                    registrators_db.append(r)

    def _append_user_code( self ):
        find_classes = code_creators.creator_finder.find_by_class_instance
        class_creators = find_classes( what=code_creators.class_t
                                       , where=self.__extmodule.body.creators
                                       , recursive=True )

        ctext_t = code_creators.custom_text_t
        for cls_creator in class_creators:
            cls_decl = cls_creator.declaration

            uc_creators_head = [ctext_t( uc.text, uc.works_on_instance ) for uc in cls_decl.registration_code_head]
            cls_creator.adopt_creators( uc_creators_head, 0 )

            uc_creators_tail = [ctext_t( uc.text, uc.works_on_instance ) for uc in cls_decl.registration_code_tail]
            cls_creator.adopt_creators( uc_creators_tail )

            uc_creators = [ctext_t( uc.text ) for uc in cls_decl.wrapper_code]
            if uc_creators:
                cls_creator.wrapper.adopt_creators( uc_creators )

            uc_creators = [ctext_t( uc.text ) for uc in cls_decl.declaration_code]
            insert_pos = self.__extmodule.creators.index( self.__module_body )
            self.__extmodule.adopt_creators( uc_creators, insert_pos )
            cls_creator.associated_decl_creators.extend( uc_creators )

    def __get_exposed_containers(self):
        """list of exposed declarations, which were not ``included``, but still
        were exposed. For example, std containers

        std containers exposed by `Py++`, even if the user didn't ``include`` them.
        """
        cmp_by_name = lambda cls: cls.decl_string
        used_containers = list( self.__types_db.used_containers )
        used_containers = [cls for cls in used_containers if cls.indexing_suite.include_files]
        used_containers.sort( key=cmp_by_name )
        used_containers = [cnt for cnt in used_containers if cnt.already_exposed == False]
        return used_containers

    def _treat_indexing_suite( self ):
        def create_explanation(cls):
            msg = '//WARNING: the next line of code will not compile, because "%s" does not have operator== !'
            msg = msg % cls.indexing_suite.element_type.decl_string
            return code_creators.custom_text_t( msg, False )

        def create_cls_cc( cls ):
            if isinstance( cls, declarations.class_t ):
                return code_creators.class_t( class_inst=cls )
            else:
                return code_creators.class_declaration_t( class_inst=cls )

        if not self.__types_db.used_containers:
            return

        creators = []
        created_value_traits = set()
        for cls in self.__get_exposed_containers():
            self.__print_readme( cls )

            cls_creator = create_cls_cc( cls )
            self.__dependencies_manager.add_exported( cls )
            creators.append( cls_creator )
            try:
                element_type = cls.indexing_suite.element_type
            except:
                element_type = None

            if isinstance( cls.indexing_suite, decl_wrappers.indexing_suite1_t ):
                if not ( None is element_type ) \
                   and declarations.is_class( element_type ) \
                   and not declarations.has_public_equal( element_type ):
                    cls_creator.adopt_creator( create_explanation( cls ) )
                cls_creator.adopt_creator( code_creators.indexing_suite1_t(cls) )
            else:
                class_traits = declarations.class_traits
                if not ( None is element_type ) and class_traits.is_my_case( element_type ):
                    value_cls = class_traits.get_declaration( element_type )
                    has_prerequisits = value_cls.less_than_comparable \
                                       and value_cls.equality_comparable
                    if ( not has_prerequisits ) and ( value_cls not in created_value_traits ):
                        created_value_traits.add( value_cls )
                        element_type_cc = code_creators.value_traits_t( value_cls )
                        self.__extmodule.adopt_declaration_creator( element_type_cc )
                cls_creator.adopt_creator( code_creators.indexing_suite2_t(cls) )

            scfo = self.__std_containers_free_operators
            if cls in scfo:
                for operator in scfo[cls]:
                    self.__dependencies_manager.add_exported( operator )
                    cls_creator.adopt_creator( code_creators.operator_t( operator=operator ) )

        creators.reverse()
        self.__module_body.adopt_creators( creators, 0 )

    def create(self, decl_headers=None):
        """
        create and return the module for the extension - code creators tree root.

        :param decl_headers: If None the headers for the wrapped decls are automatically found.
                             But you can pass a list of headers here to override that search.
        :rtype: :class:`code_creators.module_t`
        """
        # Invoke the appropriate visit_*() method on all decls
        for decl in self.__decls:
            self.curr_decl = decl
            declarations.apply_visitor( self, decl )
        for operator in self.__free_operators:
            self._adopt_free_operator( operator )
        self._treat_smart_pointers()
        if self.__enable_indexing_suite:
            self._treat_indexing_suite()
        for creator in code_creators.make_flatten_generator( self.__extmodule ):
            creator.target_configuration = self.__target_configuration
        #last action.
        self._append_user_code()

        add_include = self.__extmodule.add_include
        #add system headers
        system_headers = self.__extmodule.get_system_files( recursive=True, unique=True, language='c++' )
        for header in system_headers:
            add_include( header, user_defined=False, system=True )
        #add user defined header files
        if decl_headers is None:
            decl_headers = declarations.declaration_files( self.__decls )
        for header in decl_headers:
            add_include( header, user_defined=False, system=False )

        self.__dependencies_manager.inform_user()

        return self.__extmodule

    def visit_member_function( self ):
        self.__types_db.update( self.curr_decl )
        self.__dependencies_manager.add_exported( self.curr_decl )

        if self.__fc_manager.is_fake_constructor( self.curr_decl ):
            return

        fwrapper = None
        if None is self.curr_decl.call_policies:
            self.curr_decl.call_policies = self.__call_policies_resolver( self.curr_decl )

        maker_cls, fwrapper_cls = creators_wizard.find_out_mem_fun_creator_classes( self.curr_decl )

        maker = None
        fwrapper = None
        if fwrapper_cls:
            fwrapper = fwrapper_cls( function=self.curr_decl )
            if fwrapper_cls is code_creators.mem_fun_transformed_wrapper_t:
                if self.curr_code_creator.wrapper:
                    class_wrapper = self.curr_code_creator.wrapper
                    class_wrapper.adopt_creator( fwrapper )
                else:
                    self.__extmodule.adopt_declaration_creator( fwrapper )
                    self.curr_code_creator.associated_decl_creators.append(fwrapper)
            else:
                class_wrapper = self.curr_code_creator.wrapper
                class_wrapper.adopt_creator( fwrapper )

        if maker_cls:
            if fwrapper:
                maker = maker_cls( function=self.curr_decl, wrapper=fwrapper )
            else:
                maker = maker_cls( function=self.curr_decl )
            self.curr_code_creator.adopt_creator( maker )
            self.__opaque_types_manager.register_opaque( maker, self.curr_decl )

        if self.curr_decl.has_static:
            #static_method should be created only once.
            found = [creator for creator in self.curr_code_creator.creators if isinstance( creator, code_creators.static_method_t )
                                            and creator.declaration.name == self.curr_decl.name]
            if not found:
                static_method = code_creators.static_method_t( function=self.curr_decl
                                                               , function_code_creator=maker )
                self.curr_code_creator.adopt_creator( static_method )

    def visit_constructor( self ):
        self.__types_db.update( self.curr_decl )
        self.__dependencies_manager.add_exported( self.curr_decl )

        cwrapper = None
        if self.curr_decl.parent.is_wrapper_needed():
            class_wrapper = self.curr_code_creator.wrapper
            cwrapper = code_creators.constructor_wrapper_t( constructor=self.curr_decl )
            class_wrapper.adopt_creator( cwrapper )
        #TODO: FT for constructor
            #~ if self.curr_decl.transformations:
                #~ cwrapper = code_creators.constructor_transformed_wrapper_t( constructor=self.curr_decl )
                #~ class_wrapper.adopt_creator( cwrapper )
        #~ else:
            #~ if self.curr_decl.transformations:
                #~ cwrapper = code_creators.constructor_transformed_wrapper_t( constructor=self.curr_decl )
                #~ class_wrapper.adopt_creator( cwrapper )
                #~ self.__module_body.adopt_creator( cwrapper )
                #~ self.curr_code_creator.associated_decl_creators.append( cwrapper )
        #~ maker = None
        #~ if self.curr_decl.transformations:
            #~ maker = code_creators.constructor_transformed_t( constructor=self.curr_decl )
        #~ else:
        maker = code_creators.constructor_t( constructor=self.curr_decl, wrapper=cwrapper )
        if None is self.curr_decl.call_policies:
            self.curr_decl.call_policies = self.__call_policies_resolver( self.curr_decl )
        self.curr_code_creator.adopt_creator( maker )

        if self.curr_decl.allow_implicit_conversion:
            maker = code_creators.casting_constructor_t( constructor=self.curr_decl )
            #casting constructor will be generated in the same file as class
            self.curr_code_creator.adopt_creator( maker )


    def visit_destructor( self ):
        pass

    def visit_member_operator( self ):
        if self.curr_decl.symbol in ( '()', '[]', '=' ):
            self.visit_member_function()
        else:
            self.__types_db.update( self.curr_decl )
            maker = code_creators.operator_t( operator=self.curr_decl )
            self.curr_code_creator.adopt_creator( maker )
            self.__dependencies_manager.add_exported( self.curr_decl )

    def visit_casting_operator( self ):
        self.__dependencies_manager.add_exported( self.curr_decl )
        if None is self.curr_decl.call_policies:
            self.curr_decl.call_policies = self.__call_policies_resolver( self.curr_decl )

        self.__types_db.update( self.curr_decl )
        if not self.curr_decl.parent.is_abstract and not declarations.is_reference( self.curr_decl.return_type ):
            maker = code_creators.casting_operator_t( operator=self.curr_decl )
            self.__module_body.adopt_creator( maker )
            self.__opaque_types_manager.register_opaque( maker, self.curr_decl )

        #what to do if class is abstract
        maker = code_creators.casting_member_operator_t( operator=self.curr_decl )
        self.curr_code_creator.adopt_creator( maker )
        self.__opaque_types_manager.register_opaque( maker, self.curr_decl )

    def visit_free_function( self ):
        if self.curr_decl in self.__exposed_free_fun_overloads:
            return

        if self.__fc_manager.is_fake_constructor( self.curr_decl ):
            self.__types_db.update( self.curr_decl )
            self.__dependencies_manager.add_exported( self.curr_decl )
            return

        elif self.curr_decl.use_overload_macro:
            parent_decl = self.curr_decl.parent
            names = set( [decl.name for decl in parent_decl.free_functions( allow_empty=True, recursive=False )] )
            for name in names:
                overloads = parent_decl.free_functions( name, allow_empty=True, recursive=False )
                overloads = [decl for decl in overloads if decl.ignore == False and decl.use_overload_macro]
                if not overloads:
                    continue
                else:
                    self.__exposed_free_fun_overloads.update( overloads )
                    for f in overloads:
                        self.__types_db.update( f )
                        self.__dependencies_manager.add_exported( f )
                        if None is f.call_policies:
                            f.call_policies = self.__call_policies_resolver( f )

                    overloads_cls_creator = code_creators.free_fun_overloads_class_t( overloads )
                    self.__extmodule.adopt_declaration_creator( overloads_cls_creator )

                    overloads_reg = code_creators.free_fun_overloads_t( overloads_cls_creator )
                    self.curr_code_creator.adopt_creator( overloads_reg )
                    overloads_reg.associated_decl_creators.append( overloads_cls_creator )
                    self.__opaque_types_manager.register_opaque( overloads_reg, overloads )

                    ctext_t = code_creators.custom_text_t
                    for f in overloads:
                        uc_creators = [ctext_t( uc.text ) for uc in f.declaration_code]
                        insert_pos = self.__extmodule.creators.index( self.__module_body )
                        self.__extmodule.adopt_creators( uc_creators, insert_pos )
                        overloads_reg.associated_decl_creators.extend( uc_creators )
        else:
            self.__types_db.update( self.curr_decl )
            self.__dependencies_manager.add_exported( self.curr_decl )
            if None is self.curr_decl.call_policies:
                self.curr_decl.call_policies = self.__call_policies_resolver( self.curr_decl )

            maker = None
            if self.curr_decl.transformations:
                wrapper = code_creators.free_fun_transformed_wrapper_t( self.curr_decl )
                self.__extmodule.adopt_declaration_creator( wrapper )
                maker = code_creators.free_fun_transformed_t( self.curr_decl, wrapper )
                maker.associated_decl_creators.append( wrapper )
            else:
                maker = code_creators.free_function_t( function=self.curr_decl )
            self.curr_code_creator.adopt_creator( maker )
            self.__opaque_types_manager.register_opaque( maker, self.curr_decl )

            ctext_t = code_creators.custom_text_t
            uc_creators = [ctext_t( uc.text ) for uc in self.curr_decl.declaration_code]
            insert_pos = self.__extmodule.creators.index( self.__module_body )
            self.__extmodule.adopt_creators( uc_creators, insert_pos )
            maker.associated_decl_creators.extend( uc_creators )

    def visit_free_operator( self ):
        self.__types_db.update( self.curr_decl )

        operator = self.curr_decl
        target_class = operator.target_class
        scfo = self.__std_containers_free_operators
        if target_class and target_class.indexing_suite:
            if target_class not in scfo:
                scfo[ target_class ] = [ operator ]
            else:
                scfo[ target_class ].append( operator )
        else:
            self.__free_operators.append( self.curr_decl )

    def visit_class_declaration(self ):
        pass

    def expose_overloaded_mem_fun_using_macro( self, cls, cls_creator ):
        #returns set of exported member functions
        exposed = set()
        names = set( [decl.name for decl in cls.member_functions( allow_empty=True, recursive=False )] )
        for name in names:
            overloads = cls.member_functions( name, allow_empty=True, recursive=False )
            overloads = [decl for decl in overloads if decl.ignore == False and decl.use_overload_macro]
            if not overloads:
                continue
            else:
                exposed.update( overloads )

                for f in overloads:
                    self.__types_db.update( f )
                    self.__dependencies_manager.add_exported( f )
                    if None is f.call_policies:
                        f.call_policies = self.__call_policies_resolver( f )

                overloads_cls_creator = code_creators.mem_fun_overloads_class_t( overloads )
                self.__extmodule.adopt_declaration_creator( overloads_cls_creator )

                overloads_reg = code_creators.mem_fun_overloads_t( overloads_cls_creator )
                cls_creator.adopt_creator( overloads_reg )
                overloads_reg.associated_decl_creators.append( overloads_cls_creator )

                self.__opaque_types_manager.register_opaque( overloads_reg, overloads )
        return exposed

    def visit_class(self ):
        if self.curr_decl.indexing_suite:
            self.__types_db.update_containers( self.curr_decl )
            return #it will be exposed later, using other code creators
        self.__dependencies_manager.add_exported( self.curr_decl )
        cls_decl = self.curr_decl
        cls_parent_cc = self.curr_code_creator
        exportable_members = self.curr_decl.get_exportable_members(sort_algorithms.sort)

        wrapper = None
        cls_cc = None
        if cls_decl.introduces_new_scope:
            cls_cc = code_creators.class_t( class_inst=self.curr_decl )
        else:
            cls_cc = self.curr_code_creator

        if self.curr_decl.is_wrapper_needed():
            wrapper = code_creators.class_wrapper_t( declaration=self.curr_decl
                                                     , class_creator=cls_cc )
            cls_cc.wrapper = wrapper
            cls_cc.associated_decl_creators.append( wrapper )
            #insert wrapper before module body
            if isinstance( self.curr_decl.parent, declarations.class_t ):
                #we deal with internal class
                self.curr_code_creator.wrapper.adopt_creator( wrapper )
            else:
                self.__extmodule.adopt_declaration_creator( wrapper )

            #next constructors are not present in code, but compiler generated
            #Boost.Python requiers them to be declared in the wrapper class
            noncopyable_vars = self.curr_decl.find_noncopyable_vars()

            copy_constr = self.curr_decl.find_copy_constructor()
            if not self.curr_decl.noncopyable and copy_constr and copy_constr.is_artificial:
                cccc = code_creators.copy_constructor_wrapper_t( constructor=copy_constr)
                wrapper.adopt_creator( cccc )

            trivial_constr = self.curr_decl.find_trivial_constructor()
            if trivial_constr and trivial_constr.is_artificial and not noncopyable_vars:
                tcons = code_creators.null_constructor_wrapper_t( constructor=trivial_constr )
                wrapper.adopt_creator( tcons )

        exposed = self.expose_overloaded_mem_fun_using_macro( cls_decl, cls_cc )

        if cls_decl.introduces_new_scope:
            cls_parent_cc.adopt_creator( cls_cc )
        self.curr_code_creator = cls_cc

        if cls_decl.expose_this:
            cls_cc.adopt_creator( code_creators.expose_this_t( cls_decl ) )

        if cls_decl.expose_sizeof:
            cls_cc.adopt_creator( code_creators.expose_sizeof_t( cls_decl ) )

        for decl in exportable_members:
            if decl in exposed:
                continue
            self.curr_decl = decl
            declarations.apply_visitor( self, decl )

        for redefined_func in cls_decl.redefined_funcs():
            if isinstance( redefined_func, declarations.operator_t ):
                continue
            self.curr_decl = redefined_func
            declarations.apply_visitor( self, redefined_func )

        #all static_methods_t should be moved to the end
        #better approach is to move them after last def of relevant function
        static_methods = [creator for creator in cls_cc.creators if isinstance( creator, code_creators.static_method_t )]
        for static_method in static_methods:
            cls_cc.remove_creator( static_method )
            cls_cc.adopt_creator( static_method )

        if cls_decl.exception_translation_code:
            translator = code_creators.exception_translator_t( cls_decl )
            self.__extmodule.adopt_declaration_creator( translator )
            cls_cc.associated_decl_creators.append( translator )
            translator_register \
                = code_creators.exception_translator_register_t( cls_decl, translator )
            cls_cc.adopt_creator( translator_register )

        for property_def in cls_decl.properties:
            cls_cc.adopt_creator( code_creators.property_t(property_def) )

        if wrapper and cls_decl.destructor_code:
            destructor = code_creators.destructor_wrapper_t( class_=cls_decl )
            wrapper.adopt_creator( destructor )

        for fc in cls_decl.fake_constructors:
            if self.__fc_manager.should_generate_code( fc ):
                self.__dependencies_manager.add_exported( fc )
                cls_cc.adopt_creator( code_creators.make_constructor_t( fc ) )

        self.curr_decl = cls_decl
        self.curr_code_creator = cls_parent_cc


    def visit_enumeration(self):
        self.__dependencies_manager.add_exported( self.curr_decl )
        maker = None
        if self.curr_decl.name:
            maker = code_creators.enum_t( enum=self.curr_decl )
        else:
            maker = code_creators.unnamed_enum_t( unnamed_enum=self.curr_decl )
        self.curr_code_creator.adopt_creator( maker )

    def visit_namespace(self):
        pass

    def visit_typedef(self):
        pass

    def _register_array_1( self, array_type ):
        data = ( array_type.decl_string, declarations.array_size( array_type ) )
        if data in self.__array_1_registered:
            return False
        else:
            self.__array_1_registered.add( data )
            return True

    def visit_variable(self):
        self.__types_db.update( self.curr_decl )
        self.__dependencies_manager.add_exported( self.curr_decl )

        if self.curr_decl.expose_address:
            creator_type = None
            if isinstance( self.curr_decl.parent, declarations.namespace_t ):
                creator_type = code_creators.global_variable_addressof_t
            else:
                creator_type = code_creators.member_variable_addressof_t
            self.curr_code_creator.adopt_creator( creator_type(self.curr_decl) )
            return

        if not self.curr_decl.expose_value:
            return

        if declarations.is_array( self.curr_decl.type ):
            if self._register_array_1( self.curr_decl.type ):
                array_1_registrator = code_creators.array_1_registrator_t( array_type=self.curr_decl.type )
                self.curr_code_creator.adopt_creator( array_1_registrator )

        if isinstance( self.curr_decl.parent, declarations.namespace_t ):
            maker = None
            wrapper = None
            if declarations.is_array( self.curr_decl.type ):
                wrapper = code_creators.array_gv_wrapper_t( variable=self.curr_decl )
                maker = code_creators.array_gv_t( variable=self.curr_decl, wrapper=wrapper )
            else:
                maker = code_creators.global_variable_t( variable=self.curr_decl )
            if wrapper:
                self.__extmodule.adopt_declaration_creator( wrapper )
        else:
            maker = None
            wrapper = None
            if self.curr_decl.bits != None:
                wrapper = code_creators.bit_field_wrapper_t( variable=self.curr_decl )
                maker = code_creators.bit_field_t( variable=self.curr_decl, wrapper=wrapper )
            elif declarations.is_array( self.curr_decl.type ):
                wrapper = code_creators.array_mv_wrapper_t( variable=self.curr_decl )
                maker = code_creators.array_mv_t( variable=self.curr_decl, wrapper=wrapper )
            elif declarations.is_pointer( self.curr_decl.type ):
                wrapper = code_creators.member_variable_wrapper_t( variable=self.curr_decl )
                maker = code_creators.member_variable_t( variable=self.curr_decl, wrapper=wrapper )
            elif declarations.is_reference( self.curr_decl.type ):
                if None is self.curr_decl.getter_call_policies:
                    self.curr_decl.getter_call_policies = self.__call_policies_resolver( self.curr_decl, 'get' )
                if None is self.curr_decl.setter_call_policies:
                    self.curr_decl.setter_call_policies = self.__call_policies_resolver( self.curr_decl, 'set' )
                wrapper = code_creators.mem_var_ref_wrapper_t( variable=self.curr_decl )
                maker = code_creators.mem_var_ref_t( variable=self.curr_decl, wrapper=wrapper )
                self.__opaque_types_manager.register_opaque( maker, self.curr_decl )
            else:
                maker = code_creators.member_variable_t( variable=self.curr_decl )
            if wrapper:
                self.curr_code_creator.wrapper.adopt_creator( wrapper )
        self.curr_code_creator.adopt_creator( maker )
