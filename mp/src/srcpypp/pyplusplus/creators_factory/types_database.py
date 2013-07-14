# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from pyplusplus import messages
from pygccxml import declarations
from pyplusplus import code_creators
from pyplusplus import _logging_
templates = declarations.templates

class types_database_t( object ):
    def __init__( self ):
        object.__init__( self )
        self.__variables = {} # decl_string : [type]
        self.__return_types = {} # decl_string : [type]
        self.__arguments_types = {} #decl_string : [type]
        self.__smart_ptrs = [ 'shared_ptr', 'auto_ptr' ]
        self.__fundamental_strs = list(declarations.FUNDAMENTAL_TYPES.keys())
        self.__normalize_data = [ ',', '<', '>', '*', '&', '(', ')', '::' ]
        self.__containers = set()

    def update_containers( self, decl ):
        assert decl.indexing_suite
        self.__containers.add( decl )

    def update( self, decl ):
        if isinstance( decl, declarations.calldef_t ):
            if not isinstance( decl, declarations.constructor_t ):
                self._update_db( self.__return_types, decl.return_type )
            for arg in decl.arguments:
                self._update_db( self.__arguments_types, arg.type )
        elif isinstance( decl, declarations.variable_t ):
            self._update_db( self.__variables, decl.type )
        else:
            assert not "types_database_t class can not process " + str( decl )

    def _is_relevant(self, decl_string):
        for smart_ptr in self.__smart_ptrs:
            if smart_ptr in decl_string:
                return True
        return False

    def _is_relevant_inst( self, name, args ):
        return self._is_relevant( name )

    def _normalize( self, decl_string ):
        if decl_string.startswith( '::' ):
            decl_string = decl_string[2:]
        answer = decl_string
        for data in self.__normalize_data:
            answer = answer.replace( data + ' ', data )
            answer = answer.replace( ' ' + data, data )
        return answer.replace( '  ', ' ' )

    def _update_containers_db( self, type_ ):
        #will return True is type was treated
        type_ = declarations.remove_alias( type_ )
        type_ = declarations.remove_pointer( type_ )
        type_ = declarations.remove_reference( type_ )
        type_ = declarations.remove_cv( type_ )
        type_ = declarations.remove_declarated( type_ )

        class_traits = declarations.class_traits
        class_declaration_traits = declarations.class_declaration_traits
        if not class_traits.is_my_case( type_ ) and not class_declaration_traits.is_my_case( type_ ):
            return False

        if class_traits.is_my_case( type_ ):
            container_cls = class_traits.get_declaration( type_ )
        else:
            container_cls = class_declaration_traits.get_declaration( type_ )

        if None is container_cls.indexing_suite:
            return False

        try:
            #check extraction of element type from container
            container_cls.indexing_suite.element_type
        except RuntimeError:
            decls_logger = _logging_.loggers.declarations
            if not messages.filter_disabled_msgs([messages.W1042], container_cls.disabled_messages ):
                return #user disabled property warning
            decls_logger.warn( "%s;%s" % ( container_cls, messages.W1042 ) )
        self.__containers.add( container_cls )
        return True


    def _update_db( self, db, type_ ):
        if self._update_containers_db( type_ ):
            return
        decl_string = self._normalize( declarations.base_type( type_ ).decl_string )
        if not templates.is_instantiation( decl_string ):
            return
        if not self._is_relevant( decl_string ):
            return
        insts = [inst for inst in templates.split_recursive( decl_string ) if self._is_relevant_inst( inst[0], inst[1] )]
        for smart_ptr, args in insts:
            assert len( args ) == 1
            pointee = self._normalize( args[0] )
            if pointee not in db:
                db[ pointee ] = []
            smart_ptr = self._normalize( smart_ptr )
            if (smart_ptr, type_) not in db[pointee]:
                db[ pointee ].append( (smart_ptr, type_) )

    def _find_smart_ptrs( self, db, class_decl ):
        decl_string = self._normalize( class_decl.decl_string )
        if decl_string in db:
            return db[ decl_string ]
        else:
            return None

    def create_holder( self, class_decl ):
        #holder should be created when we pass object created in python
        #as parameter to function in C++ that takes the smart pointer by reference
        found = self._find_smart_ptrs( self.__arguments_types, class_decl )
        if not found:
            return None#not found or ambiguty

        held_type = None
        for smart_ptr, type_ in found:
            if declarations.is_reference( type_ ) and not declarations.is_const( type_.base ):
                temp = code_creators.held_type_t( smart_ptr=smart_ptr )
                if not held_type or 'shared_ptr' in smart_ptr:
                    held_type = temp
        return held_type

    def _create_registrators_from_db( self, db, class_creator, registered ):
        spregistrator_t = code_creators.smart_pointer_registrator_t
        found = self._find_smart_ptrs( db, class_creator.declaration )
        if not found:
            return
        for smart_ptr, type_ in found:
            already_registered = [registrator for registrator in registered if registrator.smart_ptr == smart_ptr]
            if not already_registered:
                registered.append( spregistrator_t( smart_ptr=smart_ptr, class_creator=class_creator) )

    def create_registrators( self, class_creator ):
        """
        looks for all places where the class may be used as smart pointer.

        If found, then creates :class:`code_creators.smart_pointer_registrator_t`
        for that class and pointer type.
        """
        spconverter_t = code_creators.smart_pointers_converter_t
        registrators = []
        dbs = [ self.__arguments_types, self.__return_types, self.__variables ]
        for db in dbs:
            self._create_registrators_from_db( db, class_creator, registrators )
        if not class_creator.declaration.bases:
            return registrators
        # Add implicit converters from me to base classes and from derived classes to me
        answer = []
        for registrator in registrators:
            answer.append( registrator )
            decl = registrator.declaration
            for hierarchy_info in decl.recursive_bases:
                if hierarchy_info.access_type != declarations.ACCESS_TYPES.PRIVATE:
                    converter = spconverter_t( smart_ptr=registrator.smart_ptr
                                               , source=class_creator.declaration
                                               , target=hierarchy_info.related_class )
                    answer.append( converter )
            for hierarchy_info in decl.recursive_derived:
                if hierarchy_info.access_type != declarations.ACCESS_TYPES.PRIVATE:
                    converter = spconverter_t( smart_ptr=registrator.smart_ptr
                                               , source=hierarchy_info.related_class
                                               , target=class_creator.declaration )
                    answer.append( converter )
        return answer

    def _print_single_db(self, db):
        for decl_string in list(db.keys()):
            print('decl_string : ', decl_string)
            for smart_ptr, type_ in db[ decl_string ]:
                print('    smart_ptr : ', smart_ptr)
                print('    type_     : ', type_.decl_string)

    def print_db( self ):
        dbs = [ self.__arguments_types, self.__return_types, self.__variables ]
        for db in dbs:
            self._print_single_db( db )

    def _get_used_containers( self ):
        return self.__containers
    used_containers = property( _get_used_containers)

