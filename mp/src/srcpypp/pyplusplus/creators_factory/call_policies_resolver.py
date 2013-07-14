# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from . import opaque_types_manager
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators
from pyplusplus.decl_wrappers import python_traits

#TODO: add opaque to documentation

class resolver_t( object ):
    def __init__( self ):
        object.__init__( self )

    def __call__(self, decl, hint=None):
        raise NotImplementedError()

class default_policy_resolver_t(resolver_t):
    def __init__( self ):
        resolver_t.__init__( self )
        self.__const_char_pointer \
            = declarations.pointer_t( declarations.const_t( declarations.char_t() ) )

    def _resolve_by_type( self, some_type ):
        temp_type = declarations.remove_alias( some_type )
        temp_type = declarations.remove_cv( temp_type )
        if isinstance( temp_type, declarations.fundamental_t ) \
           or isinstance( temp_type, declarations.declarated_t ):
            return decl_wrappers.default_call_policies()
        if declarations.is_same( some_type, self.__const_char_pointer ):
            return decl_wrappers.default_call_policies()
        return None

    def __call__(self, calldef, hint=None):
        if not isinstance( calldef, declarations.calldef_t ):
            return None
            
        if not isinstance( calldef, declarations.constructor_t ):
            return self._resolve_by_type( calldef.return_type )
        else:
            for arg in calldef.arguments:
                if not self._resolve_by_type( arg.type ):
                    return None
            return decl_wrappers.default_call_policies()

class void_pointer_resolver_t(resolver_t):
    def __init__( self ):
        resolver_t.__init__( self )

    def __call__( self, calldef, hint=None ):
        if not isinstance( calldef, declarations.calldef_t ):
            return None

        if isinstance( calldef, declarations.constructor_t ):
            return None
        return_type = declarations.remove_alias( calldef.return_type )
        void_ptr = declarations.pointer_t( declarations.void_t() )
        const_void_ptr = declarations.pointer_t( declarations.const_t( declarations.void_t() ) )
        if declarations.is_same( return_type, void_ptr ) \
           or declarations.is_same( return_type, const_void_ptr ):
            return decl_wrappers.return_value_policy( decl_wrappers.return_opaque_pointer )
        return None

class return_value_policy_resolver_t(resolver_t):
    def __init__( self ):
        resolver_t.__init__( self )
        self.__const_wchar_pointer \
            = declarations.pointer_t( declarations.const_t( declarations.wchar_t() ) )
        
    def __call__(self, calldef, hint=None):
        if not isinstance( calldef, declarations.calldef_t ):
            return None

        if isinstance( calldef, declarations.constructor_t ):
            return None

        return_type = declarations.remove_alias( calldef.return_type )
        if isinstance( return_type, declarations.reference_t ) \
           and isinstance( return_type.base, declarations.const_t ):
            return decl_wrappers.return_value_policy( decl_wrappers.copy_const_reference )

        if declarations.is_same( return_type, self.__const_wchar_pointer ):
            return decl_wrappers.return_value_policy( decl_wrappers.return_by_value )
        
        if opaque_types_manager.find_out_opaque_decl( return_type, ensure_opaque_decl=True ):
            return decl_wrappers.return_value_policy( decl_wrappers.return_opaque_pointer )
            
        return None
        
class return_internal_reference_resolver_t( resolver_t ):
    def __init__( self ):    
        resolver_t.__init__( self )
        
    def __call__(self, calldef, hint=None):
        if not isinstance( calldef, declarations.calldef_t ):
            return None

        if not isinstance( calldef, declarations.member_operator_t ):
            return None
        
        if calldef.symbol != '[]':
            return None
            
        return_type = declarations.remove_cv( calldef.return_type )
        if declarations.is_reference( return_type ): 
            return_type = declarations.remove_reference( return_type )
        if python_traits.is_immutable( return_type ):
            if declarations.is_const( calldef.return_type ):
                return decl_wrappers.return_value_policy( decl_wrappers.copy_const_reference )
            else:
                return decl_wrappers.return_value_policy( decl_wrappers.copy_non_const_reference )
        else:
            return decl_wrappers.return_internal_reference()

class return_self_resolver_t( resolver_t ):
    def __init__( self ):    
        resolver_t.__init__( self )
        
    def __call__(self, calldef, hint=None):
        if not isinstance( calldef, declarations.member_operator_t ):
            return None
        
        if calldef.symbol != '=':
            return None

        return decl_wrappers.return_self()

class variable_accessors_resolver_t( resolver_t ):
    def __init__( self ):    
        resolver_t.__init__( self )
    
    def __call__( self, variable, hint=None ):
        if not isinstance( variable, declarations.variable_t ):
            return None

        assert hint in ( 'get', 'set' )
        
        if not declarations.is_reference( variable.type ):
            return None
        
        no_ref = declarations.remove_reference( variable.type )
        base_type = declarations.remove_const( no_ref )
        if python_traits.is_immutable( base_type ):
            #the relevant code creator will generate code, that will return this member variable
            #by value
            return decl_wrappers.default_call_policies()
        
        if not isinstance( base_type, declarations.declarated_t ):
            return None
        
        base_type = declarations.remove_alias( base_type )
        decl = base_type.declaration
        
        if declarations.is_class_declaration( decl ):
            return None
        
        if decl.is_abstract:
            return None
        if declarations.has_destructor( decl ) and not declarations.has_public_destructor( decl ): 
            return None
        if not declarations.has_copy_constructor(decl):
            return None
        if hint == 'get':
            #if we rich this line, it means that we are able to create an obect using
            #copy constructor. 
            if declarations.is_const( no_ref ):
                return decl_wrappers.return_value_policy( decl_wrappers.copy_const_reference )
            else:
                return decl_wrappers.return_value_policy( decl_wrappers.copy_non_const_reference )
        else:
            return decl_wrappers.default_call_policies()

class built_in_resolver_t(resolver_t):
    def __init__( self, config=None):
        resolver_t.__init__( self )
        self.__resolvers = [ default_policy_resolver_t()
                             , return_value_policy_resolver_t() ]        
        assert not config or isinstance( config, code_creators.target_configuration_t )
        if not config or config.boost_python_supports_void_ptr:
            self.__resolvers.append( void_pointer_resolver_t() )
        self.__resolvers.append( return_internal_reference_resolver_t() )
        self.__resolvers.append( variable_accessors_resolver_t() )
        self.__resolvers.append( return_self_resolver_t() )        

    def __call__( self, calldef, hint=None ):
        for resolver in self.__resolvers:
            resolved = resolver( calldef, hint )
            if resolved:
                return resolved
        return None
