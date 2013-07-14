# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines :class:`scopedef_t` class"""

import time
from . import algorithm
from . import templates
from . import declaration
from . import mdecl_wrapper
from pygccxml import utils
from . import matcher as matcher_module
import collections

class scopedef_t( declaration.declaration_t ):
    """
    Base class for :class:`namespace_t` and :class:`class_t` classes.

    This is the base class for all declaration classes that may have
    children nodes. The children can be accessed via the :attr:`scopedef_t.declarations`
    property.

    Also this class provides "get/select/find" interface. Using this class you
    can get instance or instances of internal declaration(s).

    You can find declaration(s) using next criteria:

        1. `name` - declaration name, could be full qualified name

        2. `header_dir` - directory, to which belongs file, that the declaration was declared in.
           `header_dir` should be absolute path.

        3. `header_file` - file that the declaration was declared in.

        4. `function` - user ( your ) custom criteria. The interesting thing is that
           this function will be joined with other arguments ( criteria ).

        5. `recursive` - the search declaration range, if True will be search in
           internal declarations too.


    Every ""query"" API, takes name or function as the first argument.

    .. code-block:: python

        global_namespace.member_function( "do_something )

    the statement returns reference to member function named "do_something".
    If there the function doesn't exist or more than one function exists,
    an exception is raised.

    If you want to query for many declarations, use other function(s):

    .. code-block:: python

        do_something = global_namespace.member_functions( "do_something )

    the statement returns :class:`mdecl_wrapper_t` instance. That object will save
    you writing `for` loops. For more information see :class:`the class <mdecl_wrapper_t>`
    documentation.
    """

    RECURSIVE_DEFAULT = True
    ALLOW_EMPTY_MDECL_WRAPPER = False

    declaration_not_found_t = matcher_module.matcher.declaration_not_found_t
    multiple_declarations_found_t = matcher_module.matcher.multiple_declarations_found_t

    _impl_matchers = {} #this class variable is used to prevent recursive imports
    _impl_decl_types = {} #this class variable is used to prevent recursive imports
    _impl_all_decl_types = [] #this class variable is used to prevent recursive imports

    def __init__( self, name=''):
        declaration.declaration_t.__init__( self, name )

        self._optimized = False
        self._type2decls = {}
        self._type2name2decls = {}
        self._type2decls_nr = {}
        self._type2name2decls_nr = {}
        self._all_decls = None
        self._all_decls_not_recursive = None

    def _get_logger( self ):
        return utils.loggers.queries_engine
    _logger = property( _get_logger, doc="reference to :attr:`pygccxml.utils.loggers.queries_engine` logger" )

    def _get__cmp__scope_items(self):
        """implementation details"""
        raise NotImplementedError()

    def _get__cmp__items(self):
        """implementation details"""
        items = []
        if self._optimized:
            #in this case we don't need to build class internal declarations list
            items.append( self._sorted_list( self._all_decls_not_recursive ) )
        else:
            items.append( self._sorted_list( self.declarations ) )
        items.extend( self._get__cmp__scope_items() )
        return items

    def __eq__(self, other):
        if not declaration.declaration_t.__eq__( self, other ):
            return False
        return self._sorted_list( self.declarations[:] ) \
               == other._sorted_list( other.declarations[:] )
        #self_decls = self._all_decls_not_recursive
        #if not self._optimized:
            #self_decls = self._sorted_list( self.declarations[:] )
        #other_decls = other._all_decls_not_recursive[:]
        #if not other._optimized:
            #other_decls = other._sorted_list( other.declarations[:] )
        #else:
            #return  self_decls == other_decls

    def __hash__(self):
        return super.__hash__(self)

    def _get_declarations_impl(self):
        raise NotImplementedError()

    def _get_declarations(self):
        if self._optimized:
            return self._all_decls_not_recursive
        else:
            return self._get_declarations_impl()
    declarations = property( _get_declarations, doc="list of children :class:`declarations <declaration_t>`" )

    def remove_declaration( self, decl ):
        raise NotImplementedError()

    def __decl_types( self, decl ):
        """implementation details"""
        types = []
        bases = list( decl.__class__.__bases__ )
        visited = set()
        if 'pygccxml' in decl.__class__.__module__:
            types.append( decl.__class__ )
        while bases:
            base = bases.pop()
            if base is declaration.declaration_t:
                continue
            if base in visited:
                continue
            if 'pygccxml' not in base.__module__:
                continue
            types.append( base )
            bases.extend( base.__bases__ )
        return types

    def clear_optimizer(self):
        """Cleans query optimizer state"""
        self._optimized = False
        self._type2decls = {}
        self._type2name2decls = {}
        self._type2decls_nr = {}
        self._type2name2decls_nr = {}
        self._all_decls = None
        self._all_decls_not_recursive = None

        for decl in self.declarations:
            if isinstance( decl, scopedef_t ):
                decl.clear_optimizer()

    def init_optimizer(self):
        """
        Initializes query optimizer state.

        There are 4 internals hash tables:
            1. from type to declarations
            2. from type to declarations for non-recursive queries
            3. from type to name to declarations
            4. from type to name to declarations for non-recursive queries

        Almost every query includes declaration type information. Also very
        common query is to search some declaration(s) by name or full name.
        Those hash tables allows to search declaration very quick.
        """
        if self.name == '::':
            self._logger.debug( "preparing data structures for query optimizer - started" )
        start_time = time.clock()

        self.clear_optimizer()

        for dtype in scopedef_t._impl_all_decl_types:
            self._type2decls[ dtype ] = []
            self._type2decls_nr[ dtype ] = []
            self._type2name2decls[ dtype ] = {}
            self._type2name2decls_nr[ dtype ] = {}

        self._all_decls_not_recursive = self.declarations
        self._all_decls = algorithm.make_flatten( self._all_decls_not_recursive )
        for decl in self._all_decls:
            types = self.__decl_types( decl )
            for type_ in types:
                self._type2decls[ type_ ].append( decl )
                name2decls = self._type2name2decls[ type_ ]
                if decl.name not in name2decls:
                    name2decls[ decl.name ] = []
                name2decls[ decl.name ].append( decl )
                if self is decl.parent:
                    self._type2decls_nr[ type_ ].append( decl )
                    name2decls_nr = self._type2name2decls_nr[ type_ ]
                    if decl.name not in name2decls_nr:
                        name2decls_nr[ decl.name ] = []
                    name2decls_nr[ decl.name ].append( decl )

        for decl in self._all_decls_not_recursive:
            if isinstance( decl, scopedef_t ):
                decl.init_optimizer()
        if self.name == '::':
            self._logger.debug( "preparing data structures for query optimizer - done( %f seconds ). "
                                % ( time.clock() - start_time ) )
        self._optimized = True

    def _build_operator_function( self, name, function ):
        if isinstance( name, collections.Callable):
            return name
        else:
            return function

    def _build_operator_name( self, name, function, symbol ):
        """implementation details"""
        def add_operator( sym ):
            if 'new' in sym or 'delete' in sym:
                return 'operator ' + sym
            else:
                return 'operator'+ sym
        if isinstance( name, collections.Callable) and None is function:
            name = None
        if name:
            if not 'operator' in name:
               name = add_operator( name )
            return name
        elif symbol:
            return add_operator( symbol )
        return name #both name and symbol are None

    def _on_rename( self ):
        for decl in self.decls(allow_empty=True):
            decl.cache.reset_name_based()
        #I am not sure whether to introduce this or not?
        #It could be very time consuming operation + it changes optimize query
        #data structures.
        #if self.parent:
        #    if self.parent._optimized:
        #        self.parent.init_optimizer()

    def __normalize_args( self, **keywds ):
        """implementation details"""
        if isinstance( keywds['name'], collections.Callable) and None is keywds['function']:
            keywds['function'] = keywds['name']
            keywds['name'] = None
        return keywds

    def __findout_recursive( self, **keywds ):
        """implementation details"""
        if None is keywds[ 'recursive' ]:
            return self.RECURSIVE_DEFAULT
        else:
            return keywds[ 'recursive' ]

    def __findout_allow_empty( self, **keywds ):
        """implementation details"""
        if None is keywds[ 'allow_empty' ]:
            return self.ALLOW_EMPTY_MDECL_WRAPPER
        else:
            return keywds[ 'allow_empty' ]

    def __findout_decl_type( self, match_class, **keywds ):
        """implementation details"""
        if 'decl_type' in keywds:
            return keywds['decl_type']

        matcher_args = keywds.copy()
        del matcher_args['function']
        del matcher_args['recursive']
        if 'allow_empty' in matcher_args:
            del matcher_args['allow_empty']

        matcher = match_class( **matcher_args )
        if matcher.decl_type:
            return matcher.decl_type
        return None

    def __create_matcher( self, match_class, **keywds ):
        """implementation details"""
        matcher_args = keywds.copy()
        del matcher_args['function']
        del matcher_args['recursive']
        if 'allow_empty' in matcher_args:
            del matcher_args['allow_empty']

        matcher = match_class( **matcher_args )
        if keywds['function']:
            self._logger.debug( 'running query: %s and <user defined function>' % str( matcher ) )
            return lambda decl: matcher( decl ) and keywds['function'](decl)
        else:
            self._logger.debug( 'running query: %s' % str( matcher ) )
            return matcher

    def __findout_range( self, name, decl_type, recursive ):
        """implementation details"""
        if not self._optimized:
            self._logger.debug( 'running non optimized query - optimization has not been done' )
            decls = self.declarations
            if recursive:
                decls = algorithm.make_flatten( self.declarations )
            if decl_type:
                decls = [d for d in decls if isinstance( d, decl_type )]
            return decls

        if name and templates.is_instantiation( name ):
            #templates has tricky mode to compare them, so lets check the whole
            #range
            name = None

        if name and decl_type:
            matcher = scopedef_t._impl_matchers[ scopedef_t.decl ]( name=name )
            if matcher.is_full_name():
                name = matcher.decl_name_only
            if recursive:
                self._logger.debug( 'query has been optimized on type and name' )
                if name in self._type2name2decls[decl_type]:
                    return self._type2name2decls[decl_type][name]
                else:
                    return []
            else:
                self._logger.debug( 'non recursive query has been optimized on type and name' )
                if name in self._type2name2decls_nr[decl_type]:
                    return self._type2name2decls_nr[decl_type][name]
                else:
                    return []
        elif decl_type:
            if recursive:
                self._logger.debug( 'query has been optimized on type' )
                return self._type2decls[ decl_type ]
            else:
                self._logger.debug( 'non recursive query has been optimized on type' )
                return self._type2decls_nr[ decl_type ]
        else:
            if recursive:
                self._logger.debug( 'query has not been optimized ( hint: query does not contain type and/or name )' )
                return self._all_decls
            else:
                self._logger.debug( 'non recursive query has not been optimized ( hint: query does not contain type and/or name )' )
                return self._all_decls_not_recursive

    def _find_single( self, match_class, **keywds ):
        """implementation details"""
        self._logger.debug( 'find single query execution - started' )
        start_time = time.clock()
        norm_keywds = self.__normalize_args( **keywds )
        matcher = self.__create_matcher( match_class, **norm_keywds )
        dtype = self.__findout_decl_type( match_class, **norm_keywds )
        recursive_ = self.__findout_recursive( **norm_keywds )
        decls = self.__findout_range( norm_keywds['name'], dtype, recursive_ )
        found = matcher_module.matcher.get_single( matcher, decls, False )
        self._logger.debug( 'find single query execution - done( %f seconds )' % ( time.clock() - start_time ) )
        return found

    def _find_multiple( self, match_class, **keywds ):
        """implementation details"""
        self._logger.debug( 'find all query execution - started' )
        start_time = time.clock()
        norm_keywds = self.__normalize_args( **keywds )
        matcher = self.__create_matcher( match_class, **norm_keywds )
        dtype = self.__findout_decl_type( match_class, **norm_keywds )
        recursive_ = self.__findout_recursive( **norm_keywds )
        allow_empty = self.__findout_allow_empty( **norm_keywds )
        decls = self.__findout_range( norm_keywds['name'], dtype, recursive_ )
        found = matcher_module.matcher.find( matcher, decls, False )
        mfound = mdecl_wrapper.mdecl_wrapper_t( found )
        self._logger.debug( '%d declaration(s) that match query' % len(mfound) )
        self._logger.debug( 'find single query execution - done( %f seconds )'
                     % ( time.clock() - start_time ) )
        if not mfound and not allow_empty:
            raise RuntimeError( "Multi declaration query returned 0 declarations." )
        return mfound

    def decl( self, name=None, function=None, decl_type=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.decl ]
                                  , name=name
                                  , function=function
                                  , decl_type=decl_type
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)

    def decls( self, name=None, function=None, decl_type=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.decl ]
                                    , name=name
                                    , function=function
                                    , decl_type=decl_type
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def class_( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to class declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.class_ ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.class_ ]
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)

    def classes( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of class declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.class_ ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.class_ ]
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def variable( self, name=None, function=None, type=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to variable declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.variable ]
                                  , name=name
                                  , function=function
                                  , type=type
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)
    var = variable #small alias

    def variables( self, name=None, function=None, type=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of variable declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.variable ]
                                    , name=name
                                    , function=function
                                    , type=type
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)
    vars = variables #small alias

    def calldef( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to "calldef" declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.calldef ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.calldef ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )

    def calldefs( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of :class:`calldef_t` declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.calldef ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.calldef ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def operator( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to operator declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.operator ]
                                  , name=self._build_operator_name( name, function, symbol )
                                  , symbol=symbol
                                  , function=self._build_operator_function( name, function )
                                  , decl_type=self._impl_decl_types[ scopedef_t.operator ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )

    def operators( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of operator declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.operator ]
                                    , name=self._build_operator_name( name, function, symbol )
                                    , symbol=symbol
                                    , function=self._build_operator_function( name, function )
                                    , decl_type=self._impl_decl_types[ scopedef_t.operator ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def member_function( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to member declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.member_function ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.member_function ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )
    mem_fun = member_function

    def member_functions( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of member function declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.member_function ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.member_function ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)
    mem_funs = member_functions

    def constructor( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to constructor declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.constructor ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.constructor ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )

    def constructors( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of constructor declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.constructor ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.constructor ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def member_operator( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to member operator declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.member_operator ]
                                  , name=self._build_operator_name( name, function, symbol )
                                  , symbol=symbol
                                  , function=self._build_operator_function( name, function )
                                  , decl_type=self._impl_decl_types[ scopedef_t.member_operator ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )
    mem_oper = member_operator
    def member_operators( self, name=None, function=None, symbol=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of member operator declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.member_operator ]
                                    , name=self._build_operator_name( name, function, symbol )
                                    , symbol=symbol
                                    , function=self._build_operator_function( name, function )
                                    , decl_type=self._impl_decl_types[ scopedef_t.member_operator ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)
    mem_opers = member_operators

    def casting_operator( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to casting operator declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.casting_operator ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.casting_operator ]
                                  , return_type=return_type
                                  , arg_types=arg_types
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive )

    def casting_operators( self, name=None, function=None, return_type=None, arg_types=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of casting operator declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.casting_operator ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.casting_operator ]
                                    , return_type=return_type
                                    , arg_types=arg_types
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def enumeration( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to enumeration declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.enumeration ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.enumeration ]
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)

    enum = enumeration
    """adding small aliase to enumeration method"""

    def enumerations( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of enumeration declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.enumeration ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.enumeration ]
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)
    #adding small aliase
    enums = enumerations

    def typedef( self, name=None, function=None, header_dir=None, header_file=None, recursive=None ):
        """returns reference to typedef declaration, that is matched defined criteria"""
        return self._find_single( self._impl_matchers[ scopedef_t.typedef ]
                                  , name=name
                                  , function=function
                                  , decl_type=self._impl_decl_types[ scopedef_t.typedef ]
                                  , header_dir=header_dir
                                  , header_file=header_file
                                  , recursive=recursive)

    def typedefs( self, name=None, function=None, header_dir=None, header_file=None, recursive=None, allow_empty=None ):
        """returns a set of typedef declarations, that are matched defined criteria"""
        return self._find_multiple( self._impl_matchers[ scopedef_t.typedef ]
                                    , name=name
                                    , function=function
                                    , decl_type=self._impl_decl_types[ scopedef_t.typedef ]
                                    , header_dir=header_dir
                                    , header_file=header_file
                                    , recursive=recursive
                                    , allow_empty=allow_empty)

    def __getitem__(self, name_or_function):
        """
        Allow simple name based find of declarations.  Internally just calls `decls` method.
        :param name_or_function:  Name of `decl` to lookup or finder function.
        """
        return self.decls(name_or_function)





