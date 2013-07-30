# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
free function invocation parser

The parser is able to extract function name and list of arguments from a function
invocation statement. For example, for the following code

.. code-block:: c++

  do_something( x1, x2, x3 )

the parser will extract
- function name - `do_something`
- argument names - `[ x1, x2, x3 ]`

"""

from . import pattern_parser

__THE_PARSER = pattern_parser.parser_t( '(', ')', ',' )

def is_call_invocation( decl_string ):
    """
    returns True if `decl_string` is function invocation and False otherwise

    :param decl_string: string that should be checked for pattern presence
    :type decl_string: str

    :rtype: bool
    """
    global __THE_PARSER
    return __THE_PARSER.has_pattern( decl_string )

def name( decl_string ):
    """
    returns name of function

    :type decl_string: str
    :rtype: str
    """
    global __THE_PARSER
    return __THE_PARSER.name( decl_string )

def args( decl_string ):
    """
    returns list of function arguments

    :type decl_string: str
    :rtype: [str]
    """
    global __THE_PARSER
    return __THE_PARSER.args( decl_string )

NOT_FOUND = __THE_PARSER.NOT_FOUND
def find_args( text, start=None ):
    """
    finds arguments within function invocation.

    :type text: str
    :rtype: [ arguments ] or :data:NOT_FOUND if arguments could not be found
    """
    global __THE_PARSER
    return __THE_PARSER.find_args( text, start )

def split( decl_string ):
    """returns (name, [arguments] )"""
    global __THE_PARSER
    return __THE_PARSER.split( decl_string )

def split_recursive( decl_string ):
    """returns [(name, [arguments])]"""
    global __THE_PARSER
    return __THE_PARSER.split_recursive( decl_string )

def join( name, args, arg_separator=None ):
    """returns name( argument_1, argument_2, ..., argument_n )"""
    global __THE_PARSER
    return __THE_PARSER.join( name, args, arg_separator )
