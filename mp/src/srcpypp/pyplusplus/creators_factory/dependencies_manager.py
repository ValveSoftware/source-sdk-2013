# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class, which informs user about used, but unexposed declarations"""

import os
from pyplusplus import utils
from pyplusplus import messages
from pygccxml import declarations
from pyplusplus import decl_wrappers

class duplicated_names_reporter_t(object):
    def __init__( self, decls, value_getter, msg ):
        self.decls = decls
        self.get_value = value_getter
        self.msg = msg

    def __select( self ):
        duplicated = {}
        for decl in self.decls:
            value = self.get_value( decl )
            if value not in duplicated:
                duplicated[ value ] = set()
            duplicated[ value ].add( decl )
        result = duplicated.copy()
        for value, buggy_decls in list(duplicated.items()):
            if 1 == len( buggy_decls ):
                del result[ value ]
        return result

    def __report_single( self, control_decl, duplicated, logger ):
        value = self.get_value( control_decl )
        if value not in duplicated:
            return
        buggy_decls = duplicated[value].copy()
        buggy_decls.remove( control_decl )
        warning = self.msg % ( value, os.linesep.join( map( str, buggy_decls ) ) )
        logger.warn( "%s;%s" % ( str( control_decl ), warning ) )

    def report( self, logger ):
        duplicated = self.__select()
        for decl in self.decls:
            self.__report_single( decl, duplicated, logger )

duplicated_aliases_reporter \
    = lambda decls: duplicated_names_reporter_t( decls, lambda d: d.alias, messages.W1047 )

duplicated_wrapper_aliases_reporter \
    = lambda decls: duplicated_names_reporter_t( decls, lambda d: d.wrapper_alias, messages.W1065 )


class manager_t( object ):
    def __init__( self, logger ):
        object.__init__( self )
        self.__exported_decls = []
        self.__logger = logger

    def add_exported( self, decl ):
        self.__exported_decls.append( decl )
        if isinstance( decl, declarations.class_t ) and decl.indexing_suite:
            included_decls = decl.decls( lambda d: d.ignore==False, allow_empty=True, recursive=True )
            for decl in included_decls: self.add_exported(decl)

    def __is_std_decl( self, decl ):
        #Every class under std should be exported by Boost.Python and\\or `Py++`
        #Also this is not the case right now, I prefer to hide the warnings
        dpath = declarations.declaration_path( decl )
        if len( dpath ) < 3:
            return False
        if dpath[1] != 'std':
            return False
        if decl.name.startswith( 'pair<' ):
            #special case
            return False
        return True

    def __build_dependencies( self, decl ):
        if self.__is_std_decl( decl ):
            #TODO add element_type to the list of dependencies
            return [] #std declarations should be exported by `Py++`!
        if decl.already_exposed:
            return []
        dependencies = decl.i_depend_on_them(recursive=False)

        if isinstance( decl, declarations.class_t ):
            dependencies = [d for d in dependencies if d.access_type != declarations.ACCESS_TYPES.PRIVATE]

        return dependencies

    def __has_unexposed_dependency( self, exported_ids, depend_on_decl, dependency ):
        sptr_traits = declarations.smart_pointer_traits

        if None is depend_on_decl:
            return

        if self.__is_std_decl( depend_on_decl ):
            return

        if sptr_traits.is_smart_pointer( depend_on_decl ):
            try:
                value_type = sptr_traits.value_type( depend_on_decl )
                if isinstance( value_type, declarations.type_t ):
                    value_type = declarations.remove_cv( value_type )
                    value_type = declarations.remove_declarated( value_type )
                if isinstance( value_type, declarations.declaration_t ):
                    return self.__has_unexposed_dependency( exported_ids, value_type, dependency )
            except RuntimeError:
                pass

        if isinstance( depend_on_decl, decl_wrappers.decl_wrapper_t ):
            if depend_on_decl.already_exposed:
                return
            if isinstance( depend_on_decl, declarations.class_types ):
                if depend_on_decl.opaque:
                    return
                if dependency.hint == "base class":
                    return #base class for some class don't have to be exported
            if isinstance( depend_on_decl, declarations.variable_t ):
                if not decl.expose_value:
                    return

        if isinstance( dependency.decl, declarations.variable_t ):
            #the only dependency of the variable is its type
            if not dependency.decl.expose_value:
                return

        if dependency.hint == "return type":
            #in this case we don't check, the return type but the function
            if isinstance( dependency.decl, declarations.calldef_t ):
                if dependency.decl.return_type and dependency.decl.call_policies \
                   and decl_wrappers.is_return_opaque_pointer_policy( dependency.decl.call_policies ):
                   return

        return id( depend_on_decl ) not in exported_ids

    def __find_out_used_but_not_exported( self ):
        used_not_exported = []
        exported_ids = set( [id( d ) for d in self.__exported_decls] )
        for decl in self.__exported_decls:
            for dependency in self.__build_dependencies( decl ):
                for depend_on_decl in dependency.find_out_depend_on_it_declarations():
                    if self.__has_unexposed_dependency( exported_ids, depend_on_decl, dependency ):
                        if messages.filter_disabled_msgs([messages.W1040], depend_on_decl.disabled_messages ):
                            #need to report dependency errors
                            used_not_exported.append(dependency)
        return used_not_exported

    def __group_by_unexposed( self, dependencies ):
        groups = {}
        for dependency in dependencies:
            for depend_on_decl in dependency.find_out_depend_on_it_declarations():
                if id( depend_on_decl ) not in groups:
                    groups[ id( depend_on_decl ) ] = []
                groups[ id( depend_on_decl ) ].append( dependency )
        return groups

    def __create_dependencies_msg( self, dependencies ):
        msg = []
        for depend_on_decl in dependencies[0].find_out_depend_on_it_declarations():
            decls = []
            for dependency in dependencies:
                decls.append( os.linesep + ' ' + str( dependency.declaration ) )
            msg.append( "%s;%s" % ( depend_on_decl, messages.W1040 % ''.join( decls ) ) )
        return os.linesep.join( msg )

    def __report_duplicated_aliases( self ):
        decls = [decl for decl in self.__exported_decls if isinstance( decl, declarations.class_types ) \
                                     and isinstance( decl.parent, declarations.namespace_t )]

        dar = duplicated_aliases_reporter( decls )
        dar.report( self.__logger )

        classes = [c for c in decls if isinstance( c, declarations.class_t )]
        query = lambda decl: isinstance( decl, declarations.class_types ) \
                             and decl.ignore == False \
                             and decl._already_exposed == False

        for cls in classes:
            internal_decls = cls.decls( query, recursive=False, allow_empty=True)
            dar = duplicated_aliases_reporter( internal_decls )
            dar.report( self.__logger )

    def __report_duplicated_wrapper_aliases( self ):
        decls = [decl for decl in self.__exported_decls if isinstance( decl, declarations.class_t ) \
                                     and isinstance( decl.parent, declarations.namespace_t )]

        dwar = duplicated_wrapper_aliases_reporter( decls )
        dwar.report( self.__logger )

        query = lambda decl: decl.ignore == False and decl._already_exposed == False

        for cls in decls:
            internal_decls = cls.classes( query, recursive=False, allow_empty=True)
            dwar = duplicated_wrapper_aliases_reporter( internal_decls )
            dwar.report( self.__logger )

    def inform_user( self ):
        used_not_exported_decls = self.__find_out_used_but_not_exported()
        groups = self.__group_by_unexposed( used_not_exported_decls )
        for group in groups.values():
            self.__logger.warn( self.__create_dependencies_msg( group ) )
        self.__report_duplicated_aliases()
        self.__report_duplicated_wrapper_aliases()
