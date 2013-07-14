# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines logger classes and few convenience methods, not related to the declarations tree"""

import os
import sys
import logging
import tempfile
from .fs_utils import files_walker
from .fs_utils import directories_walker

def _create_logger_( name ):
    """implementation details"""
    logger = logging.getLogger(name)
    handler = logging.StreamHandler()
    #handler.setFormatter( logging.Formatter( os.linesep + '%(levelname)s %(message)s' ) )
    handler.setFormatter( logging.Formatter( '%(levelname)s %(message)s' ) )
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    return logger

class loggers:
    """class-namespace, defines few loggers classes, used in the project"""

    cxx_parser = _create_logger_( 'pygccxml.cxx_parser' )
    """logger for C++ parser functionality

    If you set this logger level to DEBUG, you will be able to see the exact
    command line, used to invoke GCC-XML  and errors that occures during XML parsing
    """

    gccxml = cxx_parser #backward compatability

    pdb_reader = _create_logger_( 'pygccxml.pdb_reader' )
    """
    logger for MS .pdb file reader functionality
    """

    queries_engine = _create_logger_( 'pygccxml.queries_engine' )
    """logger for query engine functionality.

    If you set this logger level to DEBUG, you will be able to see what queries
    you do against declarations tree, measure performance and may be even to improve it.
    Query engine reports queries and whether they are optimized or not.
    """

    declarations_cache = _create_logger_( 'pygccxml.declarations_cache' )
    """logger for declarations tree cache functionality

    If you set this logger level to DEBUG, you will be able to see what is exactly
    happens, when you read the declarations from cache file. You will be able to
    decide, whether it worse for you to use this or that cache strategy.
    """

    root = logging.getLogger( 'pygccxml' )
    """root logger exists for your convenience only"""

    all = [ root, cxx_parser, queries_engine, declarations_cache, pdb_reader ]
    """contains all logger classes, defined by the class"""

def remove_file_no_raise(file_name ):
    """removes file from disk, if exception is raised, it silently ignores it"""
    try:
        if os.path.exists(file_name):
            os.remove( file_name )
    except Exception as error:
        loggers.root.error( "Error ocured while removing temprorary created file('%s'): %s"
                            % ( file_name, str( error ) ) )

def create_temp_file_name(suffix, prefix=None, dir=None):
    """small convenience function that creates temporal file.

    This function is a wrapper aroung Python built-in function - tempfile.mkstemp
    """
    if not prefix:
        prefix = tempfile.gettempprefix()
    fd, name = tempfile.mkstemp( suffix=suffix, prefix=prefix, dir=dir )
    file_obj = os.fdopen( fd )
    file_obj.close()
    return name

def normalize_path( some_path ):
    """return os.path.normpath( os.path.normcase( some_path ) )"""
	# Don't change the case of the normalized path!
    return os.path.normpath( some_path )
    #original: return os.path.normpath( os.path.normcase( some_path ) )

def contains_parent_dir( fpath, dirs ):
    """returns bool( filter( lambda dir: fpath.startswith( dir ), dirs ) )
    precondition: dirs and fpath should be normalize_path'ed before calling this function
    """
    f = lambda dir_: fpath.startswith( dir_ )
    return bool( list(filter( f, dirs )) )


def get_architecture():
    """returns computer architecture: 32 or 64.

    The guess is based on maxint.
    """
    if sys.maxsize == 2147483647:
        return 32
    elif sys.maxsize == 9223372036854775807:
        return 64
    else:
        raise RuntimeError( "Unknown architecture" )


#The following code is cut-and-paste from this post:
#http://groups.google.com/group/comp.lang.python/browse_thread/thread/5b71896c06bd0f76/
#Thanks to Michele Simionato, for it
class cached(property):
    'Convert a method into a cached attribute'
    def __init__(self, method):
        private = '_' + method.__name__
        def fget(s):
            try:
                return getattr(s, private)
            except AttributeError:
                value = method(s)
                setattr(s, private, value)
                return value
        def fdel(s):
            del s.__dict__[private]
        super(cached, self).__init__(fget, fdel=fdel)

    @staticmethod
    def reset(self):
        cls = self.__class__
        for name in dir(cls):
            attr = getattr(cls, name)
            if isinstance(attr, cached):
                delattr(self, name)

class enum( object ):
    """
    Usage example:

    .. code-block:: python

       class fruits(enum):
           apple = 0
           orange = 1

       fruits.has_value( 1 )
       fruits.name_of( 1 )

    """

    @classmethod
    def has_value( cls, enum_numeric_value ):
        for name, value in cls.__dict__.items():
            if enum_numeric_value == value:
                return True
        else:
            return False

    @classmethod
    def name_of( cls, enum_numeric_value ):
        for name, value in cls.__dict__.items():
            if enum_numeric_value == value:
                return name
        else:
            raise RuntimeError( 'Unable to find name for value(%d) in enumeration "%s"'
                                % ( enum_numeric_value, cls.__name__ ) )

class native_compiler:
    """provides information about the compiler, which was used to build the Python executable"""

    @staticmethod
    def get_version():
        if 'nt' != os.name:
            return None #not implemented yet
        else:
            from distutils import msvccompiler
            return ( 'msvc', str( msvccompiler.get_build_version() ) )

    @staticmethod
    def get_gccxml_compiler():
        compiler = native_compiler.get_version()
        if not compiler:
            return None
        else:
            n = compiler[1].replace( '.', '' )
            if n.endswith('0'):
                n = n[:-1]
            return compiler[0] + n.replace( '.', '' )


