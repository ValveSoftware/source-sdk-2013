# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import sys

from pyplusplus import _logging_
from pyplusplus import decl_wrappers

class module_builder_t(object):
    """base class for different module builders."""

    def __init__( self, global_ns=None, encoding='ascii' ):
        """
        """
        object.__init__( self )
        self.logger = _logging_.loggers.module_builder
        self.__encoding = encoding
        self.__global_ns = global_ns

    def __get_global_ns( self ):
        if not self.__global_ns:
            raise RuntimeError( "Reference to global namespace declaration was not set." )
        return self.__global_ns
    def __set_global_ns( self, global_ns ):
        self.__global_ns = global_ns

    global_ns = property( __get_global_ns, __set_global_ns
                          ,  doc="""reference to global namespace""" )

    @property
    def encoding( self ):
        return self.__encoding

    def run_query_optimizer(self):
        """
        It is possible to optimize time that takes to execute queries. In most cases
        this is done from the :meth:`__init__` method. But there are use-case,
        when you need to disable optimizer and run it later.
        """
        self.global_ns.init_optimizer()

    def print_declarations(self, decl=None, detailed=True, recursive=True, writer=sys.stdout.write):
        """
        This function will print detailed description of all declarations or
        some specific one.

        :param decl: optional, if passed, then only it will be printed
        :type decl: instance of :class:`decl_wrappers.decl_wrapper_t` class
        """
        if None is decl:
            decl = self.global_ns
        decl_wrappers.print_declarations( decl, detailed, recursive, writer )

    #select decl(s) interfaces
    def decl( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.decl( name=name
                                    , function=function
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive)

    def decls( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.decls( name=name
                                     , function=function
                                     , header_dir=header_dir
                                     , header_file=header_file
                                     , recursive=recursive
                                     , allow_empty=allow_empty)

    def class_( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.class_( name=name
                                      , function=function
                                      , header_dir=header_dir
                                      , header_file=header_file
                                      , recursive=recursive)

    def classes( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.classes( name=name
                                       , function=function
                                       , header_dir=header_dir
                                       , header_file=header_file
                                       , recursive=recursive
                                       , allow_empty=allow_empty)

    def variable( self, name=None, function=None, type=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.variable( name=name
                                        , function=function
                                        , type=type
                                        , header_dir=header_dir
                                        , header_file=header_file
                                        , recursive=recursive)
    var = variable

    def variables( self, name=None, function=None, type=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.variables( name=name
                                         , function=function
                                         , type=type
                                         , header_dir=header_dir
                                         , header_file=header_file
                                         , recursive=recursive
                                         , allow_empty=allow_empty)
    vars = variables

    def calldef( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.calldef( name=name
                                       , function=function
                                       , return_type=return_type
                                       , arg_types=arg_types
                                       , header_dir=header_dir
                                       , header_file=header_file
                                       , recursive=recursive )

    def calldefs( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.calldefs( name=name
                                        , function=function
                                        , return_type=return_type
                                        , arg_types=arg_types
                                        , header_dir=header_dir
                                        , header_file=header_file
                                        , recursive=recursive
                                        , allow_empty=allow_empty)

    def operator( self, name=None, symbol=None, return_type=None, arg_types=None, decl_type=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.operator( name=name
                                        , symbol=symbol
                                        , return_type=return_type
                                        , arg_types=arg_types
                                        , header_dir=header_dir
                                        , header_file=header_file
                                        , recursive=recursive )

    def operators( self, name=None, symbol=None, return_type=None, arg_types=None, decl_type=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.operators( name=name
                                         , symbol=symbol
                                         , return_type=return_type
                                         , arg_types=arg_types
                                         , header_dir=header_dir
                                         , header_file=header_file
                                         , recursive=recursive 
                                         , allow_empty=allow_empty )

    def member_function( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.member_function( name=name
                                               , function=function
                                               , return_type=return_type
                                               , arg_types=arg_types
                                               , header_dir=header_dir
                                               , header_file=header_file
                                               , recursive=recursive )
    mem_fun = member_function

    def member_functions( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.member_functions( name=name
                                                , function=function
                                                , return_type=return_type
                                                , arg_types=arg_types
                                                , header_dir=header_dir
                                                , header_file=header_file
                                                , recursive=recursive
                                                , allow_empty=allow_empty)

    mem_funs = member_functions

    def constructor( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.constructor( name=name
                                           , function=function
                                           , return_type=return_type
                                           , arg_types=arg_types
                                           , header_dir=header_dir
                                           , header_file=header_file
                                           , recursive=recursive )

    def constructors( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.constructors( name=name
                                            , function=function
                                            , return_type=return_type
                                            , arg_types=arg_types
                                            , header_dir=header_dir
                                            , header_file=header_file
                                            , recursive=recursive
                                            , allow_empty=allow_empty)

    def member_operator( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.member_operator( name=name
                                               , symbol=symbol
                                               , function=function
                                               , return_type=return_type
                                               , arg_types=arg_types
                                               , header_dir=header_dir
                                               , header_file=header_file
                                               , recursive=recursive )

    def member_operators( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.member_operators( name=name
                                                , symbol=symbol
                                                , function=function
                                                , return_type=return_type
                                                , arg_types=arg_types
                                                , header_dir=header_dir
                                                , header_file=header_file
                                                , recursive=recursive
                                                , allow_empty=allow_empty )

    def casting_operator( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.casting_operator( name=name
                                                , function=function
                                                , return_type=return_type
                                                , arg_types=arg_types
                                                , header_dir=header_dir
                                                , header_file=header_file
                                                , recursive=recursive )

    def casting_operators( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.casting_operators( name=name
                                                 , function=function
                                                 , return_type=return_type
                                                 , arg_types=arg_types
                                                 , header_dir=header_dir
                                                 , header_file=header_file
                                                 , recursive=recursive
                                                 , allow_empty=allow_empty)

    def enumeration( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.enumeration( name=name
                                           , function=function
                                           , header_dir=header_dir
                                           , header_file=header_file
                                           , recursive=recursive)
    enum = enumeration

    def enumerations( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.scopedef_t` class documentation"""
        return self.global_ns.enumerations( name=name
                                            , function=function
                                            , header_dir=header_dir
                                            , header_file=header_file
                                            , recursive=recursive
                                            , allow_empty=allow_empty)

    enums = enumerations

    def namespace( self, name=None, function=None, recursive=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.namespace( name=name
                                         , function=function
                                         , recursive=recursive )

    def namespaces( self, name=None, function=None, recursive=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.namespaces( name=name
                                          , function=function
                                          , recursive=recursive)

    def free_function( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.free_function( name=name
                                             , function=function
                                             , return_type=return_type
                                             , arg_types=arg_types
                                             , header_dir=header_dir
                                             , header_file=header_file
                                             , recursive=recursive )
    free_fun = free_function

    def free_functions( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.free_functions( name=name
                                              , function=function
                                              , return_type=return_type
                                              , arg_types=arg_types
                                              , header_dir=header_dir
                                              , header_file=header_file
                                              , recursive=recursive
                                              , allow_empty=allow_empty)
    free_funs = free_functions

    def free_operator( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.free_operator( name=name
                                             , symbol=symbol
                                             , function=function
                                             , return_type=return_type
                                             , arg_types=arg_types
                                             , header_dir=header_dir
                                             , header_file=header_file
                                             , recursive=recursive )

    def free_operators( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.free_operators( name=name
                                              , symbol=symbol
                                              , function=function
                                              , return_type=return_type
                                              , arg_types=arg_types
                                              , header_dir=header_dir
                                              , header_file=header_file
                                              , recursive=recursive
                                              , allow_empty=allow_empty )
                                              
    def typedef( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.typedef( name=name
                                  , function=function
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)

    def typedefs( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """Please see :class:`decl_wrappers.namespace_t` class documentation"""
        return self.global_ns.typedefs( name=name
                                    , function=function
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)
