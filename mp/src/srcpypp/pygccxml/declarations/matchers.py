# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines all "built-in" classes that implement declarations compare functionality
according to some criteria
"""

import os
import re
import types
from . import algorithm
from . import variable
from . import namespace
from . import calldef
from . import cpptypes
from . import templates
from . import class_declaration
from pygccxml import utils

class matcher_base_t(object):
    """matcher_base_t class defines interface for classes that will implement
       compare functionality according to some criteria.
    """
    def __init__( self ):
        object.__init__( self )

    def __call__(self, decl):
        raise NotImplementedError( "matcher must always implement the __call__() method." )

    def __invert__(self):
        """not-operator (~)"""
        return not_matcher_t(self)

    def __and__(self, other):
        """and-operator (&)"""
        return and_matcher_t([self, other])

    def __or__(self, other):
        """or-operator (|)"""
        return or_matcher_t([self, other])

    def __str__( self ):
        return "base class for all matchers"

class and_matcher_t(matcher_base_t):
    """
    Combine several other matchers with "&" (and) operator.

    For example: find all private functions with name XXX

    .. code-block:: python

       matcher = access_type_matcher_t( 'private' ) & calldef_matcher_t( name='XXX' )
    """
    def __init__(self, matchers):
        matcher_base_t.__init__(self)
        self.matchers = matchers

    def __call__(self, decl):
        for matcher in self.matchers:
            if not matcher(decl):
                return False
        return True

    def __str__(self):
        return " & ".join( ["(%s)" % str( x ) for x in self.matchers] )


class or_matcher_t(matcher_base_t):
    """Combine several other matchers with "|" (or) operator.

    For example: find all functions and variables with name 'XXX'

    .. code-block:: python

       matcher = variable_matcher_t( name='XXX' ) | calldef_matcher_t( name='XXX' )

    """
    def __init__(self, matchers):
        matcher_base_t.__init__(self)
        self.matchers = matchers

    def __call__(self, decl):
        for matcher in self.matchers:
            if matcher(decl):
                return True
        return False

    def __str__(self):
        return " | ".join( ["(%s)" % str( x ) for x in self.matchers] )


class not_matcher_t(matcher_base_t):
    """
    return the inverse result of a matcher

    For example: find all public and protected declarations

    .. code-block:: python

       matcher = ~access_type_matcher_t( 'private' )
    """
    def __init__(self, matcher):
        matcher_base_t.__init__(self)
        self.matcher = matcher

    def __call__(self, decl):
        return not self.matcher(decl)

    def __str__(self):
        return "~(%s)"%str(self.matcher)

class declaration_matcher_t( matcher_base_t ):
    """
    Instance of this class will match declarations by next criteria:
          - declaration name, also could be fully qualified name
            Example: `wstring` or `::std::wstring`
          - declaration type
            Example: :class:`class_t`, :class:`namespace_t`, :class:`enumeration_t`
          - location within file system ( file or directory )
    """
    def __init__( self, name=None, decl_type=None, header_dir=None, header_file=None ):
        """
        :param decl_type: declaration type to match by. For example :class:`enumeration_t`.
        :type decl_type: any class that derives from :class:`declaration_t` class

        :param name: declaration name, could be full name.
        :type name: str

        :param header_dir: absolute directory path
        :type header_dir: str

        :param header_file: absolute file path
        :type header_file: str

        """
        #An other option is that pygccxml will create absolute path using
        #os.path.abspath function. But I think this is just wrong, because abspath
        #builds path using current working directory. This behavior is fragile
        #and very difficult to find a bug.
        matcher_base_t.__init__( self )
        self.decl_type = decl_type
        self.__name = None
        self.__opt_is_tmpl_inst = None
        self.__opt_tmpl_name = None
        self.__opt_is_full_name = None
        self.__decl_name_only = None

        self._set_name( name )

        self.header_dir = header_dir
        self.header_file = header_file

        if self.header_dir:
            self.header_dir = utils.normalize_path( self.header_dir )
            if not os.path.isabs( self.header_dir ):
                raise RuntimeError( "Path to header directory should be absolute!" )

        if self.header_file:
            self.header_file = utils.normalize_path( self.header_file )
            if not os.path.isabs( self.header_file ):
                raise RuntimeError( "Path to header file should be absolute!" )

    def _get_name(self):
        return self.__name

    def _set_name( self, name ):
        self.__name = name
        if not self.__name:
            self.__opt_is_tmpl_inst = None
            self.__opt_tmpl_name = None
            self.__opt_is_full_name = None
            self.__decl_name_only = None
        else:
            self.__opt_is_tmpl_inst = templates.is_instantiation( self.__name )
            self.__opt_tmpl_name = templates.name( self.__name )
            if self.__opt_is_tmpl_inst:
                if '::' in self.__opt_tmpl_name:
                    self.__opt_is_full_name = True
                    self.__decl_name_only = self.__opt_tmpl_name.split('::')[-1]
                else:
                    self.__opt_is_full_name = False
                    self.__decl_name_only = self.__opt_tmpl_name
                self.__name = templates.normalize( name )
            else:
                if '::' in self.__name:
                    self.__opt_is_full_name = True
                    self.__decl_name_only = self.__name.split('::')[-1]
                else:
                    self.__opt_is_full_name = False
                    self.__decl_name_only = self.__name


    name = property( _get_name, _set_name )

    def __str__( self ):
        msg = []
        if not None is self.decl_type:
            msg.append( '(decl type==%s)' % self.decl_type.__name__ )
        if not None is self.name:
            msg.append( '(name==%s)' % self.name )
        if not None is self.header_dir:
            msg.append( '(header dir==%s)' % self.header_dir )
        if not None is self.header_file:
            msg.append( '(header file==%s)' % self.header_file )
        if not msg:
            msg.append( 'any' )
        return ' and '.join( msg )

    def __call__( self, decl ):
        if not None is self.decl_type:
            if not isinstance( decl, self.decl_type ):
                return False
        if not None is self.name:
            if not self.check_name( decl ):
                return False
        if not None is self.header_dir:
            if decl.location:
                decl_dir = os.path.abspath( os.path.dirname( decl.location.file_name ) )
                decl_dir = utils.normalize_path( decl_dir )
                if decl_dir[:len(self.header_dir)] != self.header_dir:
                    return False
            else:
                return False
        if not None is self.header_file:
            if decl.location:
                decl_file = os.path.abspath( decl.location.file_name )
                decl_file = utils.normalize_path( decl_file )
                if decl_file != self.header_file:
                    return False
            else:
                return False
        return True

    def check_name( self, decl ):
        assert not None is self.name
        if self.__opt_is_tmpl_inst:
            if not self.__opt_is_full_name:
                if self.name != templates.normalize( decl.name ) \
                   and self.name != templates.normalize( decl.partial_name ):
                    return False
            else:
                if self.name != templates.normalize( algorithm.full_name( decl, with_defaults=True ) ) \
                   and self.name != templates.normalize( algorithm.full_name( decl, with_defaults=False ) ):
                    return False
        else:
            if not self.__opt_is_full_name:
                if self.name != decl.name and self.name != decl.partial_name:
                    return False
            else:
                if self.name != algorithm.full_name( decl, with_defaults=True ) \
                   and self.name != algorithm.full_name( decl, with_defaults=False ):
                    return False
        return True

    def is_full_name(self):
        return self.__opt_is_full_name

    def _get_decl_name_only(self):
        return self.__decl_name_only
    decl_name_only = property( _get_decl_name_only )

class variable_matcher_t( declaration_matcher_t ):
    """
    Instance of this class will match variables by next criteria:
        - :class:`declaration_matcher_t` criteria
        - variable type. Example: :class:`int_t` or 'int'
    """
    def __init__( self, name=None, type=None, header_dir=None, header_file=None ):
        """
        :param type: variable type
        :type type: string or instance of :class:`type_t` derived class
        """
        declaration_matcher_t.__init__( self
                                        , name=name
                                        , decl_type=variable.variable_t
                                        , header_dir=header_dir
                                        , header_file=header_file )
        self.type = type

    def __call__( self, decl ):
        if not super( variable_matcher_t, self ).__call__( decl ):
            return False
        if not None is self.type:
            if isinstance( self.type, cpptypes.type_t ):
                if self.type != decl.type:
                    return False
            else:
                if self.type != decl.type.decl_string:
                    return False
        return True

    def __str__( self ):
        msg = [ super( variable_matcher_t, self ).__str__() ]
        if msg == [ 'any' ]:
            msg = []
        if not None is self.type:
            msg.append( '(value type==%s)' % str(self.type) )
        if not msg:
            msg.append( 'any' )
        return ' and '.join( msg )


class namespace_matcher_t( declaration_matcher_t ):
    """Instance of this class will match namespaces by name."""

    def __init__( self, name=None ):
        declaration_matcher_t.__init__( self, name=name, decl_type=namespace.namespace_t)

    def __call__( self, decl ):
        if self.name and decl.name == '':
            #unnamed namespace have same name as thier parent, we should prevent
            #this happens. The price is: user should search for unnamed namespace
            #directly.
            return False
        return super( namespace_matcher_t, self ).__call__( decl )


class calldef_matcher_t( declaration_matcher_t ):
    """
    Instance of this class will match callable by the following criteria:
       * :class:`declaration_matcher_t` criteria
       * return type. For example: :class:`int_t` or 'int'
       * argument types

    """

    def __init__( self, name=None, return_type=None, arg_types=None, decl_type=None, header_dir=None, header_file=None):
        """
        :param return_type: callable return type
        :type return_type: string or instance of :class:`type_t` derived class

        :type arg_types: list
        :param arg_types: list of function argument types. `arg_types` can contain.
                          Any item within the list could be string or instance
                          of :class:`type_t` derived class. If you don't want
                          some argument to participate in match you can put None.

        For example:

          .. code-block:: python

             calldef_matcher_t( arg_types=[ 'int &', None ] )

        will match all functions that takes 2 arguments, where the first one is
        reference to integer and second any
        """
        if None is decl_type:
            decl_type = calldef.calldef_t
        declaration_matcher_t.__init__( self
                                        , name=name
                                        , decl_type=decl_type
                                        , header_dir=header_dir
                                        , header_file=header_file )

        self.return_type = return_type
        self.arg_types = arg_types

    def __call__( self, decl ):
        if not super( calldef_matcher_t, self ).__call__( decl ):
            return False
        if not None is self.return_type \
           and not self.__compare_types( self.return_type, decl.return_type ):
            return False
        if self.arg_types:
            if isinstance( self.arg_types, (list, tuple)):
                if len(self.arg_types) != len( decl.arguments ):
                    return False
                for type_or_str, arg in zip( self.arg_types, decl.arguments ):
                    if None == type_or_str:
                        continue
                    else:
                        if not self.__compare_types( type_or_str, arg.type ):
                            return False
        return True

    def __compare_types( self, type_or_str, type ):
        assert type_or_str
        if type is None:
            return False
        if isinstance( type_or_str, cpptypes.type_t ):
            if type_or_str != type:
                return False
        else:
            if type_or_str != type.decl_string:
                return False
        return True

    def __str__( self ):
        msg = [ super( calldef_matcher_t, self ).__str__() ]
        if msg == [ 'any' ]:
            msg = []
        if not None is self.return_type:
            msg.append( '(return type==%s)' % str(self.return_type) )
        if self.arg_types:
            for i in range( len( self.arg_types ) ):
                if self.arg_types[i] is None:
                    msg.append( '(arg %d type==any)' % i )
                else:
                    msg.append( '(arg %d type==%s)' % ( i, str( self.arg_types[i] ) ) )
        if not msg:
            msg.append( 'any' )
        return ' and '.join( msg )


class operator_matcher_t( calldef_matcher_t ):
    """
    Instance of this class will match operators by next criteria:
        * :class:`calldef_matcher_t` criteria
        * operator symbol: =, !=, (), [] and etc
    """
    def __init__( self, name=None, symbol=None, return_type=None, arg_types=None, decl_type=None, header_dir=None, header_file=None):
        """
        :param symbol: operator symbol
        :type symbol: str
        """
        if None is decl_type:
            decl_type = calldef.operator_t
        calldef_matcher_t.__init__( self
                                    , name=name
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , decl_type=decl_type
                                    , header_dir=header_dir
                                    , header_file=header_file)
        self.symbol = symbol

    def __call__( self, decl ):
        if not super( operator_matcher_t, self ).__call__( decl ):
            return False
        if not None is self.symbol:
            if self.symbol != decl.symbol:
                return False
        return True

    def __str__( self ):
        msg = [ super( operator_matcher_t, self ).__str__() ]
        if msg == [ 'any' ]:
            msg = []
        if not None is self.symbol:
            msg.append( '(symbol==%s)' % str(self.symbol) )
        if not msg:
            msg.append( 'any' )
        return ' and '.join( msg )

class regex_matcher_t( matcher_base_t ):
    """
    Instance of this class will match declaration using regular expression.
    User should supply a function that will extract from declaration desired
    information as string. Later, this matcher will match that string using
    user regular expression.
    """
    def __init__( self, regex, function=None ):
        """
        :param regex: regular expression
        :type regex: string, an instance of this class will compile it for you

        :param function: function that will be called to get an information from
                         declaration as string. As input this function takes single
                         argument - reference to a declaration. Return value
                         should be string. If function is None, then the matcher
                         will use declaration name.

        """
        matcher_base_t.__init__(self)
        self.regex = re.compile( regex )
        self.function = function
        if None is self.function:
            self.function = lambda decl: decl.name

    def __call__( self, decl ):
        text = self.function( decl )
        return bool( self.regex.match( text ) )

    def __str__( self ):
        return '(regex=%s)' % self.regex

class access_type_matcher_t( matcher_base_t ):
    """
    Instance of this class will match declaration by its access type: public,
    private or protected. If declarations does not have access type, for example
    free function, then `False` will be returned.
    """

    def __init__( self, access_type ):
        """
        :param access_type: declaration access type, could be "public", "private", "protected"
        :type access_type: :class: `str`
        """
        matcher_base_t.__init__( self )
        self.access_type = access_type

    def __call__( self, decl ):
        if not isinstance( decl.parent, class_declaration.class_t ):
            return False
        return self.access_type == decl.parent.find_out_member_access_type( decl )

    def __str__( self ):
        return '(access type=%s)' % self.access_type

class virtuality_type_matcher_t( matcher_base_t ):
    """
    Instance of this class will match declaration by its virtual type: not virtual,
    virtual or pure virtual. If declarations does not have "virtual" property,
    for example free function, then `False` will be returned.
    """

    def __init__( self, virtuality_type ):
        """
        :param access_type: declaration access type
        :type access_type: :class:VIRTUALITY_TYPES defines few constants for your convenience.
        """
        matcher_base_t.__init__( self )
        self.virtuality_type = virtuality_type

    def __call__( self, decl ):
        if not isinstance( decl.parent, class_declaration.class_t ):
            return False
        return self.virtuality_type == decl.virtuality

    def __str__( self ):
        return '(virtuality type=%s)' % self.virtuality_type


class custom_matcher_t( matcher_base_t ):
    """
    Instance of this class will match declaration by user custom criteria.
    """

    def __init__( self, function ):
        """
        :param function: callable, that takes single argument - declaration instance
                         should return True or False
        """
        matcher_base_t.__init__( self )
        self.function = function

    def __call__( self, decl ):
        return bool( self.function( decl ) )

    def __str__( self ):
        return '(user criteria)'
