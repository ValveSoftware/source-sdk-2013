# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines base class for :class:`decl_wrappers.class_t` and :class:`decl_wrappers.namespace_t` classes"""

from . import decl_wrapper
from pyplusplus import messages

class scopedef_t(decl_wrapper.decl_wrapper_t):
    """base class for :class:`decl_wrappers.class_t` and :class:`decl_wrappers.namespace_t` classes

    It provides convenience functionality: include\\exclude all internal declarations
    (not) to be exported.
    """

    def __init__(self):
        decl_wrapper.decl_wrapper_t.__init__( self )

    def exclude( self, compilation_errors=False ):
        """exclude "self" and child declarations from being exposed.
        
        If compile_time_errors is True, than only declarations, which will cause
        compilation error will be excluded
        """
        if False == compilation_errors:
            #exclude all unconditionaly
            self.ignore = True
            for decl in self.declarations: decl.exclude()
        else:
            if [msg for msg in self.readme() if isinstance( msg, messages.compilation_error )]:
                self.exclude()
            else:
                for decl in self.declarations: decl.exclude(compilation_errors=True)

    def include( self, already_exposed=False  ):
        """Include "self" and child declarations to be exposed."""
        self.ignore = False
        self.already_exposed = already_exposed
        for decl in self.declarations: decl.include(already_exposed)