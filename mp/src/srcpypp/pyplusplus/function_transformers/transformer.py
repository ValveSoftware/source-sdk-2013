# Copyright 2006 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines :class:transformer_t class"""

import sys, os.path, copy, re, types
from pygccxml import declarations, parser

return_ = -1
#return_ is a spacial const, which represent an index of return type

class transformer_t(object):
    """Base class for a function transformer."""

    USE_1_BASED_INDEXING = False

    def __init__(self, function):
        """:param function: reference to function declaration"""
        object.__init__( self )
        self.__function = function

    @property
    def function( self ):
        """reference to the function, for which a wrapper will be generated"""
        return self.__function

    def required_headers( self ):
        """Returns list of header files that transformer generated code depends on."""
        raise NotImplementedError( self.__class__.__name__ )

    def get_argument( self, reference ):
        """returns reference to the desired argument

        :param reference: name( str ) or index( int ) of the argument
        """
        if isinstance( reference, str ):
            found = [arg for arg in self.function.arguments if arg.name == reference]
            if len( found ) == 1:
                return found[0]
            raise RuntimeError( "Argument with name \"%s\" was not found" % reference )
        else:
           assert isinstance( reference, int )
           if transformer_t.USE_1_BASED_INDEXING:
               reference += 1
           return self.function.arguments[ reference ]

    def get_type( self, reference ):
        """returns type of the desired argument or return type of the function

        :param reference: name( str ) or index( int ) of the argument
        """
        global return_
        if isinstance( reference, int ) and reference == return_:
            return self.function.return_type
        else:
            return self.get_argument( reference ).type

    def configure_mem_fun( self, controller ):
        """Transformers should override the method, in order to define custom
        transformation for non-virtual member function.

        :param controller: instance of :class:`mem_fun_controller_t` class
        """
        raise NotImplementedError(self.__class__.__name__)

    def configure_free_fun( self, controller ):
        """
        transformers should override the method, in order to define custom
        transformation for free function.

        :param controller: instance of :class:`free_fun_controller_t` class
        """
        raise NotImplementedError(self.__class__.__name__)

    def configure_virtual_mem_fun( self, controller ):
        """Transformers should override the method, in order to define custom
        transformation for virtual member function.

        :param controller: instance of :class:`virtual_mem_fun_controller_t` class
        """
        raise NotImplementedError(self.__class__.__name__)

#TODO: FT for constructor
    #~ def configure_constructor( self, controller ):
        #~ """Transformers should override the method, in order to define custom
        #~ transformation for constructor.

        #~ :param controller: instance of L{constructor_controller_t} class
        #~ """
        #~ raise NotImplementedError(self.__class__.__name__)

