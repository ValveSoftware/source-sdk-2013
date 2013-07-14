# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines declarations visitor class interface
"""

class decl_visitor_t(object):
    """
    declarations visitor interface
    
    All functions within this class should be redefined in derived classes.
    """
    def __init__(self):
        object.__init__(self)
    
    def visit_member_function( self ):
        raise NotImplementedError()
        
    def visit_constructor( self ):
        raise NotImplementedError()
    
    def visit_destructor( self ):
        raise NotImplementedError()
    
    def visit_member_operator( self ):
        raise NotImplementedError()
    
    def visit_casting_operator( self ):
        raise NotImplementedError()
    
    def visit_free_function( self ):
        raise NotImplementedError()
    
    def visit_free_operator( self ):
        raise NotImplementedError()

    def visit_class_declaration(self ):
        raise NotImplementedError()
        
    def visit_class(self ):
        raise NotImplementedError()
        
    def visit_enumeration(self ):
        raise NotImplementedError()
        
    def visit_namespace(self ):
        raise NotImplementedError()
        
    def visit_typedef(self ):
        raise NotImplementedError()
        
    def visit_variable(self ):
        raise NotImplementedError()