# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from pygccxml import utils
from pygccxml import declarations


class default_argument_patcher_t( object ):
    def __init__( self, enums ):
        object.__init__( self )
        self.__enums = enums

    def __call__(self, decl):
        for arg in decl.arguments:
            if not arg.default_value:
                continue
            fixer = self.__find_fixer( decl, arg )
            if fixer:
                arg.default_value = fixer( decl, arg )

    def __find_fixer(self, func, arg):
        if not arg.default_value:
            return False
        elif self.__is_unqualified_enum( func, arg ):
            return self.__fix_unqualified_enum
        elif self.__is_double_call( func, arg ):
            return self.__fix_double_call
        elif self.__is_invalid_integral( func, arg ):
            return self.__fix_invalid_integral
        elif self.__is_constructor_call( func, arg ):
            return self.__fix_constructor_call
        else:
            return None

    def __join_names( self, prefix, suffix ):
        if prefix == '::':
            return '::' + suffix
        else:
            return prefix + '::' + suffix

    def __is_unqualified_enum(self, func, arg):
        type_ = declarations.remove_reference( declarations.remove_cv( arg.type ) )
        if not declarations.is_enum( type_ ):
            return False
        enum_type = declarations.enum_declaration( type_ )
        return enum_type.has_value_name( arg.default_value )

    def __fix_unqualified_enum( self, func, arg):
        type_ = declarations.remove_reference( declarations.remove_cv( arg.type ) )
        enum_type = declarations.enum_declaration( type_ )
        return self.__join_names( enum_type.parent.decl_string, arg.default_value )

    def __is_invalid_integral(self, func, arg):
        type_ = declarations.remove_reference( declarations.remove_cv( arg.type ) )
        if not declarations.is_integral( type_ ):
            return False
        try:
            int( arg.default_value )
            return False
        except:
            return True

    def __fix_invalid_integral(self, func, arg):
        try:
            int( arg.default_value )
            return arg.default_value
        except:
            pass

        try:
            int( arg.default_value, 16 )
            if 64 == utils.get_architecture():
                #on 64 bit architecture, gccxml reports 0fffff, which is valid number
                #the problem is that in this case it is so buggy so pygccxml can not fix it
                #users will have to fix the default value manually
                return arg.default_value
            default_value = arg.default_value.lower()
            found_hex = [ch for ch in default_value if ch in 'abcdef']
            if found_hex and not default_value.startswith( '0x' ):
                int( '0x' + default_value, 16 )
                return '0x' + default_value
        except:
            pass

        #may be we deal with enum
        parent = func.parent
        while parent:
            found = self.__find_enum( parent, arg.default_value )
            if found:
                if declarations.is_fundamental( arg.type ) and ' ' in arg.type.decl_string:
                    template = '(%s)(%s)'
                else:
                    template = '%s(%s)'
                return template % ( arg.type.decl_string
                                    , self.__join_names( found.parent.decl_string, arg.default_value ) )
            else:
                parent = parent.parent
        return arg.default_value

    def __find_enum( self, scope, default_value ):
        #this algorithm could be improved: it could take into account
        #1. unnamed namespaced
        #2. location within files

        for enum in self.__enums:
            if enum.parent is scope and enum.has_value_name( default_value ):
                return enum
        return None

    def __is_double_call( self, func, arg ):
        call_invocation = declarations.call_invocation
        dv = arg.default_value
        found1 = call_invocation.find_args( dv )
        if found1 == call_invocation.NOT_FOUND:
            return False
        found2 = call_invocation.find_args( dv, found1[1] + 1 )
        if found2 == call_invocation.NOT_FOUND:
            return False
        args1 = call_invocation.args( dv[ found1[0] : found1[1] + 1 ] )
        args2 = call_invocation.args( dv[ found2[0] : found2[1] + 1 ] )
        return len(args1) == len(args2)

    def __fix_double_call( self, func, arg ):
        call_invocation = declarations.call_invocation
        dv = arg.default_value
        found1 = call_invocation.find_args( dv )
        found2 = call_invocation.find_args( dv, found1[1] + 1 )
        #args1 = call_invocation.args( dv[ found1[0] : found1[1] + 1 ] )
        args2 = call_invocation.args( dv[ found2[0] : found2[1] + 1 ] )
        return call_invocation.join( dv[:found1[0]], args2 )

    def __is_constructor_call( self, func, arg ):
        #if '0.9' in func.compiler:
        #    return False
        call_invocation = declarations.call_invocation
        dv = arg.default_value
        if not call_invocation.is_call_invocation( dv ):
            return False
        name = call_invocation.name( dv )
        base_type = declarations.base_type( arg.type )
        if not isinstance( base_type, declarations.declarated_t ):
            return False
        decl = base_type.declaration
        return decl.name == name \
               or ( isinstance( decl, declarations.class_t ) \
                    and name in [typedef.name for typedef in decl.aliases] )

    def __fix_constructor_call( self, func, arg ):
        call_invocation = declarations.call_invocation
        dv = arg.default_value
        if not call_invocation.is_call_invocation( dv ):
            return False
        base_type = declarations.base_type( arg.type )
        decl = base_type.declaration
        name, args = call_invocation.split( dv )
        if decl.name != name:
            #we have some alias to the class
            relevant_typedefs = [typedef for typedef in decl.aliases if typedef.name == name]
            if 1 == len( relevant_typedefs ):
                f_q_name = self.__join_names( declarations.full_name( relevant_typedefs[0].parent )
                                              , name )
            else:#in this case we can not say which typedef user uses:
                f_q_name = self.__join_names( declarations.full_name( decl.parent )
                                              , decl.name )
        else:
            f_q_name = self.__join_names( declarations.full_name( decl.parent ), name )

        return call_invocation.join( f_q_name, args )

class casting_operator_patcher_t( object ):
    def __init__( self ):
        object.__init__( self )

    def __call__(self, decl):
        decl.name = 'operator ' + decl.return_type.decl_string

_casting_oper_patcher_ = casting_operator_patcher_t()

def fix_calldef_decls(decls, enums):
    default_arg_patcher = default_argument_patcher_t(enums)
    #decls should be flat list of all declarations, you want to apply patch on
    for decl in decls:
        default_arg_patcher( decl )
        if isinstance( decl, declarations.casting_operator_t):
            _casting_oper_patcher_( decl )
