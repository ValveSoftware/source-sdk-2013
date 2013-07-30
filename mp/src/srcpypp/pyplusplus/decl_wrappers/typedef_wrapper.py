# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class that configure typedef exposing"""

from pygccxml import declarations
from . import decl_wrapper

class typedef_t(decl_wrapper.decl_wrapper_t, declarations.typedef_t):
    """defines a set of properties, that will instruct `Py++` how to expose the typedef

    Today, `Py++` does not exposes typedefs, but this could be changed in future.
    In C++, it is a common practices to give an aliases to the class. May be in
    future, `Py++` will generate code, that will register all those aliases.
    """

    def __init__(self, *arguments, **keywords):
        declarations.typedef_t.__init__(self, *arguments, **keywords )
        decl_wrapper.decl_wrapper_t.__init__( self )
        self.__is_directive = None

    @property
    def is_directive( self ):
        if None is self.__is_directive:
            dpath = declarations.declaration_path( self )
            if len( dpath ) != 4:
                self.__is_directive = False
            else:
                self.__is_directive = dpath[:3] == ['::', 'pyplusplus', 'aliases']
        return self.__is_directive
