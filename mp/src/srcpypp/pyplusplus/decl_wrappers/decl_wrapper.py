# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines base class for all code generator configuration classes"""

from . import algorithm
from pyplusplus import _logging_
from pygccxml import declarations
from pyplusplus import messages

class CODE_GENERATOR_TYPES:
    BOOST_PYTHON = 'Boost.Python'
    CTYPES = 'ctypes'
    all = [ BOOST_PYTHON, CTYPES ]

class decl_wrapper_t(object):
    """code generator declaration configuration base class

    This class contains configuration that could be applied to all declarations.
    """

    SPECIAL_TYPEDEF_PICK_ANY = True

    def __init__(self):
        object.__init__(self)
        self._alias = None
        self._ignore = False
        self._already_exposed = False
        self._exportable = None
        self._exportable_reason = None
        self._documentation = None
        self.__msgs_to_ignore = set()
        self._include_files = []
        self._code_generator = None

    @property
    def code_generator( self ):
        """code generator type, could be Boost.Python or ctypes"""
        return self._code_generator

    @property
    def logger( self ):
        """reference to :attr:`_logging_.loggers.declarations`"""
        return _logging_.loggers.declarations

    def _get_documentation( self ):
        return self._documentation
    def _set_documentation( self, value ):
        self._documentation = value
    documentation = property( _get_documentation, _set_documentation
                             , doc="exposed declaration Python documentation string" )

    def _generate_valid_name(self, name=None):
        if name == None:
            name = self.name
        return algorithm.create_valid_name( name )

    def __select_alias_directives( self, be_smart ):
        if not isinstance( self, declarations.class_types ):
            return []
        typedefs = list( set( [typedef for typedef in self.aliases if typedef.is_directive] ) )
        if decl_wrapper_t.SPECIAL_TYPEDEF_PICK_ANY:
            if typedefs and be_smart:
                longest_name_len = 0
                longest_typedef = None
                for typedef in typedefs:
                    typedef_name_len = len( typedef.name )
                    if longest_name_len < typedef_name_len:
                        longest_name_len = typedef_name_len
                        longest_typedef = typedef
                return [longest_typedef]
            else:
                return typedefs
        else:
            return typedefs

    def _get_alias(self):
        if not self._alias:
            directives = self.__select_alias_directives(be_smart=True)
            if 1 == len( directives ):
                self._alias = directives[0].name
            else:
                if declarations.templates.is_instantiation( self.name ):
                    container_aliases = [ 'value_type', 'key_type', 'mapped_type' ]
                    if isinstance( self, declarations.class_t ) \
                        and 1 == len( set( [typedef.name for typedef in self.aliases] ) ) \
                        and self.aliases[0].name not in container_aliases:
                            self._alias = self.aliases[0].name
                    else:
                        self._alias = algorithm.create_valid_name( self.partial_name )
                else:
                    if declarations.is_class( self ) or declarations.is_class_declaration( self ):
                        self._alias = algorithm.create_valid_name( self.partial_name )
                    else:
                        self._alias = self.partial_name
        return self._alias
    def _set_alias(self, alias):
        self._alias = alias
    alias = property( _get_alias, _set_alias
                      , doc="The name under which, Python will know the declaration. Code generators: ctypes, Boost.Python")

    def rename( self, new_name ):
        """give new name to the declaration, under which Python will know the declaration

        Code generators: ctypes, Boost.Python
        """
        self.alias = new_name

    def _get_ignore( self ):
        return self._ignore
    def _set_ignore( self, value ):
        self._ignore = value
    ignore = property( _get_ignore, _set_ignore
                       , doc="Boolean flag, which says whether to export declaration to Python or not. Code generators: ctypes, Boost.Python" )

    def get_already_exposed( self ):
        return self._already_exposed
    def set_already_exposed( self, value ):
        self._already_exposed = value
    already_exposed = property( get_already_exposed, set_already_exposed
                                , doc="boolean flag, which says whether the declaration is already exposed or not" )

    def exclude( self, compilation_errors=False ):
        """exclude "self" and child declarations from being exposed.

        If compile_time_errors is True, than only declarations, which will cause
        compilation error will be excluded

        Code generators: ctypes, Boost.Python
        """
        self.ignore = True

    def include( self, already_exposed=False ):
        """include "self" and child declarations to be exposed.

        Code generators: ctypes, Boost.Python.
        """
        self.ignore = False
        self.already_exposed = already_exposed

    def why_not_exportable( self ):
        """return the reason( string ) that explains why this declaration could not be exported

        If declaration could be exported, than method will return None
        """
        if None is self._exportable_reason:
            self.get_exportable()
        return self._exportable_reason

    def _exportable_impl( self ):
        return ''

    def get_exportable( self ):
        """return True if declaration could be exposed to Python, False otherwise"""
        if self._exportable is None:
            if self.name.startswith( '__' ) or '.' in self.name:
                self._exportable_reason = messages.W1000
            elif self.location and self.location.file_name == "<internal>":
                self._exportable_reason = messages.W1001
            elif self.is_artificial \
                 and not isinstance( self, ( declarations.class_t, declarations.enumeration_t ) ):
                self._exportable_reason = messages.W1002
            else:
                self._exportable_reason = self._exportable_impl( )
            self._exportable = not bool( self._exportable_reason )
        return self._exportable
    def set_exportable( self, exportable ):
        """change "exportable" status

        This function should be use in case `Py++` made a mistake and signed the
        declaration as non-exportable."""
        self._exportable = exportable

    exportable = property( get_exportable, set_exportable
                          , doc="Returns True if declaration could be exported to Python, otherwise False" )

    def _readme_impl( self ):
        return []

    def readme( self, skip_ignored=True ):
        """return important information( hints/tips/warning message ) `Py++` has about
        this declaration.

        skip_ignored argument allows you to control the information reported to you.
        For more information please read documentation about warnings.
        """
        msgs = []
        if not self.exportable:
            msgs.append( self.why_not_exportable() )

        if declarations.templates.is_instantiation( self.name ) \
           and self.alias == self._generate_valid_name():
            msgs.append( messages.W1043 % self.alias )

        directives = self.__select_alias_directives(be_smart=False)
        if 1 < len( directives ):
            msgs.append( messages.W1048
                         % ( self.alias, ', '.join( [typedef.name for typedef in directives] ) ) )

        msgs.extend( self._readme_impl() )

        return messages.filter_disabled_msgs( msgs, self.__msgs_to_ignore )

    @property
    def disabled_messages( self ):
        """list of messages to ignore"""
        return self.__msgs_to_ignore
    disabled_messaged = disabled_messages

    def disable_messages( self, *args ):
        """
        disable messages - `Py++` will not report the disabled messages
        Usage example:

        .. code-block:: python

           decl.disable_messages( messages.W1001, messages.W1040 )
        """
        for msg in args:
            msg_id = messages.find_out_message_id( msg )
            if not msg_id:
                raise RuntimeError( "Unable to find out message id. The message is: " + msg )
            self.__msgs_to_ignore.add( msg )
    disable_warnings = disable_messages

    @property
    def include_files( self ):
        """list of header files, to be included from the file, the generated code will be placed-in"""
        return self._include_files
