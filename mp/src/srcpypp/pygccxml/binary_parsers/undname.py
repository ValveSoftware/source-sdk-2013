# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
provides low-level functionality, needed to undecorate\demangle compiler generated
unique names and map them to the declarations

On Windows:
    ctypes package is used to call `UnDecorateSymbolName` function from `dbghelp.dll`

On Linux:
    "nm" utility is used.
"""

import os
import re
import sys
import ctypes
from .. import declarations

class UNDECORATE_NAME_OPTIONS:
    """defines few constants for `UnDecorateSymbolName` function"""

    UNDNAME_COMPLETE = 0x0000 #Enables full undecoration.
    UNDNAME_NO_LEADING_UNDERSCORES = 0x0001 #Removes leading underscores from Microsoft extended keywords.
    UNDNAME_NO_MS_KEYWORDS = 0x0002 #Disables expansion of Microsoft extended keywords.
    UNDNAME_NO_FUNCTION_RETURNS = 0x0004 #Disables expansion of return type for primary declaration.
    UNDNAME_NO_ALLOCATION_MODEL = 0x0008 #Disables expansion of the declaration model.
    UNDNAME_NO_ALLOCATION_LANGUAGE = 0x0010 #Disables expansion of the declaration language specifier.
    UNDNAME_RESERVED1 = 0x0020 #RESERVED.
    UNDNAME_RESERVED2 = 0x0040 #RESERVED.
    UNDNAME_NO_THISTYPE = 0x0060 #Disables all modifiers on the this type.
    UNDNAME_NO_ACCESS_SPECIFIERS = 0x0080 #Disables expansion of access specifiers for members.
    UNDNAME_NO_THROW_SIGNATURES = 0x0100 #Disables expansion of "throw-signatures" for functions and pointers to functions.
    UNDNAME_NO_MEMBER_TYPE = 0x0200 #Disables expansion of static or virtual members.
    UNDNAME_NO_RETURN_UDT_MODEL = 0x0400 #Disables expansion of the Microsoft model for UDT returns.
    UNDNAME_32_BIT_DECODE = 0x0800 #Undecorates 32-bit decorated names.
    UNDNAME_NAME_ONLY = 0x1000 #Gets only the name for primary declaration; returns just [scope::]name. Expands template params.
    UNDNAME_TYPE_ONLY = 0x2000 #Input is just a type encoding; composes an abstract declarator.
    UNDNAME_HAVE_PARAMETERS = 0x4000 #The real template parameters are available.
    UNDNAME_NO_ECSU = 0x8000 #Suppresses enum/class/struct/union.
    UNDNAME_NO_IDENT_CHAR_CHECK = 0x10000 #Suppresses check for valid identifier characters.
    UNDNAME_NO_PTR64 = 0x20000 #Does not include ptr64 in output.

    UNDNAME_SCOPES_ONLY = UNDNAME_NO_LEADING_UNDERSCORES \
                          | UNDNAME_NO_MS_KEYWORDS \
                          | UNDNAME_NO_FUNCTION_RETURNS \
                          | UNDNAME_NO_ALLOCATION_MODEL \
                          | UNDNAME_NO_ALLOCATION_LANGUAGE \
                          | UNDNAME_NO_ACCESS_SPECIFIERS \
                          | UNDNAME_NO_THROW_SIGNATURES \
                          | UNDNAME_NO_MEMBER_TYPE \
                          | UNDNAME_NO_ECSU \
                          | UNDNAME_NO_IDENT_CHAR_CHECK

    SHORT_UNIQUE_NAME = UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_ECSU

class undname_creator_t:
    """implementation details - should not be used directly

    formats declarations string representation and exported symbols, so they
    could be matched later.

    The class formats variables, free and member functions, symbols exported from
    .dll, .map and .so files.

    On Windows, the class works with unique name produced by MSVC compiler and
    with undecorated names produced by `dbghelp.dll`

    On Linux, the class works with mangled names produced by GCC-XML ( GCC 4.2 )
    compiler and demangled name produced by "nm" utility.
    """
    def __init__( self ):
        if 'nt' == os.name:
            import ctypes.wintypes
            self.__undname = ctypes.windll.dbghelp.UnDecorateSymbolName
            self.__undname.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint]

        self.__clean_ecsu = ( re.compile( r'(?P<startswith>^|\W)(?:(class|enum|struct|union)\s)' )
                              , '%(startswith)s' )
        self.__fundamental_types = (
              ( 'short unsigned int', 'unsigned short')
            , ( 'short int', 'short' )
            , ( 'long int', 'long' )
            , ( 'long unsigned int', 'unsigned long' )
        )
        self.__calling_conventions = ( re.compile( r'(?P<startswith>^|\s)(?:__(cdecl|clrcall|stdcall|fastcall|thiscall)\s)' )
                                       , '%(startswith)s' )

    def normalize_undecorated( self, undname, options=None ):
        if options is None:
            options = UNDECORATE_NAME_OPTIONS.SHORT_UNIQUE_NAME
        if UNDECORATE_NAME_OPTIONS.UNDNAME_NO_ECSU & options:
            expr, substitute = self.__clean_ecsu
            undname = expr.sub( lambda m: substitute % m.groupdict(), undname )
        if UNDECORATE_NAME_OPTIONS.UNDNAME_NO_ACCESS_SPECIFIERS & options:
            for prefix in ( 'public: ', 'private: ', 'protected: ' ):
                if undname.startswith( prefix ):
                    undname = undname[ len(prefix): ]
                    break
        if UNDECORATE_NAME_OPTIONS.UNDNAME_NO_MS_KEYWORDS & options:
            expr, substitute = self.__calling_conventions
            undname = expr.sub( lambda m: substitute % m.groupdict(), undname )
        return undname.strip()

    def undecorate_blob( self, name, options=None ):
        if options is None:
            options = UNDECORATE_NAME_OPTIONS.SHORT_UNIQUE_NAME
        buffer = ctypes.create_string_buffer(1024*16)
        res = self.__undname( str(name), buffer, ctypes.sizeof(buffer), options)
        if res:
            return self.normalize_undecorated( str(buffer[:res]) )
        else:
            return name

    def __remove_leading_scope( self, s ):
        if s and s.startswith( '::' ):
            return s[2:]
        else:
            return s

    def __format_type_as_undecorated( self, type_, is_argument, hint ):
        result = []
        type_ = declarations.remove_alias( type_ )
        if declarations.is_array( type_ ):
            result.append( declarations.array_item_type( type_ ).decl_string )
            result.append( '*' )
            if is_argument:
                result.append( 'const' )
        else:
            result.append( self.__remove_leading_scope( type_.decl_string ) )

        result = ' '.join( result )
        if hint == 'nm':
            for x in ( '*', '&' ):
                result = result.replace( ' ' + x, x )
        return result

    def __normalize( self, name ):
        for what, with_ in self.__fundamental_types:
            name = name.replace( what, with_ )
        name = name.replace( ', ', ',' )
        return name

    def format_argtypes( self, argtypes, hint ):
        if not argtypes:
            if hint == 'msvc':
                return 'void'
            else:
                return ''
        else:
            formater = lambda type_: self.__format_type_as_undecorated( type_, True, hint )
            argsep =','
            if hint == 'nm':
                argsep = ',  ' #ugly hack, later, I will replace ', ' with ',', so single space will still exist
            return argsep.join( map( formater, argtypes ) )

    def format_calldef( self, calldef, hint ):
        calldef_type = calldef.function_type()
        result = []
        is_mem_fun = isinstance( calldef, declarations.member_calldef_t )
        if is_mem_fun and hint == 'msvc' and calldef.virtuality != declarations.VIRTUALITY_TYPES.NOT_VIRTUAL:
            result.append( 'virtual ' )
        if is_mem_fun and hint == 'msvc'and calldef.has_static:
            result.append( 'static ' )
        if  hint == 'msvc' and calldef_type.return_type:
            #nm doesn't dump return type information
            result.append( self.__format_type_as_undecorated( calldef.return_type, False, hint ) )
            result.append( ' ' )
        if is_mem_fun:
            result.append( self.__remove_leading_scope( calldef.parent.decl_string ) + '::')

        result.append( calldef.name )
        if isinstance( calldef, ( declarations.constructor_t, declarations.destructor_t) ) \
           and declarations.templates.is_instantiation( calldef.parent.name ):
            if hint == 'msvc':
                result.append( '<%s>' % ','.join( declarations.templates.args( calldef.parent.name ) ) )

        result.append( '(%s)' % self.format_argtypes( calldef_type.arguments_types, hint ) )
        if is_mem_fun and calldef.has_const:
            if hint == 'nm':
                result.append( ' ' )
            result.append( 'const' )
        return ''.join( result )

    def format_var( self, decl, hint ):
        result = []
        is_mem_var = isinstance( decl.parent, declarations.class_t )
        if is_mem_var and decl.type_qualifiers.has_static and hint == 'msvc':
            result.append( 'static ' )
        if hint == 'msvc':
            result.append( self.__format_type_as_undecorated( decl.type, False, hint ) )
            result.append( ' ' )
        if is_mem_var:
            result.append( self.__remove_leading_scope( decl.parent.decl_string ) + '::' )
        result.append( decl.name )
        return ''.join( result )

    def format_decl(self, decl, hint=None):
        """returns string, which contains full function name formatted exactly as
        result of `dbghelp.UnDecorateSymbolName`, with UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_ECSU
        options.

        Different compilers/utilities undecorate/demangle mangled string ( unique names ) in a different way.
        `hint` argument will tell pygccxml how to format declarations, so they could be mapped later to the blobs.
        The valid options are: "msvc" and "nm".
        """
        name = None
        if hint is None:
            if 'nt' == os.name:
                hint = 'msvc'
            else:
                hint = 'nm'

        if isinstance( decl, declarations.calldef_t ):
            name = self.format_calldef( decl, hint )
        elif isinstance( decl, declarations.variable_t ):
            name = self.format_var( decl, hint )
        else:
            raise NotImplementedError()
        return self.__normalize( name )
