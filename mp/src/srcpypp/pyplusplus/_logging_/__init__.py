# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#TODO: find better place for it

"""defines logger classes"""

import os
import sys
import logging
import io
from .multi_line_formatter import multi_line_formatter_t

def create_handler( stream=None ):
    handler = None
    if stream:
        handler = logging.StreamHandler(stream)
    else:
        handler = logging.StreamHandler(stream)
    handler.setFormatter( multi_line_formatter_t( os.linesep + '%(levelname)s: %(message)s' ) )
    return handler

def _create_logger_( name, stream=None ):    
    """implementation details"""
    logger = logging.getLogger(name)
    logger.propagate = False
    logger.addHandler( create_handler(stream) )
    logger.setLevel(logging.INFO)
    return logger

class loggers:
    """class-namespace, defines few loggers classes, used in the project"""
    stream = None
    
    file_writer = _create_logger_( 'pyplusplus.file_writer' )
    """logger for classes that write code to files"""
    
    declarations = _create_logger_( 'pyplusplus.declarations' )
    """logger for declaration classes
    
    This is very import logger. All important messages: problems with declarations,
    warnings or hints are written to this logger.
    """
    
    module_builder = _create_logger_( 'pyplusplus.module_builder' )
    """logger that in use by :class:`module_builder.module_builder_t` class.

    Just another logger. It exists mostly for `Py++` developers.
    """
    
    #root logger exists for configuration purpose only
    root = logging.getLogger( 'pyplusplus' )
    """root logger exists for your convenience only"""
    
    all = [ root, file_writer, module_builder, declarations ]
    """contains all logger classes, defined by the class"""

    @staticmethod
    def make_inmemory():
        loggers.stream = io.StringIO()
        for logger in loggers.all:
            for h in logger.handlers[:]:
                logger.removeHandler( h )
            logger.addHandler( create_handler( loggers.stream ) )
