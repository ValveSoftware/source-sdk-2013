# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import types
from pyplusplus import decl_wrappers
from pyplusplus import code_repository

class code_creator_t(object):
    """
    code_creator_t is the base class for all code creators.

    This class defines the interface that every code creator should implement.
    Also it provides few convenience functions.

    The purpose of a code creator is the generation of a block of C++
    source code as it will appear in the final source code for the
    extension module. The source code is obtained by calling the :meth:`code_creator_t.create`
    method. Derived classes must implement the :meth:`code_creator_t._create_impl` method
    which is called by the :meth:`code_creator_t.create` method.
    """
    PYPLUSPLUS_NS_NAME = 'pyplusplus'
    __INDENTATION = '    '
    LINE_LENGTH = 80
    PARAM_SEPARATOR = ', '
    CODE_GENERATOR_TYPES = decl_wrappers.CODE_GENERATOR_TYPES

    def __init__(self):
        """Constructor.

        :param parent: Parent code creator.
        :type parent: code_creator_t
        """
        object.__init__(self)
        self._parent = None
        self._target_configuration = None
        self._works_on_instance = True
        self._code_generator = None

    @property
    def code_generator( self ):
        if self._code_generator is None:
            self._code_generator = self.top_parent.code_generator
        return self._code_generator

    def _get_works_on_instance(self):
        return self._works_on_instance
    def _set_works_on_instance(self, works_on_instance):
        self._works_on_instance = works_on_instance
    works_on_instance = property( _get_works_on_instance, _set_works_on_instance )

    def _get_parent( self ):
        return self._parent
    def _set_parent( self, new_parent ):
        if new_parent:
            assert isinstance( new_parent, code_creator_t )
        self._parent = new_parent
    """parent - reference to parent code creator"""
    parent = property( _get_parent, _set_parent,
                       doc="""Parent code creator or None if this is the root node.
                       @type: :class:`code_creators.code_creator_t`
                       """)

    def _get_target_configuration( self ):
        return self._target_configuration
    def _set_target_configuration( self, config ):
        self._target_configuration = config
    """target_configuration - reference to target_configuration_t class instance"""
    target_configuration = property( _get_target_configuration, _set_target_configuration,
                                     doc="""Target configuration.
                                     @type: :class:`target_configuration_t`
                                     """)

    @property
    def top_parent(self):
        """top_parent - reference to top parent code creator

        @type: :class:`code_creators.code_creator_t`
        """
        parent = self.parent
        me = self
        while True:
            if not parent:
                return me
            else:
                me = parent
                parent = me.parent

    def _create_impl(self):
        """
        function that all derived classes should implement. This function
        actually creates code and returns it. Return value of this function is
        string.

        :rtype: str
        """
        raise NotImplementedError()

    def create(self):
        """
        generates source code

        :rtype: str
        """
        code = self._create_impl()
        assert isinstance( code, str )
        return self.beautify( code )

    @staticmethod
    def unique_headers( headers ):
        used = set()
        uheaders = []
        for h in headers:
            if h not in used:
                used.add( h )
                uheaders.append( h )
        return uheaders

    def _get_system_files_impl( self ):
        """Return list of system header files the generated code depends on"""
        raise NotImplementedError(self.__class__.__name__)

    def get_system_files( self, recursive=False, unique=False, language='any' ):
        files = []
        if self.code_generator == self.CODE_GENERATOR_TYPES.BOOST_PYTHON:
            files.append( "boost/python.hpp" )
            files.append( code_repository.named_tuple.file_name )
        else:
            files.append( code_repository.ctypes_utils.file_name )
        files.extend( self._get_system_files_impl() )
        files = [_f for _f in files if _f]
        if unique:
            files = self.unique_headers( files )

        language = language.lower()
        if language == 'python':
            selector = lambda f: os.path.splitext( f )[1] in ( '.py' )
        elif language == 'c++':
            selector = lambda f: ( f.startswith( '<' ) and f.endswith('>') ) \
                                 or os.path.splitext( f )[1] in ( '.h', '.hpp', '.cpp' )
        else:
            selector = None

        return list(filter( selector, files ))

    def beautify( self, code ):
        """
        function that returns code without leading and trailing white spaces.

        :param code: A code block with C++ source code.
        :type code: str
        :rtype: str
        """
        assert isinstance( code, str )
        return code.strip()

    @staticmethod
    def indent( code, size=1 ):
        """
        function that implements code indent algorithm.

        :param code: C++/Python code block.
        :type code: str
        :param size: The number of indentation levels that the code is shifted
        :type size: int
        :rtype: str
        """
        assert isinstance( code, str )
        return code_creator_t.__INDENTATION * size\
               + code.replace( os.linesep
                               , os.linesep + code_creator_t.__INDENTATION * size )

    @staticmethod
    def unindent( code ):
        """
        function that implements code unindent algorithm.

        :param code: C++ code block.
        :type code: str
        :rtype: str
        """
        assert isinstance( code, str )
        if code.startswith(code_creator_t.__INDENTATION):
            code = code[ len( code_creator_t.__INDENTATION ):]
        return code.replace( os.linesep + code_creator_t.__INDENTATION
                               , os.linesep )

    @staticmethod
    def is_comment( line, language='C++' ):
        """
        function that returns true if content of the line is comment, otherwise
        false.

        :param line: C++ source code
        :type line: str
        :param language: the programming language, the line was written in. Possible values: C++, Python
        :type line: str

        :rtype: bool
        """
        assert isinstance( line, str )
        l = line.lstrip()
        if language == 'C++':
            return l.startswith( '//' ) or l.startswith( '/*' )
        elif language == 'Python':
            return l.startswith( '#' )
        else:
            raise RuntimeError( "Language %s is not supported. The possible values are: Python, C++"
                                % language )

    @staticmethod
    def iif( condition, true_, false_ ):
        if condition:
            return true_
        else:
            return false_


class separator_t(code_creator_t):
    """Creates Python import directive"""
    def __init__( self, num=1):
        code_creator_t.__init__(self)
        self.__code = os.linesep * num

    def _create_impl(self):
        return self.__code

    def _get_system_files_impl( self ):
        return []
