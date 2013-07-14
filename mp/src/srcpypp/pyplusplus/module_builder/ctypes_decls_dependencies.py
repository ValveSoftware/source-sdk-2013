# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


"""
This file contains algorithm, which calculates from exported symbols, all declaration
that should be exposed too.
"""

from pygccxml import declarations

decls_traits = ( declarations.class_traits
                 , declarations.enum_traits 
                 , declarations.class_declaration_traits )
                 
i_depend_on_them = declarations.dependency_info_t.i_depend_on_them

def get_decl_dependencies(decl):    
    dependencies = set()
    for dependency in i_depend_on_them( decl ):
        for traits in decls_traits:
            if traits.is_my_case( dependency ):
                dd = traits.get_declaration( dependency )
                dependencies.add( dd )
    return dependencies

def get_parent_classes( decl ):
    classes = set()
    parent = decl.parent
    while True:
        if isinstance( parent, declarations.namespace_t ):
            break
        classes.add( parent )
        parent = parent.parent
    return classes
        
def find_out_dependencies( included_decls ):
    dependencies = set()
    for d in included_decls:
        dependencies.update( get_parent_classes(d) )
        dependencies.update( get_decl_dependencies( d ) )
        
    visited = set()
    while dependencies:
        d = dependencies.pop()
        if d in visited:
            continue
        visited.add( d )
        if isinstance( d, declarations.class_t ):
            for var in d.vars( recursive=False, allow_empty=True ):
                dependencies.update( get_decl_dependencies( var ) )
            dependencies.update( [hi.related_class for hi in d.recursive_bases] )
            dependencies.update( get_parent_classes( d ) )
    return visited
