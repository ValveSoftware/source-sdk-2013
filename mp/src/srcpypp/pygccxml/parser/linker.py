# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import types
from pygccxml.declarations import *

class linker_t( decl_visitor_t, type_visitor_t, object ):
    def __init__( self, decls, types, access, membership, files ):
        decl_visitor_t.__init__(self)
        type_visitor_t.__init__(self)
        object.__init__(self)

        self.__decls = decls
        self.__types = types
        self.__access = access
        self.__membership = membership
        self.__files = files
        self.__inst = None

        self.__compiler = None
        if self.__decls:
            for d in self.__decls.values():
                self.__compiler = d.compiler
                break

    def _get_inst(self):
        return self.__inst
    def _set_inst(self, inst):
        self.__inst = inst
        #use inst, to reduce attribute access time
        if isinstance( inst, declaration_t ) and inst.location:
            inst.location.file_name = self.__files[inst.location.file_name]
    instance = property( _get_inst, _set_inst )

    def __link_type(self, type_id):
        if type_id is None:
            return None #in some situations type_id is None, return_type of constructor or destructor
        elif type_id in self.__types:
            return self.__types[type_id]
        elif type_id in self.__decls:
            base = declarated_t( declaration=self.__decls[ type_id ] )
            self.__types[type_id] = base
            return base
        elif '...' == type_id:
            return ellipsis_t()
        else:
            return unknown_t()

    def __link_compound_type(self):
        self.__inst.base = self.__link_type( self.__inst.base )

    def __link_members(self):
        if id(self.__inst) not in self.__membership:
            return
        for member in self.__membership[ id(self.__inst) ]:
            if member not in self.__access:
                continue
            access = self.__access[member]
            if member not in self.__decls:
                continue
            decl = self.__decls[member]
            if isinstance( self.__inst, class_t ):
                self.__inst.adopt_declaration( decl, access )
            else:
                self.__inst.adopt_declaration( decl )

    def __link_calldef(self):
        self.__inst.return_type = self.__link_type( self.__inst.return_type )
        if isinstance( self.__inst, type_t ):
            linked_args = [ self.__link_type( arg ) for arg in self.__inst.arguments_types ]
            self.__inst.arguments_types = linked_args
        else:
            for arg in self.__inst.arguments:
                arg.type = self.__link_type(arg.type)
            for index in range( len( self.__inst.exceptions ) ):
                try:
                    self.__inst.exceptions[index] = self.__decls[ self.__inst.exceptions[index] ]
                except KeyError:
                    self.__inst.exceptions[index] = self.__link_type( self.__inst.exceptions[index] )

    def visit_member_function( self ):
        self.__link_calldef()

    def visit_constructor( self ):
        self.__link_calldef()

    def visit_destructor( self ):
        self.__link_calldef()

    def visit_member_operator( self ):
        self.__link_calldef()

    def visit_casting_operator( self ):
        self.__link_calldef()
        #will be fixed by patcher. It is needed because of demangled name taken into account
        #self.__inst._name = 'operator ' + self.__inst.return_type.decl_string

    def visit_free_function( self ):
        self.__link_calldef()

    def visit_free_operator( self ):
        self.__link_calldef()

    def visit_class_declaration(self ):
        pass

    def visit_class(self ):
        self.__link_members()
        #GCC-XML sometimes generates constructors with names that does not match
        #class name. I think this is because those constructors are compiler
        #generated. I need to find out more about this and to talk with Brad

        new_name = self.__inst._name
        if templates.is_instantiation( new_name ):
            new_name = templates.name( new_name )

        for decl in self.__inst.declarations:
            if not isinstance( decl, constructor_t ):
                continue
            if '.' in decl._name or '$' in decl._name:
                decl._name = new_name

        bases = self.__inst.bases.split()
        self.__inst.bases = []
        for base in bases:
            #it could be "_5" or "protected:_5"
            data = base.split(':')
            base_decl = self.__decls[ data[-1] ]
            access = ACCESS_TYPES.PUBLIC
            if 2 == len( data ):
                access = data[0]
            self.__inst.bases.append( hierarchy_info_t( base_decl, access ) )
            base_decl.derived.append( hierarchy_info_t( self.__inst, access ) )

    def visit_enumeration(self ):
        pass

    def visit_namespace(self ):
        self.__link_members()

    def visit_typedef(self ):
        self.__inst.type = self.__link_type(self.__inst.type)

    def visit_variable(self ):
        self.__inst.type = self.__link_type(self.__inst.type)

    def visit_void( self ):
        pass

    def visit_char( self ):
        pass

    def visit_signed_char( self ):
        pass

    def visit_unsigned_char( self ):
        pass

    def visit_wchar( self ):
        pass

    def visit_short_int( self ):
        pass

    def visit_short_unsigned_int( self ):
        pass

    def visit_bool( self ):
        pass

    def visit_int( self ):
        pass

    def visit_unsigned_int( self ):
        pass

    def visit_long_int( self ):
        pass

    def visit_long_unsigned_int( self ):
        pass

    def visit_long_long_int( self ):
        pass

    def visit_long_long_unsigned_int( self ):
        pass

    def visit_int128( self ):
        pass

    def visit_uint128( self ):
        pass

    def visit_float( self ):
        pass

    def visit_double( self ):
        pass

    def visit_long_double( self ):
        pass

    def visit_complex_long_double(self):
        pass

    def visit_complex_double(self):
        pass

    def visit_complex_float(self):
        pass

    def visit_jbyte(self):
        pass

    def visit_jshort(self):
        pass

    def visit_jint(self):
        pass

    def visit_jlong(self):
        pass

    def visit_jfloat(self):
        pass

    def visit_jdouble(self):
        pass

    def visit_jchar(self):
        pass

    def visit_jboolean(self):
        pass

    def visit_volatile( self ):
        if isinstance( self.__inst.base, const_t ):
            const_type_inst = self.__inst.base
            const_type_inst.base = self.__link_type( const_type_inst.base )
        else:
            self.__link_compound_type()

    def visit_const( self ):
        self.__link_compound_type()

    def visit_pointer( self ):
        if '0.9' in self.__compiler and isinstance( self.__inst.base, member_variable_type_t ):
            original_inst = self.__inst
            self.__inst = self.__inst.base
            self.visit_member_variable_type()
            self.__inst = original_inst
        else:
            self.__link_compound_type()

    def visit_reference( self ):
        self.__link_compound_type()

    def visit_array( self ):
        self.__link_compound_type()

    def visit_free_function_type( self ):
        self.__link_calldef()

    def visit_member_function_type( self ):
        self.__link_calldef()
        if isinstance( self.__inst, type_t ):
            self.__inst.class_inst = self.__link_type( self.__inst.class_inst )

    def visit_member_variable_type( self ):
        self.__inst.variable_type = self.__link_type( self.__inst.variable_type )
        self.__link_compound_type()

    def visit_declarated( self ):
        if isinstance( self.__inst.declaration, str ):
            self.__inst.declaration = self.__decls[self.__inst.declaration]

    def visit_restrict( self ):
        self.__link_compound_type()

