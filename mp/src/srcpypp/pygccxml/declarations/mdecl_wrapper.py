# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines class :class:`mdecl_wrapper_t` that allows to work on set of declarations,
as it was one declaration.

The :class:`class <mdecl_wrapper_t>` allows user to not write "for" loops within the code.
"""

import os

class call_redirector_t( object ):
    """Internal class used to call some function of objects"""
    def __init__( self, name, decls ):
        """creates call_redirector_t instance.

        :param name: name of method, to be called on every object in the `decls` list
        :param decls: list of objects
        """
        object.__init__( self )
        self.name = name
        self.decls = decls

    def __call__( self, *arguments, **keywords ):
        """calls method :attr:`call_redirector_t.name` on every object within the :attr:`call_redirector_t.decls` list"""
        for d in self.decls:
            callable_ = getattr(d, self.name)
            callable_( *arguments, **keywords )

class mdecl_wrapper_t( object ):
    """
    multiple declarations class wrapper

    The main purpose of this class is to allow an user to work on many declarations,
    as they were only one single declaration.

    For example, instead of writing `for` loop like the following

    .. code-block:: python

       for c in global_namespace.classes():
           c.compiler = "GCCXML 1.127"

    you can write:

    .. code-block:: python

       global_namespace.classes().compiler = "GCCXML 1.127"

    The same functionality could be applied on "set" methods too.
    """

    def __init__( self, decls ):
        """:param decls: list of declarations to operate on.
        :type decls: list of :class:`declaration wrappers <decl_wrapper_t>`
        """
        object.__init__( self )
        self.__dict__['declarations'] = decls

    def __bool__( self ):
        return bool( self.declarations )

    def __len__( self ):
        """returns the number of declarations"""
        return len( self.declarations )

    def __getitem__( self, index ):
        """provides access to declaration"""
        return self.declarations[index]

    def __iter__( self ):
        return iter(self.declarations)

    def __ensure_attribute( self, name ):
        invalid_decls = [d for d in self.declarations if not hasattr( d, name )]
        sep = os.linesep + '    '
        if invalid_decls:
            raise RuntimeError( "Next declarations don't have '%s' attribute: %s"
                                % ( name, sep.join( map( str, invalid_decls ) ) ) )

    def __setattr__( self, name, value ):
        """Updates the value of attribute on all declarations.
        :param name: name of attribute
        :param value: new value of attribute
        """
        self.__ensure_attribute( name )
        for d in self.declarations:
            setattr( d, name, value )

    def __getattr__( self, name ):
        """:param name: name of method
        """
        return call_redirector_t( name, self.declarations )

    def __contains__( self, item ):
        return item in self.declarations

    def to_list(self):
        l = []
        for d in self.declarations:
            l.append( d )
        return l
