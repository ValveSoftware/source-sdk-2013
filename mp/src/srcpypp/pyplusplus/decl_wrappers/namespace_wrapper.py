# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class that configure namespace exposing."""

from . import scopedef_wrapper
from pygccxml import declarations

class namespace_t(scopedef_wrapper.scopedef_t, declarations.namespace_t):
    """defines a set of properties, that will instruct `Py++` how to expose the namespace

    Today, `Py++` does not exposes namespaces, but this could be changed in future.
    The future direction I see here, is to expose every namespace as sub-module
    of the main one.
    """
    def __init__(self, *arguments, **keywords):
        scopedef_wrapper.scopedef_t.__init__( self )
        declarations.namespace_t.__init__(self, *arguments, **keywords )
