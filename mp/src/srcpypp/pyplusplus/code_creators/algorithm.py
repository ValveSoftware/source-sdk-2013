# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This module is a collection of unrelated algorithms, that works on code creators
tree.
"""

from pyplusplus.decl_wrappers.algorithm import *


import types

def _make_flatten_list( creator_or_creators ):
    from . import compound
    def proceed_single( creator ):
        answer = [ creator ]
        if not isinstance( creator, compound.compound_t):
            return answer
        for internal in creator.creators:
            if isinstance( creator, compound.compound_t):
                answer.extend( proceed_single( internal ) )
            else:
                answer.append( internal )
        return answer

    creators = []
    if isinstance( creator_or_creators, list ):
        creators.extend( creator_or_creators )
    else:
        creators.append( creator_or_creators )
    answer = []
    for creator in creators:
        answer.extend( proceed_single( creator ) )
    return answer

def make_flatten_generator( creator_or_creators ):
    from . import compound
    def proceed_single( creator ):
        yield creator
        if not isinstance( creator, compound.compound_t):
            return
        for internal in creator.creators:
            if isinstance( internal, compound.compound_t):
                for internal_internal in proceed_single( internal ):
                    yield internal_internal
            else:
                yield internal

    if isinstance( creator_or_creators, list ):
        for creator in creator_or_creators:
            for internal in proceed_single( creator ):
                yield internal
    else:
        for internal in proceed_single( creator_or_creators ):
            yield internal

"""
make_flatten - function that will create flat representation of code creators tree.
"""
make_flatten_list = _make_flatten_list
make_flatten = _make_flatten_list

class creator_finder:
    """
    This class is used as container for different find algorithms.
    """
    "creator_finder - this class used as namespace"

    @staticmethod
    def find_by_declaration( declaration_matcher, where, recursive=True ):
        """Finds code creator by declaration.
        declaration_matcher should be callable, that takes single argument
        declaration, and returns True or False
        where - code creator or list of code creators
        This function returns a list of all relevant code creators
        """
        from . import declaration_based #prevent cyclic import
        search_area = where
        if recursive:
            search_area = make_flatten_generator( where )
        return [inst for inst in search_area if isinstance( inst, declaration_based.declaration_based_t ) \
                                    and declaration_matcher( inst.declaration )]

    @staticmethod
    def find_by_declaration_single( declaration_matcher, where, recursive=True ):
        answer = creator_finder.find_by_declaration( declaration_matcher, where, recursive )
        if len( answer ) == 1:
            return answer[0]
        return None

    @staticmethod
    def find_by_class_instance( what, where, recursive=True ):
        search_area = where
        if recursive:
            search_area = make_flatten_generator( where )
        return [inst for inst in search_area if isinstance( inst, what )]

def make_id_creator( code_creator ):
    return lambda decl_string: create_identifier( code_creator, decl_string )

def complete_py_name( decl ):
    aliases = []
    current = decl
    while current:
        aliases.append( current.alias )
        current = current.parent
    del aliases[-1] # :: from the global namespace
    aliases.reverse()
    return '.'.join( aliases )

