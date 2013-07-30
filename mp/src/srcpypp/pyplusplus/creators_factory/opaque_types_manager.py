# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from pygccxml import declarations
from pyplusplus import code_creators
from pyplusplus import decl_wrappers

def find_out_opaque_decl( type_, ensure_opaque_decl ):
    naked_type = declarations.remove_cv( type_ )
    if not declarations.is_pointer( naked_type ):
        return None
    naked_type = declarations.remove_pointer( declarations.remove_cv( type_ ) )
    if decl_wrappers.python_traits.is_immutable( naked_type ):
        return None#immutable types could not be opaque
    decl = None
    if declarations.is_class( naked_type ):
        decl = declarations.class_traits.get_declaration( naked_type )
    elif declarations.is_class_declaration( naked_type ):#class declaration:
        decl = declarations.class_declaration_traits.get_declaration( naked_type )
    else:
        return None
    if ensure_opaque_decl:
        if decl.opaque:
            return decl
        else:
            return None
    else:
        return decl


class manager_t( object ):
    def __init__( self, extmodule ):
        object.__init__( self )
        self.__extmodule = extmodule
        self.__exposed_opaque_decls = {} #decl: creator
        
    def __find_out_opaque_decls( self, decl ):
        opaque_types = []
        is_opaque_policy = decl_wrappers.is_return_opaque_pointer_policy
        if isinstance( decl, declarations.variable_t ):
            opaque_decl = find_out_opaque_decl( decl.type, ensure_opaque_decl=True )
            if opaque_decl:
                opaque_types.append( opaque_decl )
            elif is_opaque_policy( decl.getter_call_policies ) or is_opaque_policy( decl.setter_call_policies ):
                opaque_decl = find_out_opaque_decl( decl.type, ensure_opaque_decl=False )
                if opaque_decl:
                    opaque_types.append( opaque_decl )
            else:
                pass
        elif isinstance( decl, declarations.calldef_t ):
            if is_opaque_policy( decl.call_policies ):
                opaque_decl = find_out_opaque_decl( decl.return_type, ensure_opaque_decl=False )
                if opaque_decl:
                    opaque_types.append( opaque_decl )
            all_types = decl.argument_types[:]
            if decl.return_type:
                all_types.append( decl.return_type )    
            for type_ in all_types:
                opaque_decl = find_out_opaque_decl( type_, ensure_opaque_decl=True )
                if opaque_decl:
                    opaque_types.append( opaque_decl )
        else:
            pass
        return opaque_types
            
            
    def register_opaque( self, creator, decl_or_decls ):
        opaque_decls = []
        for decl in declarations.make_flatten( decl_or_decls ):
            opaque_decls.extend( self.__find_out_opaque_decls( decl ) )
            
        for decl in opaque_decls:
            opaque_type_registrator = None
            if id(decl) not in list(self.__exposed_opaque_decls.keys()):
                opaque_type_registrator = code_creators.opaque_type_registrator_t( decl )
                self.__exposed_opaque_decls[ id(decl) ] = opaque_type_registrator
                self.__extmodule.adopt_declaration_creator( opaque_type_registrator )
            else:
                opaque_type_registrator = self.__exposed_opaque_decls[ id(decl) ]
            creator.associated_decl_creators.append(opaque_type_registrator)
